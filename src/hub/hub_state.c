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

#include "hub/hub.h"

inline ConnectionState *Hub_new_or_reuse_conn(Hub *hub)
{
  ConnectionState *state = NULL;

  if((state = hub->closed) != NULL) {
    // there's some dead ones we can reuse
    SGLIB_LIST_DELETE(ConnectionState, hub->closed, state, next);
    memset(state, 0, sizeof(ConnectionState));
  } else {
    // nothing we can reuse, make a new one
    state = calloc(1,sizeof(ConnectionState));
  }
  assert_mem(state);

  return state;
}

void Hub_queue_conn(Hub *hub, MyriadClient *client)
{
  ConnectionState *state = Hub_new_or_reuse_conn(hub);

  state->hub = hub;
  state->client = client;

  ConnectionState_init(state);
  ConnectionState_exec(state, UEv_OPEN);
}

void Hub_connection_handler(void *data)
{
  assert_not(data, NULL);

  MyriadClient *client = (MyriadClient *)data;
  Hub *hub = (Hub *)client->server->data;

  Hub_exec(hub, UEv_OPEN);
  Hub_queue_conn(hub, client);
  Hub_exec(hub, UEv_CLOSE);
}

int Hub_init(char *argv_0)
{
  assert_not(argv_0, NULL);

  check(CryptState_init(), "failed to init crypto");

  server_init(argv_0);

  return 1;
  on_fail(return 0);
}


void Hub_listen(Hub *hub)
{
  Hub_exec(hub, UEv_LISTEN);
}


void Hub_broadcast(Hub *hub, Message *msg)
{
  Member *te = NULL;
  struct sglib_Member_iterator it;
  assert_not(msg, NULL);
  assert_not(hub, NULL);

  dbg("BROADCASTING MESSAGE:");
  Message_dump(msg);

  for(te=sglib_Member_it_init(&it,hub->members); te!=NULL; te=sglib_Member_it_next(&it)) {
    Member_send_msg(te, msg);
  } 
}
