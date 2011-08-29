#include "hub.h"

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

void ConnectionState_destroy(ConnectionState *state)
{
  free(state);
}

int ConnectionState_dequeue_msg(ConnectionState *state)
{
  if(ConnectionState_done(state)) {
    return 0;
  }

  state->send.msg = Member_first_msg(state->member);

  return state->send.msg != NULL;
}

int ConnectionState_send_msg(ConnectionState *state)
{
  int rc = 0;
  Message *msg = state->send.msg;

  if(ConnectionState_done(state)) {
    rc = 0;
  } else {
    // make a new header with the msgid the connected client expects
    Node *hdr = Message_cons_header(state->send_count++);
    rc = Peer_send(state->member->peer, hdr, msg->body);
    Node_destroy(hdr);
  }

  return rc;
}

void ConnectionState_outgoing(void *data)
{
  ConnectionState *state = (ConnectionState *)data;

  while(ConnectionState_dequeue_msg(state)) {
    ConnectionState_lock(state);

    if(ConnectionState_exec(state, UEv_MSG_QUEUED) == 0) {
      if(ConnectionState_send_msg(state)) {
        ConnectionState_exec(state, UEv_MSG_SENT);
      } else {
        ConnectionState_exec(state, UEv_FAIL);
        break; // abort on any error
      }
    }

    ConnectionState_unlock(state);
  }

  ConnectionState_exec(state, UEv_WRITE_CLOSE);
}


int ConnectionState_confirm_key(CryptState *state)
{
  return ConnectionState_exec((ConnectionState *)(state->data), UEv_PASS) == 0;
}

int ConnectionState_read_msg(ConnectionState *state)
{
  if(ConnectionState_done(state) || state->client == NULL) return 0;

  state->recv.body = Peer_recv(state->member->peer, &(state->recv.hdr));

  return state->recv.body != NULL;
}

void ConnectionState_incoming(void *data)
{
  ConnectionState *state = (ConnectionState *)data;

  ConnectionState_exec(state, UEv_KEY_PRESENT);
  ConnectionState_exec(state, UEv_PASS);
  ConnectionState_exec(state, UEv_PASS);

  while(ConnectionState_read_msg(state)) {
    ConnectionState_lock(state);

    if(ConnectionState_exec(state, UEv_MSG_RECV) == 0 && state->recv.msg) {
      ConnectionState_exec(state, UEv_SVC_RECV);
    } 

    ConnectionState_unlock(state);
  }

  ConnectionState_exec(state, UEv_READ_CLOSE);
}


int ConnectionState_half_close(ConnectionState *state)
{
  if(state->client) {
    server_destroy_client(state->client);
    state->client = NULL;

    if(state->member) {
      Route_unregister_all(state->hub->routes, state->member);
      Member_logout(&state->hub->members, state->member);
    }

    state->member = NULL;
    return 1;
  } else {
    return 0;
  }
}

int ConnectionState_rest_close(ConnectionState *state)
{
  assert(state->client == NULL && "Attempting to call rest_close before half_close called.");

  if(state->hub->closed != state) {
    SGLIB_LIST_ADD(ConnectionState, state->hub->closed, state, next);
    return 1;
  } else {
    return 0;
  }
}
