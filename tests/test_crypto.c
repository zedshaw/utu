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


/**
 * A sample test layout.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cut.h"
#include "protocol/crypto.h"

int peer1_new_key = 1;
int peer2_new_key = 1;
CryptState *peer1;
CryptState *peer2;

int key_confirmed = 0;

int dumb_key_confirm(CryptState *state) {
  key_confirmed = 1;
  return 1;
}

void __CUT_BRINGUP__CryptoTest( void ) {
  // initialize the crypto system
  int rc = CryptState_init();
  ASSERT(rc, "failed init crypt");
}


void __CUT__Crypto_create_keys() {
  bstring p1name = bfromcstr("peer1");
  bstring p2name = bfromcstr("peer2");
  // create new keys and print them out
  peer1 = CryptState_create(p1name, NULL);
  ASSERT(peer1 != NULL, "failed create my key");
  peer2 = CryptState_create(p2name, NULL);
  ASSERT(peer2 != NULL, "failed create their key");

  CryptState_destroy(peer1);
  CryptState_destroy(peer2);
  bdestroy(p1name);
  bdestroy(p2name);
}


void __CUT__Crypto_enCryptState_decrypt() {
  // create new keys and print them out
  bstring p1name = bfromcstr("peer1");
  peer1 = CryptState_create(p1name, NULL);
  ASSERT(peer1 != NULL, "failed create my key");
  peer1->them.nonce = peer1->me.nonce;

  bstring header = bfromcstr("test header");
  bstring payload = bfromcstr("test payload");
  bstring shared = bfromcstr("00000000000000000000000000000000");
  symmetric_key key;
  aes_setup(bdata(shared), blength(shared), 0, &key);

  Node *msg = CryptState_encrypt_packet(peer1,&key,header,payload);
  ASSERT(msg != NULL, "failed to encrypt");
  
  bstring pld2 = CryptState_decrypt_packet(peer1,&key,header, msg);
  ASSERT(biseq(payload, pld2), "decryption failed");

  CryptState_destroy(peer1);
  Node_destroy(msg);
  bdestroy(header);
  bdestroy(shared);
  bdestroy(p1name);
}


void __CUT__Crypto_integration()
{
  bstring p1name = bfromcstr("peer1");
  bstring p2name = bfromcstr("peer2");
  bstring header = NULL;
  int rc = 0;

  if(peer1_new_key) peer1 = CryptState_create(p1name, NULL);
  ASSERT(peer1 != NULL, "failed create my key");

  if(peer2_new_key) peer2 = CryptState_create(p2name, NULL);
  ASSERT(peer2 != NULL, "failed create their key");

  // R->I: N,PubR
  Node *recvr = CryptState_receiver_start(peer1);
  ASSERT(recvr != NULL, "receiver didn't create initial header");

  // I<-R: N,PubR;  I->R: PubI, Er(I,Kir,Ni)
  Node *init_payload = CryptState_initiator_start(peer2, recvr, &header, dumb_key_confirm);
  ASSERT(init_payload != NULL, "initiator start failure");
  ASSERT(key_confirmed == 1, "key wasn't confirmed");
  key_confirmed = 0;

  // R<-I: PubI, Er(I,Kir,Ni)
  rc = CryptState_receiver_process_response(peer1, header, init_payload, dumb_key_confirm); 
  ASSERT(rc, "receiver failed to process ident response");
  ASSERT(key_confirmed == 1, "key wasn't confirmed");
  key_confirmed = 0;

  // R->I: Ei(R,Kri,Ni,Nr)
  bdestroy(header); 
  Node *recv_ident = CryptState_receiver_send_final(peer1, &header); 
  ASSERT(recv_ident != NULL, "failed to construct recv final message");

    
  // I<-R: Ei(R,Kri,Ni,Nr)
  rc = CryptState_initiator_process_response(peer2, header, recv_ident);
  ASSERT(rc, "initiator failed to process final response");

  
  // I->R: Er(Nr)
  bdestroy(header);
  Node *init_final = CryptState_initiator_send_final(peer2, &header);
  ASSERT(init_final != NULL, "failed to create initiator final");

  // R<-I: Er(Nr)
  rc = CryptState_receiver_done(peer1, header, init_final);
  assert(rc && "failed to process final message from initiator");

  // R sends hello to I
  bdestroy(header);
  header = bfromcstr("[header ");
  bstring hello = bfromcstr("this is a hello from R");

  Node *r_hello = CryptState_encrypt_packet(peer1, &peer1->them.skey, header, hello);
  // I decrypts hello from R
  bstring hello_test = CryptState_decrypt_packet(peer2, &peer2->me.skey, header, r_hello);
  ASSERT(biseq(hello,hello_test), "R's hello message didn't decrypt");
  
  // I sends hello to R
  Node_destroy(r_hello);
  hello = bfromcstr("this is a hello from I");
  Node *i_hello = CryptState_encrypt_packet(peer2, &peer2->them.skey, header, hello);
  // R decrypts hello from I
  hello_test = CryptState_decrypt_packet(peer1, &peer1->me.skey, header, i_hello);
  ASSERT(biseq(hello,hello_test), "I's hello message didn't decrypt");

  bdestroy(header);
  Node_destroy(i_hello);
  Node_destroy(recvr);
  Node_destroy(init_payload);
  Node_destroy(recv_ident);
  Node_destroy(init_final);
  if(peer1_new_key) CryptState_destroy(peer1);
  if(peer2_new_key) CryptState_destroy(peer2);
  bdestroy(p1name); bdestroy(p2name);
}


void __CUT__Crypto_hate_challenge()
{
  bstring p1name = bfromcstr("peer1");
  bstring p2name = bfromcstr("peer2");
  peer1_new_key = peer2_new_key = 0;
  peer1 = CryptState_create(p1name, NULL);
  peer2 = CryptState_create(p2name, NULL);
  Node *challenge, *answer;
  CryptNonce expecting;
  time_t start, end;
  int level = 0;

  __CUT__Crypto_integration();

  // do a series of challenge responses of different levels and then report their timings
  for(level = 5; level < 10; level++) {
    start = time(NULL);
    challenge = CryptState_generate_hate_challenge(peer1, level, &expecting);
    end = time(NULL) - start;
    dbg("GENERATE challenge at level %d: %d", level, (int)end);
    ASSERT(challenge != NULL, "failed to make challenge");

    start = time(NULL);
    answer = CryptState_answer_hate_challenge(peer2, challenge, level);
    end = time(NULL) - start;
    dbg("GENERATE answer at level %d: %d", level, (int)end);
    ASSERT(answer != NULL, "failed to make answer");

    if(!answer) return;

    start = time(NULL);
    ASSERT(CryptState_validate_hate_challenge(peer1, answer, expecting), "challenge/response failed");
    end = time(NULL) - start;
    dbg("VALIDATE answer at level %d: %d", level, (int)end);

    Node_destroy(challenge);
    Node_destroy(answer);

  }


  CryptState_destroy(peer1);
  CryptState_destroy(peer2);
  bdestroy(p1name); bdestroy(p2name);

  peer1_new_key = peer2_new_key = 1;

}

void __CUT__Crypto_speed_test()
{
  int i = 0;
  int count = 10;
  bstring p1name = bfromcstr("peer1");
  bstring p2name = bfromcstr("peer2");

  peer1 = CryptState_create(p1name, NULL);
  ASSERT(peer1 != NULL, "failed create my key");

  peer2 = CryptState_create(p2name, NULL);
  ASSERT(peer2 != NULL, "failed create their key");

  peer1_new_key = 0;
  peer2_new_key = 0;

  time_t start = time(NULL);

  for(i = 0; i < count; i++) {
    __CUT__Crypto_integration();

    if(i < count - 1) {
      CryptState_reset(peer1);
      CryptState_reset(peer2);
    }
  }

  time_t end = time(NULL) - start;

  dbg("TIME to complete %d rounds: %d seconds", count, (int)end);

  peer1_new_key = 1;
  peer2_new_key = 1;
  CryptState_destroy(peer1);
  CryptState_destroy(peer2);
  bdestroy(p1name); bdestroy(p2name);
}


void __CUT_TAKEDOWN__CryptoTest( void ) {
}
