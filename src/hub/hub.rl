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

#include <stdlib.h>
#include <stdio.h>
#include "hub/hub.h"
#include "hub/commands.h"

%%{
  machine Hub;
  access state->;

  include Hub "hub/hub_actions.rl";

### events
  listen='L';
  gen_key='K';
  gen_key_done='k';
  key_file_load='F';
  conn_open='O';
  conn_close='C';
  done='D';

### state chart
  Hub = (
    start: (
      gen_key @gen_key -> GenKeys |
      key_file_load @keys_ready -> CryptoReady
    ),

    GenKeys: ( gen_key_done @keys_ready -> CryptoReady ),

    CryptoReady: (
      listen @listen -> Listening |
      done @cleanup -> final
    ),

    Listening: (
      conn_open @open -> Listening |
      conn_close @close -> Listening |
      done @cleanup -> final
    )
  ) >begin @!error;

  main := Hub ;
}%%

%% write data;


Hub *Hub_create(bstring host, bstring port, bstring name, bstring key)
{
  assert_not(host, NULL);
  assert_not(port, NULL);
  assert_not(name, NULL);

  Hub *state = calloc(1, sizeof(Hub));
  assert_mem(state);

  state->host = host;
  state->port = port;
  state->name = name;
  state->routes = Route_create_root("root");
  assert_mem(state->routes);

  Hub_commands_register(state);

  %% write init;

  if(key == NULL) {
    check(!Hub_exec(state, UEv_GEN_KEY), "state failure UEv_GEN_KEY");
    check(!Hub_exec(state, UEv_GEN_KEY_DONE), "state failure UEv_GEN_KEY_DONE");
  } else {
    // TODO: push this to the caller really
    check(!Hub_exec(state, UEv_KEY_FILE_LOAD), "state failure UEv_KEY_FILE_LOAD");
    state->key = key;
  }

	return state;
  on_fail(Hub_destroy(state); return NULL);
}


inline int Hub_invariant(Hub *state, HubEvent event)
{
  if ( state->cs == Hub_error ) {
    log(ERROR, "Invalid Hub event '%c'", event);
    return -1;
  }

  if ( state->cs >= Hub_first_final )
    return 1;

  return 0;
}

int Hub_exec(Hub *state, HubEvent event)
{
  const char *p = (const char *)&event;
  const char *pe = p+1;

  assert_not(state, NULL);

  %% write exec;

  return Hub_invariant(state, event);
}

int Hub_destroy(Hub *state)
{
  assert_not(state, NULL);

  Hub_exec(state, UEv_DONE); 

  %% write eof;

  int rc = Hub_invariant(state, 0);

  // TODO: make this work better with the pool rather than destroy the whole world
  Route_destroy(state->routes);

  free(state);

  // clear any errno just in case
  errno = 0;

  return rc;
}

