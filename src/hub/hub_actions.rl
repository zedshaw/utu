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

%%{
  machine Hub;

  action begin {  }

  action cleanup {

    if(state->name) bdestroy(state->name);
    if(state->key) bdestroy(state->key);

    if(state->server) {
      server_destroy(state->server);
    } else {
      bdestroy(state->host);
    }

    // clean up all the remaining connection states
    ConnectionState *i = NULL;

    i = NULL;
    SGLIB_LIST_MAP_ON_ELEMENTS(ConnectionState, state->closed, i, next, ConnectionState_destroy(i));
  }

  action error { log(ERROR, "state machine error at '%c'", *fpc); }

  action listen {
    //TODO: setup a signal handler that will do a clean-up
    server_signal_ignore(SIGPIPE); // need to ignore SIGPIPE or crashing happens
    server_listen(state->server);
  }

  action gen_key {
    // need to extract the key for later
    CryptState *crypt = CryptState_create(state->name, state->key);
    assert_mem(crypt);

    // TODO: do this more efficiently
    state->key = CryptState_export_key(crypt, CRYPT_MY_KEY, PK_PRIVATE);
    CryptState_destroy(crypt);
  }


  action keys_ready {
    state->server = server_create(state->host, state->port, Hub_connection_handler, NULL, 0);
    state->server->data = state;  // needed so that the listener can get at the state
  }

  action close {
    // the conn at the front of closed is the one that just closed
    assert_not(state->closed, NULL);
    ConnectionState_finish(state->closed);
  }

  action open {
    // TODO: verify this is even needed for anything, maybe throttling
  }
}%%
