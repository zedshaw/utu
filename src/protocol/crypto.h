#ifndef utu_crypto_h
#define utu_crypto_h

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


/** This implements ISO/IEC 11770-3 Mechanism 6 (but NOT the vulnerable
 * Helsinki version) for our key exchange and peer authentication.  This
 * mechanism is characteristic in that it uses asymetric crypto only as the
 * means of doing mutual authentication and mutual key agreement.  One thing
 * that isn't clear in the descriptions is whether the final Nr nonce returned
 * by I should be sent unencrypted, encrypted with Er, or encrypted with the
 * agreed key(s).  I've decided to send it back as Er(Nr) but I'm not sure of
 * the security of this.  In the sequence of operations Nr stays encrypted
 * throughout and this keeps it from being seen.
 *
 * R: N,PubR 
 * I: PubI,Er(I,Kir,Ni) 
 * R: Ei(R,Kri,Ni,Nr) 
 * I: Er(Nr)
 *
 * The notation is R for "receiver" and I for "initiator" rather than A and B.
 * This is particularly important as, if we reverse the roles then the
 * initiator is able to force the receiver to both do the encryption work *and*
 * waste precious randomness.  With the roles situated this way the initiator
 * is forced to perform encryption first and waste randomness making it
 * slightly more difficult to cause the receiver to do wasted work.
 *
 * An additional benefit is that if the client doesn't behave well--like
 * closing the connection right before sending the last message--then the
 * server can officially ban them since they had to create a valid message with
 * their private key.  The only time when someone could cause another client to
 * be blocked in this case is if they can get that person's private key and
 * forge messages, which means the key is compromised and therefore should be
 * blocked anyway.
 *
 * After the final Nr nonce is sent back and verified both parties use the
 * given random keys as their send/recv keys.  This is done rather than hashing
 * the two for a common key in order to avoid requiring a hashing algorithm to
 * be required by the library.
 *
 * All of these functions return 1 for true, 0 for false, an ssize_t, or a
 * value.  With ssize_t it's -1 for errors or the size.  For values it's the
 * value or NULL for an error.
 *
 * Some functions set errno to various values as documented, but most of the
 * time they just return the error status and you should abort.  The functions
 * that do encryption will clear their data buffers when there's an error,
 * especially if decryption fails.
 *
 * WARNING:  The first message R sends includes a temporary randomly generated
 * nonce that is publicly viewable.  This is needed so that the LTC shared
 * ECC key won't be vulnerable for that first encryption.  This nonce is only
 * used for that one first message and is discarded for the encrypted nonce 
 * after that.
 *
 * Additionally, the nonces are 16 bytes but represented as two uint64_t
 * integers (which is 16 bytes on 32 bit machines, so really not portable).  The
 * two nonces that are exchanged encrypted are simply incremented for each message
 * and are never retransmitted.
 *
 * Finally, the public keys are obviously being exchanged in public, it's up to
 * the person using the library to present the user with a key fingerprint they
 * can use to make sure that they are connecting to the right receiver, and you
 * should store known keys to detect later tampering.
 */


#include <myriad/defend.h>
#include <myriad/bstring/bstrlib.h>
#include <tomcrypt.h>
#include "stackish/stackish.h"

/** Hate challenge node root. */
#define CRYPT_CHALLENGE_MSG "challenge"
#define CRYPT_ANSWER_MSG "answer"
#define CRYPT_INIT_MSG "init"
#define CRYPT_RECV_MSG "recv"
#define CRYPT_HEADER "header"
#define CRYPT_ENV_MSG "env"

/** Determines whether to work on my key or their key side
 * of any peer operation.
 */
typedef enum {CRYPT_THEIR_KEY=0, CRYPT_MY_KEY=1 } crypt_key_side;

/** Length of the nonce in bytes. */
#define NONCE_LENGTH (sizeof(uint64_t) * 2)
/** Default key size for ECC and AES in bytes. */
#define KEY_LENGTH  32
/** PRNG to use (after initializing by SPRNG). */
#define PRNG_NAME "fortuna"
/** HASH to use (sha256) */
#define HASH_NAME "sha256"
/** Cipher being used. */
#define CIPHER_NAME "rijndael"

/** A union that allows simply conversion from two
 * uint64_t integers and an array of unsigned
 * bytes.  This makes processing and then incrementing the
 * nonce much easier.
 */
typedef union CryptNonce {
  unsigned char raw[NONCE_LENGTH];
  struct {
    uint64_t left;
    uint64_t right;
  } num;
} CryptNonce;

/** A peer in the CryptState communication session. */
typedef struct CryptPeer {
  bstring name;
  CryptNonce nonce;
  ecc_key key;
  bstring session;
  symmetric_key skey;
} CryptPeer;

/** The state of the current communication channel with a remote
 * peer.
 */
typedef struct CryptState {
  CryptPeer me;
  CryptPeer them;
  void *data;

  symmetric_key shared;
} CryptState;


/** This callback is used to confirm the public key received of
 * a peer.  It's called right when the key is received but
 * before any additional encryption work is done.  For the 
 * receiver this is before *any* work is done, for the 
 * intiator it's after one ECC shared key decrypt and full CCM
 * round (initiator is punished on purpose).
 *
 * Your function should check the key with the user or in a database
 * of known keys and then return 1 for good and 0 for not.  If
 * 0 is returned then the encryption process is failed right then.
 */

typedef int (*CryptState_key_confirm_cb)(CryptState *state);


/** Simple macro used to "clamp" size of a bstring after it's been filled. You need S+1
 * in length so that it can add the required '\0' at the end. */
#define bsetsize(B,S) (B)->slen = (S), (B)->data[(S)] = (unsigned char) '\0'
/** Checks the result of an LTC operation. */
#define ltc_ok(R,M) check_then((R) == CRYPT_OK,"LTC FAIL: " # M, log(ERROR, "LTC %s\n", error_to_string(R)))

/** Called once for the whole application, this initializes all the various
 * LTC infrastructure needed to get things going.
 *
 */
int CryptState_init();

/** Takes a CryptNonce and increments it.  The increment is done
 * by using the num.left and num.right elements of the union and
 * doing a simple ++.
 */
#define CryptState_nonce_inc(N) {(N)->num.left++; (N)->num.right++;}

/** Sets the nonce to whatever is in the B bstring. */
#define CryptState_nonce_set(N,B) memcpy((N)->raw, (B)->data, NONCE_LENGTH)

/** dbg prints the two integers in the nonce so you can compare. */
#define CryptState_nonce_dump(M, N) dbg("%s: %llu.%llu", (M), (N)->num.left, (N)->num.right)

/** Compares two nonces numerically by the left and right uint64_t. */
#define CryptState_nonce_check(N1,N2) ((N1).num.left == (N2).num.left && (N1).num.right == (N2).num.right)

/** Generates randomness from the internal random number generator (fortuna). */
int CryptState_rand(void *data, size_t len);

/** Fills the given nonce with randomness so you can use it. */
int CryptState_create_nonce(CryptNonce *nonce);

/** Used internally to destroy a peer cleanly and safely. */
void CryptState_peer_destroy(CryptState *state, crypt_key_side my_key);

/** Destroys a state cleanly and securely as possible so that it doesn't 
 * use memory anymore.
 */
void CryptState_destroy(CryptState *state);

/**
 * Creates a new CryptState that you can use with the remaining
 * functions to establish a secure connection and transmit/receive
 * encrypted packets.
 *
 * If prvkey is NULL then a new key will be generated for you.  You
 * can then use crypt_export_key to save the key for later.
 */
CryptState *CryptState_create(bstring name, bstring prvkey);

/**
 * A faster way to reuse a state for another communication.  Rather than
 * continually calling CryptState_init to recreate your side, this
 * function just clears the CryptState to where it is exactly like
 * you just called CryptState_init.
 */
void CryptState_reset(CryptState *state);

/**
 * Exports the raw data of the internal ECC key to a bstring for storage
 * or transmission.  my_key indicates whether to export your key or their key,
 * and type is LTC setting of PK_PUBLIC or PK_PRIVATE to indicate the type
 * of key.  If you specify PK_PRIVATE that already assumes a PK_PUBLIC export
 * as well (meaning you don't have to call this again to save the public key
 * as well).
 */
bstring CryptState_export_key(CryptState *state, crypt_key_side my_key, int type);

/**
 * Takes a bstring from crypt_export_key and loads it into the appropriate
 * key for my_key side.  You don't need to specify whether the key is public
 * or private as the encoding says this.
 */
int CryptState_import_key(CryptState *state, crypt_key_side my_key, bstring from);

/** 
 * @function CryptState_bstr_ecc_key
 * @brief Raw bstring to ECC key convert.
 * @param key : ECC key struct.
 * @param from : bstring to extract from
 * @return int : Whether it worked or not.
 */
int CryptState_bstr_ecc_key(ecc_key *key, bstring from);

/** 
 * @function CryptState_ecc_key_bstr
 * @brief Raw ECC key to bstring convert.
 * @param key : ECC key struct.
 * @param type : PK_PUBLIC or PK_PRIVATE.
 * @return bstring : Resulting key or NULL on fail.
 */
bstring CryptState_ecc_key_bstr(ecc_key *key, int type);

/**
 * The first function call in the establish sequence, this returns a message
 * to be sent to the initiator so they can call CryptState_initiator_start()
 * to begin their processing.
 *
 */
Node *CryptState_receiver_start(CryptState *state);

/**
 * Takes the response from CryptState_initiator_start()
 * and processes it to extract the initiator's information and verify it.
 * After this processing you would want to validate the initiator's name,
 * key, etc. to confirm they are allowed to even continue.
 *
 * Once this works, you then call CryptState_receiver_send_final() to return
 * the message that initiator needs to confirm the receiver's identity.
 *
 * hbuf is the full header you received as-is.
 */
int CryptState_receiver_process_response(CryptState *state, bstring hbuf, Node *payload, CryptState_key_confirm_cb key_confirm);

/**
 * Generates the final message sent back to the initiator to
 * prove the receiver's identity.  The initiator uses
 * CryptState_initiator_process_response() to extract the receiver's information
 * and verify it's validity.
 *
 * hbuf is an out parameter that contains the header to send to the
 * initiator exactly as-is.
 */
Node *CryptState_receiver_send_final(CryptState *state, bstring *hbuf);

/**
 * After the initiator validates the receiver's final message, they
 * return one more message with crypt_initiator_send_final to confirm
 * that they actually are the recipients of the receiver's nonce and 
 * key.  The receiver processes this message with this function.
 *
 * Once all the nonces and keys are confirmed to match, the shared key
 * is cleared out and the session keys for both sides are locked down
 * for the remaining messages.  The shared key is cleared to reduce
 * the chance that it can be read out of memory.
 */
int CryptState_receiver_done(CryptState *state, bstring header, Node *msg);

/**
 * The first call the initiator to create their first response to the
 * receiver proving their identity.  It puts a header in *hbuf as an
 * output parameter, and takes the Node returned from CryptState_receiver_start()
 * as it's input.  The Node that gets returned should be sent after the
 * header so the receiver can process it to extract the encrypted payload
 * and wire up the initiator's peer.
 *
 * The receiver will then call CryptState_receiver_send_final() to send it's
 * reply to the initiator proving the receiver's identity.
 */
Node *CryptState_initiator_start(CryptState *state, Node *recv_start, bstring *hbuf, CryptState_key_confirm_cb key_confirm);


/**
 * The initiator calls this to process the receiver's final message 
 * sent from CryptState_receiver_send_final() and validate it.  The caller
 * should also confirm the receiver identity and key after this 
 * possibly prompting the user.
 */
int CryptState_initiator_process_response(CryptState *state, bstring header, Node *msg);

/**
 * Finally, the initiator sends a small encrypted message with the nonce that
 * the receiver sent in order to prove that they actually received the message
 * and are actually the claimed initiator.  The receiver processes this
 * with CryptState_receiver_done().
 *
 * Just like CryptState_receiver_done() this method clears the shared key and
 * finalizes the two session keys for further encrypted communication.
 */
Node *CryptState_initiator_send_final(CryptState *state, bstring *hbuf);


/**
 * Used to encrypt raw bstring payload with an AAD (Additional Authenticated Data)
 * header to ensure message integrity.  It properly increments the random nonce
 * for each message and returns a Node that wraps the payload in a envelope.
 *
 * The payload is actually encrypted in place and attached to the returned Node,
 * so you don't bdestroy(payload) but instead just Node_destroy() the return
 * value.
 *
 * The header is not included in the returned message.
 */
Node *CryptState_encrypt_packet(CryptState *state, symmetric_key *key, bstring header, bstring payload);

/**
 * The inverse of CryptState_encrypt_packet() taking the Node and decrypting it (again, in
 * place) to extract the original bstring payload.  The returned payload is not copied
 * so you just Node_destroy(packet) to free it.
 */
bstring CryptState_decrypt_packet(CryptState *state, symmetric_key *key, bstring header, Node *packet);

/**
 * Convenience function that does the job of translating an arbitrary Node into 
 * an encrypted one.  It converts the payload to a bstring with Node_bstr() and
 * then encrypts.  The header is left alone.
 */
Node *CryptState_encrypt_node(CryptState *state, symmetric_key *key, bstring header, Node *payload);

/**
 * Again the inverse of CryptState_encrypt_node() this takes the encrypted packet and then
 * decrypts it and converts it directly to a Node using Node_parse().
 */
Node *CryptState_decrypt_node(CryptState *state, symmetric_key *key, bstring header, Node *packet);

/**
 * Prints a key in base64 so you can check it out.
 */
void CryptState_print_key(CryptState *state, crypt_key_side my_key, int type);

/**
 * Returns a fingerprint (sha256 hash of key) for you to show a user.  It is
 * pre-formatted to be in hex with dashes between each two hash bytes:
 *
 * bf27-3806-2dc6-19a0-894d-a3ff-875b-9685-4fba-a2b2-0635-cb94-3494-e1e2-b04b-1f45
 *
 * As an example.  It is kind of slow so don't do billions of these, but not so
 * slow that you need to avoid it.
 */
bstring CryptState_fingerprint_key(CryptState *state, crypt_key_side my_key, int type);

/**
 * Returns the same fingerprint as CryptState_fingerprint_key except it works on 
 * any bstring of any length.  This function is used by CryptState_fingerprint_key
 * to do the actual fingerprinting.
 */
bstring CryptState_fingerprint_bstr(bstring key);


/**
 * Helper macro to hash a nonce easily.
 */
#define CryptState_hash_nonce(N) CryptState_hash((N)->raw, NONCE_LENGTH)

/**
 * Helper macro to hash a bstring easily.
 */
#define CryptState_hash_bstr(B) CryptState_hash(bdata((B)), blength((B)))

/** 
 * @function CryptState_hash
 * @brief Hashes some data returning a bstring of the sha256 hash.
 * @param data : Block of data to hash.
 * @param length : Length of data to hash.
 * @return bstring : The resulting hash.
 */
bstring CryptState_hash(const unsigned char *data, size_t length);


/** 
 * Used when a peer needs to sign a message to prove that they actually
 * sent the given Node payload.  Normally this isn't necessary when
 * communicating with the Hub since it has established a secure session
 * key based on the peer's ECC key.  This function is more used to sign
 * a message for *another* peer on the Hub to prove you sent it, especially
 * when leaving messages for later pick-up or to provde identity.
 *
 * The message returned wraps the original in a BLOB and adds all the 
 * data needed by the target peer(s) to validate you did it:
 *
 *   [ 
 *     [ 'payload' 'hash' 'signature' message 
 *     [ 'pubkey' 'name' identity 
 *   signed
 *
 * The receiver has to know the expected name/public_key of the 
 * potential signer in order to validate.  Make sure they can either
 * get this information or that you have another way of doing a 
 * visual confirmation.
 *
 * @function CryptState_sign_node
 * @brief Signs the input node placing it in a signature stackish structure.
 * @param private_key : The ecc_key of the private signature side.
 * @param input : The stackish node to sign.
 * @return Node *: The new signature structure or NULL on failure.
 * @see CryptState_verify_signature
 */
Node *CryptState_sign_node(ecc_key *private_key, bstring name, Node *input);

/** 
 * Takes a Node generated from CryptState_sign_node and peels off the
 * signature layer, validates that the public key inside the message
 * did the signing, and that the identity matches what the called expects.
 *
 * The public_key parameter to this function is an OUT parameter, meaning
 * it will overwrite anything you have in there already.  This is usually
 * what you want since you may want to then use that key for later signing
 * and other operations.
 *
 * What this means is that you *must* validate that the key you get out
 * of public_key matches what you expect from the person signing it.
 * Usual usage is as follows:
 *
 * 1. Get the signed message and load it into a Node.
 * 2. Get the expected identity and key data from some other source (usually the Hub).
 * 3. Call this function with a ecc_key structure to have it filled in.
 * 4. This function will make sure the name matches, you then just have to make sure the resulting ecc_key matches.
 * 5. Do a CryptState_export_key to do this.
 *
 * The last thing to note is that everything can work, but the signature
 * could be invalid.  Rather than discard everything you give the OUT
 * parameter sig_stat.  You'll get the resulting data and the given
 * public_key and name to work with, but if sig_stat is false then
 * the signature was invalid so don't trust the message.  This is done
 * so you can block the sender, work with the resulting data, examine
 * it, etc.
 *
 * @function CryptState_verify_signature
 * @brief Breaks out a signature structure verifying the signature against the public_key.
 * @param public_key : Public key of the signer.
 * @param input : The signature node structure to break up.
 * @param sig_stat : OUT parameter that tells you if the signature was valid.
 * @return Node *: The original Node or NULL if failure.
 */
Node *CryptState_verify_signature(ecc_key *public_key, bstring expected_name, Node *input, int *sig_stat);

/**
 * Constructs a signed hate challenge that the receiver must process.
 * The algorithm is to construct two uint64_t integers and 
 * hash them.  The receiver is then only given one of the numbers and
 * has to calculate:
 *
 * sha256(left:right)
 *
 * By guessing the right number.  This conveniently is the same as a
 * nonce used in the system, so they just have to calculate the sha256
 * of a nonce that has the right side set to X.  They know they're done
 * when they have the same hash.
 *
 * The challenge is signed so that the receiver can validate the challenge 
 * actually came from who they claimed.
 *
 * The level parameter indicates how large of a value as in 2^level maximum
 * for the right side.  Larger values mean more work for the receiver.
 *
 * @function CryptState_generate_hate_challenge
 * @brief Generate a signed hate challenge.
 * @param state : Current crypto state to use for the remote peer.
 * @param level : 2^level max.
 * @param expecting : OUT variable indicating what the peer should come back with.
 * @return Node *: The full message to send the peer.
 */
Node *CryptState_generate_hate_challenge(CryptState *state, unsigned int level, CryptNonce *expecting);

/** 
 * Answers a hate challenged by attempting to calculate the hate they
 * sent after validating signatures.  The max_allowed_level is used
 * to make sure that you don't try to do more work than you are willing
 * to attempt.
 *
 * When answering the peer takes the CryptNonce and reverse the left/right
 * sides to differentiate the answer a bit.
 *
 * @function CryptState_answer_hate_challenge
 * @brief Answer a signed hate challenge.
 * @param state : Crypto state to process as the remote peer.
 * @param challenge : The challenge the remote peer signed for you.
 * @param max_allowed_level : Highest work as in 2^level you're willing to do.
 * @return Node *: A signed answer from you to give back as proof you did the work.
 */
Node *CryptState_answer_hate_challenge(CryptState *state, Node *challenge, unsigned int max_allowed_level);

/** 
 * Takes the signed answer to a previous hate challenge you sent and makes
 * sure that this is the peer who did the work *and* that they got the
 * right expected answer.  Remember the answer is inverted so the check
 * is actually right==left and left==right.
 *
 * @function CryptState_validate_hate_challenge
 * @brief Validates the signed answer is what's expected.
 * @param state : Crypt of the peer to check against.
 * @param answer : Their answer node structure.
 * @param expecting : A left/right you are expecting.
 * @return int : Whether it was valid or not.
 */
int CryptState_validate_hate_challenge(CryptState *state, Node *answer, CryptNonce expecting);

#endif

