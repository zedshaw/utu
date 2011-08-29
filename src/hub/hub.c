#line 1 "hub/hub.rl"
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

#line 56 "hub/hub.rl"



#line 25 "hub/hub.c"
static const char _Hub_actions[] = {
	0, 1, 1, 1, 2, 1, 3, 1, 
	5, 1, 6, 1, 7, 2, 0, 4, 
	2, 0, 5
};

static const char _Hub_key_offsets[] = {
	0, 0, 2, 4, 7, 8
};

static const char _Hub_trans_keys[] = {
	70, 75, 68, 76, 67, 68, 79, 107, 
	0
};

static const char _Hub_single_lengths[] = {
	0, 2, 2, 3, 1, 0
};

static const char _Hub_range_lengths[] = {
	0, 0, 0, 0, 0, 0
};

static const char _Hub_index_offsets[] = {
	0, 0, 3, 6, 10, 12
};

static const char _Hub_trans_targs_wi[] = {
	2, 4, 0, 5, 3, 0, 3, 5, 
	3, 0, 2, 0, 0, 0
};

static const char _Hub_trans_actions_wi[] = {
	16, 13, 3, 1, 5, 3, 9, 1, 
	11, 3, 7, 3, 0, 0
};

static const int Hub_start = 1;
static const int Hub_first_final = 5;
static const int Hub_error = 0;

static const int Hub_en_main = 1;

#line 59 "hub/hub.rl"


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

  
#line 90 "hub/hub.c"
	{
	 state->cs = Hub_start;
	}
#line 79 "hub/hub.rl"

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

  
#line 131 "hub/hub.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _out;
	if (  state->cs == 0 )
		goto _out;
_resume:
	_keys = _Hub_trans_keys + _Hub_key_offsets[ state->cs];
	_trans = _Hub_index_offsets[ state->cs];

	_klen = _Hub_single_lengths[ state->cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _Hub_range_lengths[ state->cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += ((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	 state->cs = _Hub_trans_targs_wi[_trans];

	if ( _Hub_trans_actions_wi[_trans] == 0 )
		goto _again;

	_acts = _Hub_actions + _Hub_trans_actions_wi[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 17 "hub/hub.rl"
	{  }
	break;
	case 1:
#line 19 "hub/hub.rl"
	{

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
	break;
	case 2:
#line 37 "hub/hub.rl"
	{ log(ERROR, "state machine error at '%c'", *p); }
	break;
	case 3:
#line 39 "hub/hub.rl"
	{
    //TODO: setup a signal handler that will do a clean-up
    server_signal_ignore(SIGPIPE); // need to ignore SIGPIPE or crashing happens
    server_listen(state->server);
  }
	break;
	case 4:
#line 45 "hub/hub.rl"
	{
    // need to extract the key for later
    CryptState *crypt = CryptState_create(state->name, state->key);
    assert_mem(crypt);

    // TODO: do this more efficiently
    state->key = CryptState_export_key(crypt, CRYPT_MY_KEY, PK_PRIVATE);
    CryptState_destroy(crypt);
  }
	break;
	case 5:
#line 56 "hub/hub.rl"
	{
    state->server = server_create(state->host, state->port, Hub_connection_handler, NULL, 0);
    state->server->data = state;  // needed so that the listener can get at the state
  }
	break;
	case 6:
#line 61 "hub/hub.rl"
	{
    // the conn at the front of closed is the one that just closed
    assert_not(state->closed, NULL);
    ConnectionState_finish(state->closed);
  }
	break;
	case 7:
#line 67 "hub/hub.rl"
	{
    // TODO: verify this is even needed for anything, maybe throttling
  }
	break;
#line 273 "hub/hub.c"
		}
	}

_again:
	if (  state->cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_out: {}
	}
#line 115 "hub/hub.rl"

  return Hub_invariant(state, event);
}

int Hub_destroy(Hub *state)
{
  assert_not(state, NULL);

  Hub_exec(state, UEv_DONE); 

  
#line 296 "hub/hub.c"
#line 126 "hub/hub.rl"

  int rc = Hub_invariant(state, 0);

  // TODO: make this work better with the pool rather than destroy the whole world
  Route_destroy(state->routes);

  free(state);

  // clear any errno just in case
  errno = 0;

  return rc;
}

