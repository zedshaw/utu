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


#include "peer.h"


Peer *Peer_create(CryptState *state, int fd, CryptState_key_confirm_cb key_confirm)
{
  assert_not(state, NULL);
  assert(fd > -1 && "invalid fd");
  assert_not(key_confirm, NULL);

  pool_t *pool = pool_create();
  Peer *peer = pool_new(Peer, pool);

  peer->pool = pool;
  peer->state = state;
  peer->source.fd = fd;
  peer->source.out =  sbuf_create(pool, PEER_DEFAULT_IO_BUF_SIZE);
  assert_mem(peer->source.out);
  peer->source.in =  sbuf_create(pool, PEER_DEFAULT_IO_BUF_SIZE);
  assert_mem(peer->source.in);
  peer->key_confirm = key_confirm;

  return peer;
}

void Peer_destroy(Peer *peer, int crypt_too)
{
  if(peer) {
    if(crypt_too) CryptState_destroy(peer->state);
    pool_t *pool = peer->pool;
    pool_destroy(pool);
  }
}


int Peer_establish_receiver(Peer *peer)
{
  assert_not(peer, NULL);

  CryptState *state = peer->state;
  bstring header = bfromcstr("");
  int rc = 0;
  Node *rmsg = NULL, *imsg = NULL, *ihdr = NULL;

  // send our first message
  rmsg = CryptState_receiver_start(state);
  rc = FrameSource_send(peer->source, header, rmsg, 1);
  check(rc, "failed to send peer message");
  Node_destroy(rmsg); bdestroy(header); rmsg = NULL;

  // read the initiator response and parse it
  imsg = FrameSource_recv(peer->source, &header, &ihdr);
  check_then(imsg, "failed to read sent message", rc = 0);

  rc = CryptState_receiver_process_response(state, header, imsg, peer->key_confirm); 
  check(rc, "failed to process initiator's identity");
  Node_destroy(imsg); imsg = NULL;
  Node_destroy(ihdr); ihdr = NULL;
  bdestroy(header);

  rmsg = CryptState_receiver_send_final(state, &header); 
  check(rmsg, "failed to build receiver final message");

  rc = FrameSource_send(peer->source, header, rmsg, 1);
  check(rc, "failed to send final message");
  Node_destroy(rmsg); rmsg = NULL;
  bdestroy(header);

  imsg = FrameSource_recv(peer->source, &header, &ihdr);
  check(imsg, "failed to read initiator's final");

  rc = CryptState_receiver_done(state, header, imsg);
  check(rc, "failed to process final message");

  ensure(bdestroy(header); 
      if(rmsg) Node_destroy(rmsg);
      if(imsg) Node_destroy(imsg);
      if(ihdr) Node_destroy(ihdr);
      return rc);
}



int Peer_establish_initiator(Peer *peer)
{
  assert_not(peer, NULL);

  CryptState *state = peer->state;
  int rc = 0;
  bstring header = NULL;
  Node *imsg = NULL;
  Node *rmsg = NULL, *rhdr = NULL;

  // read the receiver's initial message
  rmsg = FrameSource_recv(peer->source, &header, &rhdr);
  check(rmsg == NULL && rhdr, "invalid first message from receiver");
  bdestroy(header); header = NULL;

  imsg = CryptState_initiator_start(state, rhdr, &header, peer->key_confirm);
  check(imsg, "failed to make the initiator msg");
  Node_destroy(rhdr); rhdr = NULL;

  rc = FrameSource_send(peer->source, header, imsg, 1);
  check(rc, "failed to send message");
  Node_destroy(imsg); bdestroy(header); imsg = NULL;

  rmsg = FrameSource_recv(peer->source, &header, &rhdr);
  check(rmsg, "failed to read receiver's ident");

  rc = CryptState_initiator_process_response(state, header, rmsg);
  check(rc, "failed to process receiver response");
  Node_destroy(rmsg); bdestroy(header); rmsg = NULL;

  imsg = CryptState_initiator_send_final(state, &header);
  check(imsg, "failed to generate final message");

  rc = FrameSource_send(peer->source, header, imsg, 1);
  check(rc, "failed to send final message to receiver");

  ensure(bdestroy(header); 
      if(rmsg) Node_destroy(rmsg);
      if(rhdr) Node_destroy(rhdr);
      if(imsg) Node_destroy(imsg);
      return rc);
}


Node *Peer_recv(Peer *peer, Node **rhdr)
{
  assert_not(peer, NULL);
  assert_not(rhdr, NULL);

  CryptState *state = peer->state;
  Node *packet = NULL;
  Node *msg = NULL;
  bstring header = NULL;

  packet = FrameSource_recv(peer->source, &header, rhdr);
  if(!packet) return NULL; // socket probably closed

  msg = CryptState_decrypt_node(state, &state->me.skey, header, packet);
  check(msg, "failed to decrypt message");

  ensure(Node_destroy(packet); 
      bdestroy(header);
      return(msg));
}


int Peer_send(Peer *peer, Node *header, Node *payload)
{
  assert_not(peer, NULL);
  assert_not(header, NULL);
  assert_not(payload, NULL);

  CryptState *state = peer->state;
  Node *msg = NULL;
  int rc = 0;
  bstring hbuf = Node_bstr(header, 1);
  check(hbuf, "failed to convert header to bstring");

  msg = CryptState_encrypt_node(state, &state->them.skey, hbuf, payload);
  check_then(msg, "failed to encrypt payload", rc = 0);

  rc = FrameSource_send(peer->source, hbuf, msg, 1);
  Node_destroy(msg); bdestroy(hbuf);
  check(rc, "failed to send");

  ensure(return rc);
}


