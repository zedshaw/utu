/*
 * Utu -- Saving The Internet With Hate
 *
 * Copyright (c) Zed A. Shaw 2005 (zedshaw@zedshaw.com)
 *
 * This file is modifiable/redistributable under the terms of the GNU
 * General Public License.
 *
 * You should have recieved a copy of the GNU General Public License along
 * with this program; see the file COPYING. If not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 0211-1307, USA.
 */

#include "crypto.h"

/* This needs to be here because APPLE is fucking retarded. */
ltc_math_descriptor ltc_mp;

/** Taken from bstraux.h and used to wipe memory before freeing it. */

#define bSecureDestroy(b) {	                                            \
bstring bstr__tmp = (b);	                                            \
	if (bstr__tmp && bstr__tmp->mlen > 0 && bstr__tmp->data) {          \
	    (void) memset (bstr__tmp->data, 0, (size_t) bstr__tmp->mlen);   \
	    bdestroy (bstr__tmp);                                           \
	}                                                                   \
}

inline int CryptState_create_key(CryptState *state);
inline int CryptState_create_shared_key(CryptState *state, Node *header, int nonced, CryptState_key_confirm_cb key_confirm);
inline int CryptState_finalize_session(CryptState *state);

static prng_state global_prng;
static int global_hash_idx;
static int global_prng_idx;
static int global_cipher_idx;

static int CryptState_initialized = 0;

int CryptState_init()
{
  assert(!errno && "bad errno before CryptState_init");
  int rc = 0;

  if(!CryptState_initialized) {
    #ifdef HAS_TFM
    ltc_mp = tfm_desc;
    #elif HAS_LTM
    ltc_mp = ltm_desc;
    #else
     #error "You need TFM or LTM first."
    #endif

    // setup the prng
    ltc_ok(register_prng(&fortuna_desc), "register fortuna");
    ltc_ok(fortuna_test(), "fortuna not working");
    global_prng_idx = find_prng(PRNG_NAME);

    rc = rng_make_prng(128, global_prng_idx, &global_prng, NULL);
    ltc_ok(rc, "init fortuna");

    // setup the symmetric cipher
    ltc_ok(register_cipher(&rijndael_desc), "register rijndael");
    ltc_ok(rijndael_test(), "rijndael not working");
    global_cipher_idx = find_cipher(CIPHER_NAME);

    // register hash
    ltc_ok(register_hash(&sha256_desc), "register sha256");
    ltc_ok(sha256_test(), "sha256 not working");
    global_hash_idx = find_hash(HASH_NAME);

    // setup the enc+auth mode
    ltc_ok(ccm_test(), "ccm not working");

    CryptState_initialized=1;
  }

  return 1;
  on_fail(return 0);
}

void CryptState_peer_destroy(CryptState *state, crypt_key_side my_key)
{
  assert_not(state, NULL);
  assert((my_key == CRYPT_MY_KEY || my_key == CRYPT_THEIR_KEY) && "my_key wrong");

  CryptPeer *peer = my_key ? &state->me : &state->them;

  ecc_free(&peer->key);
  if(peer->name != NULL) bSecureDestroy(peer->name);
  if(peer->session != NULL) bSecureDestroy(peer->session);
  memset(&peer->skey, 0, sizeof(symmetric_key));
  memset(&peer->nonce, 0, sizeof(CryptNonce));
}

void CryptState_destroy(CryptState *state)
{
  assert_not(state, NULL);

  CryptState_peer_destroy(state, CRYPT_MY_KEY);
  CryptState_peer_destroy(state, CRYPT_THEIR_KEY);

  free(state);
}

inline void CryptState_rand_sess(CryptState *state) 
{
  int rc = 0;

  assert_not(state, NULL);
  state->me.session = bfromcstralloc(KEY_LENGTH+1,"");
  assert_mem(state->me.session);

  rc = CryptState_rand(state->me.session->data, KEY_LENGTH);
  assert(rc && "failed to gather randomness");

  bsetsize(state->me.session, KEY_LENGTH);
}

CryptState *CryptState_create(bstring name, bstring prvkey)
{
  int rc = 0;
  CryptState *state = NULL;

  assert_not(name, NULL);

  // create the state we'll return
  state = calloc(1,sizeof(CryptState));
  assert_mem(state);

  state->me.name = bstrcpy(name);
  
  // setup the private key
  if(prvkey) {
    // use the key they give
    rc = CryptState_import_key(state, CRYPT_MY_KEY, prvkey);
    check(rc, "failed to import given key");
  } else {
    // generate a new key
    rc = CryptState_create_key(state);
    check(rc, "failed to create new key");
  }

  CryptState_rand_sess(state);

  ensure() {
    return state;
  }
}

void CryptState_reset(CryptState *state)
{
  assert_not(state, NULL);

  // destroy them
  CryptState_peer_destroy(state, CRYPT_THEIR_KEY);

  // zap any possible session key and nonce we have
  memset(&state->shared, 0, sizeof(symmetric_key));
  memset(&state->me.nonce, 0, sizeof(CryptNonce));
  memset(&state->me.skey, 0, sizeof(symmetric_key));

  // make a new session key (realloc if needed)
  CryptState_rand_sess(state);
}

int CryptState_rand(void *data, size_t len) {
  assert_not(data, NULL);
  return fortuna_read(data, len, &global_prng) == len;
}

int CryptState_create_nonce(CryptNonce *nonce)
{
  assert_not(nonce, NULL);
  return CryptState_rand(nonce->raw, NONCE_LENGTH);
}

inline int CryptState_create_key(CryptState *state)
{
  int rc = 0;
  assert_not(state, NULL);

  rc = ecc_make_key(&global_prng, global_prng_idx, KEY_LENGTH, &state->me.key);
  ltc_ok(rc, "making ECC key");

  return 1;
  on_fail(return 0);
}

bstring CryptState_ecc_key_bstr(ecc_key *key, int type)
{
  unsigned long out_len = ECC_BUF_SIZE;
  bstring key_data = bfromcstralloc(out_len, "");

  ltc_ok(ecc_export(bdata(key_data), &out_len, type, key), "Failed to export key to BLOB.");
  bsetsize(key_data, out_len);

  return key_data;
  on_fail(if(key_data) bdestroy(key_data); return NULL);
}

bstring CryptState_export_key(CryptState *state, crypt_key_side my_key, int type)
{
  assert_not(state, NULL);
  assert((my_key == CRYPT_MY_KEY || my_key == CRYPT_THEIR_KEY) && "my_key wrong");
  assert((type == PK_PUBLIC || type == PK_PRIVATE) && "type wrong");

  ecc_key *key = my_key ? &state->me.key : &state->them.key;
  assert(key && "Requested key not ready to export.");

  return CryptState_ecc_key_bstr(key, type);
}

int CryptState_bstr_ecc_key(ecc_key *key, bstring from)
{
  assert_not(from, NULL);

  ltc_ok(ecc_import(bdata(from), blength(from), key), "ECC key import failed");
  return 1; on_fail(return 0);
}

int CryptState_import_key(CryptState *state, crypt_key_side my_key, bstring from)
{
  assert_not(state, NULL);
  assert((my_key == CRYPT_MY_KEY || my_key == CRYPT_THEIR_KEY) && "my_key wrong");

  return CryptState_bstr_ecc_key(my_key ? &state->me.key : &state->them.key, from);
}


inline int CryptState_create_shared_key(CryptState *state, Node *header, int nonced, CryptState_key_confirm_cb key_confirm)
{
  int rc = 0;
  bstring ecc_key = NULL;
  unsigned char shared[ECC_BUF_SIZE];
  unsigned long shared_len = ECC_BUF_SIZE;

  assert_not(state, NULL);
  assert_not(header, NULL);

  // get the initiator's public key and load it into our state
  if(nonced) {
    // this header has a nonce in it
    rc = Node_decons(header, 0, "[bnnw", &ecc_key, 
        &state->them.nonce.num.left, &state->them.nonce.num.right, CRYPT_HEADER);
    check(rc, "invalid header format from receiver");
  } else {
    // no nonce included
    rc = Node_decons(header, 0, "[bw", &ecc_key, CRYPT_HEADER);
    check(rc, "invalid header format from intiaitor");
  }

  rc = CryptState_import_key(state, CRYPT_THEIR_KEY, ecc_key);
  check(rc, "failed to import initiator public key");

  rc = key_confirm(state);
  check(rc, "key not confirmed");

  // setup the shared key from them
  // now get our ecc_key to work with
  rc = ecc_shared_secret(&state->me.key, &state->them.key, shared, &shared_len);
  ltc_ok(rc, "failed to create shared secret");

  rc = rijndael_setup(shared, shared_len, 0, &state->shared);
  ltc_ok(rc, "failed to schedule AES shared key");

  return 1;
  on_fail(return 0);
}

// R->I: N,PubR
Node *CryptState_receiver_start(CryptState *state)
{
  assert_not(state, NULL);

  // generate an initial nonce that is known, but needed for ltc's ECC shared key encrypt
  // this gets recreated later for the nonce that is encrypted and sent to initiator
  check(CryptState_create_nonce(&state->me.nonce), "failed to generate random nonce");

  Node *msg = Node_cons("[nnbw",
      state->me.nonce.num.right, state->me.nonce.num.left,
      CryptState_export_key(state, CRYPT_MY_KEY, PK_PUBLIC), CRYPT_HEADER);
  check(msg, "failed to construct receiver start");

  return msg;
  on_fail(return NULL);
}


// R<-I: PubI, Er(I,Kir,Ni)
int CryptState_receiver_process_response(CryptState *state, bstring hbuf, Node *payload, CryptState_key_confirm_cb key_confirm)
{
  Node *ident = NULL, *header = NULL;
  int rc = 0;

  assert_not(state, NULL);
  assert_not(payload, NULL);
  assert_not(hbuf, NULL);

  // parse via stackish to make the header
  header = Node_parse(hbuf);
  check(header != NULL, "returned header failed to parse");

  rc = CryptState_create_shared_key(state, header, 0, key_confirm);
  Node_destroy(header);
  check(rc, "failed to create shared key");

  // decrypt the payload and process it accordingly
  ident = CryptState_decrypt_node(state, &state->shared, hbuf, payload);
  check(ident, "failed to parse decrypted payload");

  // read payload into the "them" structure so we can start working
  rc = Node_decons(ident, 1, "[nnbsw", 
      &state->them.nonce.num.left, &state->them.nonce.num.right,
      &state->them.session, &state->them.name, CRYPT_INIT_MSG);
  Node_destroy(ident);
  check(rc, "failed to decode payload of initial message");

  return 1;
  on_fail(return 0);
}

// R->I: Ei(R,Kri,Ni,Nr)
Node *CryptState_receiver_send_final(CryptState *state, bstring *hbuf)
{
  Node *msg = NULL, *payload = NULL;

  assert_not(state, NULL);
  assert_not(hbuf, NULL);

  // base empty header since it's always required
  *hbuf = bfromcstr("[ header \n");
  check(*hbuf, "failed to allocate header");

  // generate our random nonce to be used from now on
  check(CryptState_create_nonce(&state->me.nonce), "failed to generate random nonce");

  // create our response identifying us as the one who received the right stuff
  payload = Node_cons("[sbnnnnw", 
      bstrcpy(state->me.name),
      bstrcpy(state->me.session),
      state->them.nonce.num.right, state->them.nonce.num.left,
      state->me.nonce.num.right, state->me.nonce.num.left,
      CRYPT_RECV_MSG);
  check(payload, "failed to construct payload response");

  // encrypt it with the session key
  msg = CryptState_encrypt_node(state, &state->shared, *hbuf, payload);
  check(msg, "failed to encrypt payload for sending");

  ensure() {
    Node_destroy(payload);
    return msg;
  }
}

// R<-I: Er(Nr)
inline int CryptState_receiver_done(CryptState *state, bstring header, Node *msg)
{
  Node *payload = NULL;
  int rc = 0;
  CryptNonce expected;

  assert_not(state, NULL);
  assert_not(header, NULL);
  assert_not(msg, NULL);

  payload = CryptState_decrypt_node(state, &state->shared, header, msg);
  check(payload, "Failed to decrypt final node from initiator.");

  rc = Node_decons(payload, 0, "[nnw", &expected.num.left, &expected.num.right, CRYPT_INIT_MSG);
  check(rc, "failed deconstruct msg");
  CryptState_nonce_inc(&expected);  // increment once since it was used in one encrypt

  // now we just confirm the returned nonce matches our nonce and we're good
  check_then(CryptState_nonce_check(state->me.nonce, expected), 
      "Wrong nonce returned from initiator.", rc = 0);

  rc = CryptState_finalize_session(state);
  check(rc, "failed to finalize session keys");

  ensure() {
    if(payload) Node_destroy(payload); 
    return rc;
  }
}

// I<-R: N,PubR;  I->R: PubI, Er(I,Kir,Ni)
Node *CryptState_initiator_start(CryptState *state, Node *recvhdr, bstring *hbuf, CryptState_key_confirm_cb key_confirm)
{
  int rc = 1;
  Node *msg = NULL, *payload = NULL, *header = NULL;
  bstring ecc_key = NULL;

  assert_not(state, NULL);
  assert_not(recvhdr, NULL);
  assert_not(hbuf, NULL);

  rc = CryptState_create_shared_key(state, recvhdr, 1, key_confirm);
  check(rc, "failed to create shared key");

  // export our public key for the message we return
  ecc_key = CryptState_export_key(state, CRYPT_MY_KEY, PK_PUBLIC);
  check(ecc_key, "failed to export ecc key");

  // construct our header structure
  header = Node_cons("[bw", ecc_key, CRYPT_HEADER);
  *hbuf = Node_bstr(header, 1);

  // SECURITY: at this point our nonce is tranported encrypted, their's
  // is public from the R->I: N,PubR message
  check(CryptState_create_nonce(&state->me.nonce), "failed to create nonce"); 

  // build the payload to be encrypted
  payload = Node_cons("[sbnnw", 
      bstrcpy(state->me.name), bstrcpy(state->me.session), 
      state->me.nonce.num.right, state->me.nonce.num.left,
      CRYPT_INIT_MSG);
  check(payload, "failed to create payload message");

  // encrypt the whole thing with the shared key using the expected header
  msg = CryptState_encrypt_node(state, &state->shared, *hbuf, payload);
  check(msg, "creating encrypted packet failed");

  ensure() {
    // both not needed since the message has the encoded payload and header 
    // is encoded as an output parameter in hbuf
    Node_destroy(header);
    Node_destroy(payload);
    return msg;
  }
}

// I<-R: Ei(R,Kri,Ni,Nr)
int CryptState_initiator_process_response(CryptState *state, bstring header, Node *msg)
{
  Node *payload = NULL;
  CryptNonce me_nonce;
  int rc = 0;

  assert_not(state, NULL);
  assert_not(header, NULL);
  assert_not(msg, NULL);

  payload = CryptState_decrypt_node(state, &state->shared, header, msg);
  check(payload, "failed to parse decrypted packet");

  rc = Node_decons(payload, 1, "[nnnnbsw",
      &state->them.nonce.num.left, &state->them.nonce.num.right,
      &me_nonce.num.left, &me_nonce.num.right,
      &state->them.session, 
      &state->them.name, CRYPT_RECV_MSG);
  check(state->them.name, "receiver didn't give a name");
  check(rc, "failed to deconstruct message");

  // we increment what was returned since we've used the nonce once in an encrypt
  CryptState_nonce_inc(&me_nonce);

  check(CryptState_nonce_check(state->me.nonce, me_nonce), "Invalid nonce returned for me expected nonce.");

  ensure() {
    Node_destroy(payload);
    return rc;
  }
}


// I->R: Er(Nr)
Node *CryptState_initiator_send_final(CryptState *state, bstring *hbuf)
{
  int rc = 0;
  Node *msg = NULL;
  Node *payload = NULL;

  assert_not(state, NULL);
  assert_not(hbuf, NULL);
  
  *hbuf = bfromcstr("[ header \n");

  payload = Node_cons("[nnw",
      state->them.nonce.num.right, state->them.nonce.num.left, CRYPT_INIT_MSG);
  check(payload, "failed to construct receiver done reply node");

  msg = CryptState_encrypt_node(state, &state->shared, *hbuf, payload);
  check(msg, "failed to encrypt node");

  rc = CryptState_finalize_session(state);
  check(rc, "failed to finalize session keys");

  ensure() {
    Node_destroy(payload);
    return msg;
  }
}


inline int CryptState_finalize_session(CryptState *state)
{
  int rc = 0;

  assert_not(state, NULL);
  
  // schedule our key for later operations
  rc = rijndael_setup(bdata(state->me.session), blength(state->me.session), 0, &state->me.skey);
  ltc_ok(rc, "failed to schedule MY session key");


  // schedule their key for later operations
  rc = rijndael_setup(bdata(state->them.session), blength(state->them.session), 0, &state->them.skey);
  ltc_ok(rc, "failed to schedule MY session key");

  ensure() {
    memset(&state->shared, 0, sizeof(state->shared));
    // clear the session buffer
    bSecureDestroy(state->me.session); state->me.session = NULL;
    bSecureDestroy(state->them.session); state->them.session = NULL;
    return rc == CRYPT_OK;
  }
}


Node *CryptState_encrypt_packet(CryptState *state, symmetric_key *key, bstring header, bstring payload)
{
  int rc = 0;
  bstring tag = bfromcstralloc(ECC_BUF_SIZE, "");
  unsigned long taglen = ECC_BUF_SIZE;

  assert_not(state, NULL);
  assert_not(key, NULL);
  assert_not(header, NULL);
  assert_not(payload, NULL);

  rc = ccm_memory(global_cipher_idx, NULL, 0, key, 
    state->them.nonce.raw, NONCE_LENGTH,
    bdata(header), blength(header),
          bdata(payload),blength(payload),
          bdata(payload),
          bdata(tag), &taglen, CCM_ENCRYPT);
  ltc_ok(rc, "CCM encrypt failed");

  bsetsize(tag,taglen);
  CryptState_nonce_inc(&state->them.nonce);

  Node *msg = Node_cons("[bbw", payload, tag, CRYPT_ENV_MSG);

  return msg;
  // TODO: wipe the results from memory on failure
  on_fail(return NULL);
}

Node *CryptState_encrypt_node(CryptState *state, symmetric_key *key, bstring header, Node *payload)
{
  bstring pbuf = NULL;
  Node *msg = NULL;

  assert_not(state, NULL);
  assert_not(key, NULL);
  assert_not(header, NULL);
  assert_not(payload, NULL);

  pbuf = Node_bstr(payload, 1);
  check(pbuf, "failed to convert payload to bstring");

  msg = CryptState_encrypt_packet(state, key, header, pbuf);
  check(msg, "failed to encrypt payload for sending");

  return msg;
  on_fail(bSecureDestroy(pbuf); return NULL);
}


bstring CryptState_decrypt_packet(CryptState *state, symmetric_key *key, bstring header, Node *packet)
{
  int rc = 0;
  bstring tag = bfromcstralloc(ECC_BUF_SIZE, "");
  unsigned long taglen = ECC_BUF_SIZE;
  bstring payload = NULL, rectag = NULL;

  assert_not(state, NULL);
  assert_not(key, NULL);
  assert_not(header, NULL);
  assert_not(packet, NULL);

  rc = Node_decons(packet, 0, "[bbw", &rectag, &payload, CRYPT_ENV_MSG);
  check(rc, "failed to deconstruct msg");

  rc = ccm_memory(global_cipher_idx, NULL, 0, key,
    state->me.nonce.raw, NONCE_LENGTH,
    bdata(header), blength(header),
          bdata(payload),blength(payload),
          bdata(payload),
          bdata(tag), &taglen, CCM_DECRYPT);
  ltc_ok(rc, "CCM encrypt failed");

  bsetsize(tag,taglen);
  CryptState_nonce_inc(&state->me.nonce);

  // check the tags match
  check_then(biseq(tag, rectag), "given and expected tags are not equal", bSecureDestroy(payload); payload=NULL);

  ensure() {
    bdestroy(tag);
    return payload;
  }
}

Node *CryptState_decrypt_node(CryptState *state, symmetric_key *key, bstring header, Node *packet)
{
  bstring pbuf = NULL;
  Node *payload = NULL;

  assert_not(state, NULL);
  assert_not(key, NULL);
  assert_not(header, NULL);
  assert_not(packet, NULL);

  pbuf = CryptState_decrypt_packet(state, key, header, packet);
  check(pbuf && blength(pbuf) > 0, "failed to decrypt packet");

  payload = Node_parse(pbuf);
  check_then(payload, "failed to parse decrypted packet", fwrite(pbuf->data, blength(pbuf), 1, stderr));

  ensure(return payload);
}


void CryptState_print_key(CryptState *state, crypt_key_side my_key, int type)
{
  bstring key_fp = CryptState_fingerprint_key(state, my_key, type);
  check(key_fp, "failed to generate key fingerprint");

  if(type == PK_PUBLIC) {
    fprintf(stderr, "PUBLIC: ");
  } else {
    fprintf(stderr, "PRIVATE: ");
  }

  fprintf(stderr, "%s\n", bdata(key_fp));

  ensure() {
    bSecureDestroy(key_fp);
  }
}

bstring CryptState_fingerprint_key(CryptState *state, crypt_key_side my_key, int type)
{
  bstring fp = NULL;
  bstring outkey = NULL;

  assert_mem(state);
  assert((my_key == CRYPT_MY_KEY || my_key == CRYPT_THEIR_KEY) && "my_key wrong");
  assert((type == PK_PUBLIC || type == PK_PRIVATE) && "type wrong");

  outkey = CryptState_export_key(state, my_key, type);
  check(outkey, "failed to export key");

  fp = CryptState_fingerprint_bstr(outkey);

  return fp;
  on_fail(return NULL);
}

bstring CryptState_fingerprint_bstr(bstring str)
{
  int rc = 0;
  size_t i = 0;
  bstring fp = bfromcstralloc(MAXBLOCKSIZE*3, "");
  unsigned char hash[MAXBLOCKSIZE];
  unsigned long hash_len = MAXBLOCKSIZE;

  assert_mem(str);
  assert_mem(fp);

  // hash the memory into our temp space
  rc = hash_memory(global_hash_idx, bdata(str), blength(str), hash, &hash_len);
  ltc_ok(rc, "failed to hash key fingerprint");
  bdestroy(str);

  // convert to the fingerprint format
  for(i = 0; i < hash_len - 1; i++) {
    if(i % 2) {
      bformata(fp, "%02x-", hash[i]);
    } else {
      bformata(fp, "%02x", hash[i]);
    }
  }

  bformata(fp, "%02x", hash[i]);

  return fp;
  on_fail(bdestroy(fp); return NULL);
}


bstring CryptState_hash(const unsigned char *data, size_t length)
{
  bstring hash = bfromcstralloc(MAXBLOCKSIZE, "");
  int rc = 0;
  unsigned long hash_len = MAXBLOCKSIZE;

  assert_not(data, NULL);
  assert_mem(hash);

  rc = hash_memory(global_hash_idx, (const unsigned char *)data, length, bdata(hash), &hash_len);
  ltc_ok(rc, "Failed to hash requested data block.");
  bsetsize(hash, hash_len);

  return hash;
  on_fail(bdestroy(hash); return NULL);
}



Node *CryptState_sign_node(ecc_key *private_key, bstring name, Node *input)
{
  bstring hash = NULL, signature = NULL, payload = NULL, public_key = NULL;
  unsigned long out_len = MAXBLOCKSIZE;
  Node *result = NULL;
  int rc = 0;

  // convert the node to a bstring to hash
  payload = Node_bstr(input, ' ');
  check(payload, "Failed to convert Node to BLOB to sign.");

  // hash the node to get the resulting value to sign
  hash = CryptState_hash_bstr(payload);
  check(hash, "Failed to hash the payload to sign.");

  // sign the hash
  signature = bfromcstralloc(MAXBLOCKSIZE, "");
  rc = ecc_sign_hash(bdata(hash), blength(hash),
                        bdata(signature), &out_len,
                        &global_prng, global_prng_idx, private_key);
  ltc_ok(rc, "Failed to sign the payload's hash.");
  bsetsize(signature, out_len);

  // export the PUBLIC key used to sign
  public_key = CryptState_ecc_key_bstr(private_key, PK_PUBLIC);

  // encode into the standard signature node:
  // [ [ 'payload' 'hash' 'signature' message [ 'pubkey' 'name' identity signed
  result = Node_cons("[[bbbw[bbww", payload, hash, signature, "message",
      public_key, bstrcpy(name), "identity", "signed");
  check(result, "Failed to construct signature node response.");

  return result;
  on_fail(
      if(payload) bdestroy(payload);
      if(hash) bdestroy(hash);
      if(signature) bdestroy(signature);
      if(public_key) bdestroy(public_key);
      return NULL;
      );
}

Node *CryptState_verify_signature(ecc_key *public_key, bstring name, Node *input, int *sig_stat)
{
  bstring hash = NULL, signature = NULL, payload = NULL, pubkey = NULL;
  bstring sig_name = NULL;
  Node *result = NULL;
  int rc = 0;

  // deconstruct the standard signature node:
  // [ [ 'payload' 'hash' 'signature' message [ 'pubkey' 'name' identity signed
  rc = Node_decons(input, 0, "[[bbw[bbbww", &sig_name, &pubkey, "identity", &signature, &hash, &payload, "message", "signed");
  check(rc, "Signature node is not in proper format.");

  // validate given name and expected name match
  check(biseq(sig_name, name), "Name in signature message does not match expected name.");

  // import the public_key to validate against
  check(CryptState_bstr_ecc_key(public_key, pubkey), "Invalid public key given in signature message.");

  ### @export "resume"
  // verify the signature hash
  rc = ecc_verify_hash(bdata(signature), blength(signature),
                   bdata(hash), blength(hash),
                         sig_stat, public_key);
  ltc_ok(rc, "Signature validation process didn't function.");

  // conver the final structure back to it's Node form
  result = Node_parse(payload);
  check(result, "Failed to parse signed payload after all other verification was valid.");
  ### @end

  ensure(return result);
}

Node *CryptState_generate_hate_challenge(CryptState *state, unsigned int level, CryptNonce *expecting)
{
  bstring hash = NULL;
  Node *msg = NULL;
  unsigned int level_max = 1;

  assert_not(state, NULL);
  assert_not(expecting, NULL);

  // scale level up to the max possible with some bit shifts
  level_max <<= (level + 1);
  check(level_max > 0, "Maximum level ends up being 0.  Bad level given.");

  // calculate a random left and right to the given level
  check(CryptState_create_nonce(expecting), "Failed to create random nonce.");

  // adjust so the right value is within the requested level
  expecting->num.right = expecting->num.right % level_max;

  // hash the original number to give the challenge
  hash = CryptState_hash_nonce(expecting);

  // construct the challenge (0 is given as the missing piece)
  msg = Node_cons("[nnnbw", (uint64_t)level, 0, expecting->num.left, hash, CRYPT_CHALLENGE_MSG); 
  check(msg, "Failed constructing challenge message.");

  // sign it using the standard signature method
  Node *sign = CryptState_sign_node(&state->me.key, state->me.name, msg);
  check(sign, "Failed to sign resulting hate challenge message.");

  return sign;
  on_fail(if(msg) Node_destroy(msg); return NULL);
}

Node *CryptState_answer_hate_challenge(CryptState *state, Node *challenge, unsigned int max_allowed_level)
{
  bstring hash = NULL;
  unsigned char test_hash[MAXBLOCKSIZE];
  uint64_t level;
  CryptNonce val;
  Node *answer = NULL;
  int rc = 0;
  ecc_key given_key;

  assert_not(state, NULL); 
  assert_not(challenge, NULL);
  assert(max_allowed_level > 0 && "must be > 0");

  // validate the signed message and extract it
  challenge = CryptState_verify_signature(&given_key, state->them.name, challenge, &rc);
  check(rc, "Signature is not valid from challenger.");

  // TODO: Make sure the given key matches the state's key.

  // extract the half-nonce 
  rc = Node_decons(challenge, 1, "[bnnnw", &hash, &val.num.left, &val.num.right, &level, CRYPT_CHALLENGE_MSG);
  check(rc, "Failed deconstruct challenge node.");
  check(val.num.right == 0, "Invalid challenge (right side wasn't 0).");
  check(level <= max_allowed_level, "Challenge's level is greater than allowed.");
  check(hash, "Hash not set for comparison.");

  // scale level up to the max with a bit shift
  level <<= level + 1;

  // rotate through the right side until we get the right hash or stop at the max possible
  for(val.num.right = 0; val.num.right < level; val.num.right++) {
    unsigned long hash_len = MAXBLOCKSIZE;
    rc = hash_memory(global_hash_idx, val.raw, NONCE_LENGTH, test_hash, &hash_len);
    ltc_ok(rc, "failed to hash challenge nonce");
    if(hash_len == (unsigned long)blength(hash) && bisstemeqblk(hash, test_hash, hash_len)) {
      // build answer with left and right but SWAPPED to prove they got it
      answer = Node_cons("[nnw", val.num.left, val.num.right, CRYPT_ANSWER_MSG);
      break; // found it
    }
  }
  check(answer, "Hate answer not found, possibly bogus challenge.");

  // finally return the signed payload of the answer
  Node *sign = CryptState_sign_node(&state->me.key, state->me.name, answer);
  check(sign, "Failed to sign answer node to hate challenge.");

  return sign;
  on_fail(if(answer) Node_destroy(answer); return NULL);
}

int CryptState_validate_hate_challenge(CryptState *state, Node *answer, CryptNonce expecting)
{
  int rc = 0;
  CryptNonce ans_val;
  int sig_stat = 0;
  ecc_key given_key;

  assert_not(state, NULL);
  assert_not(answer, NULL);

  // validate the signature and take out the real message from challenge
  answer = CryptState_verify_signature(&given_key, state->them.name, answer, &sig_stat);
  check(sig_stat, "Signature is not valid for the given key.");
  // TODO: Make sure the given key is valid for the expected key.

  // extract answer values from the message
  rc = Node_decons(answer, 0, "[nnw", &ans_val.num.left, &ans_val.num.right, CRYPT_ANSWER_MSG);
  check(rc, "Failed to deconstruct answer after signature validated.");

  // confirm that answers are valid (but remember answer is inverted)
  check(expecting.num.left == ans_val.num.right && expecting.num.right == ans_val.num.left, "Expected answer does not match the answer value.");

  return 1; on_fail(return 0);
}
