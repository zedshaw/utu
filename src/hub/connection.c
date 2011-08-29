#line 1 "hub/connection.rl"
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

#line 88 "hub/connection.rl"



#line 26 "hub/connection.c"
static const char _ConnectionState_actions[] = {
	0, 1, 2, 1, 3, 1, 4, 1, 
	5, 1, 6, 1, 8, 1, 10, 1, 
	14, 1, 15, 1, 16, 1, 17, 1, 
	18, 1, 19, 1, 20, 1, 21, 2, 
	0, 1, 2, 12, 20, 2, 13, 8, 
	3, 11, 9, 7
};

static const char _ConnectionState_key_offsets[] = {
	0, 0, 1, 4, 7, 10, 13, 18, 
	19, 20, 22, 24, 24, 26, 27
};

static const char _ConnectionState_trans_keys[] = {
	79, 80, 114, 119, 49, 114, 119, 49, 
	114, 119, 49, 114, 119, 72, 81, 82, 
	114, 119, 104, 98, 86, 118, 48, 101, 
	114, 119, 115, 0
};

static const char _ConnectionState_single_lengths[] = {
	0, 1, 3, 3, 3, 3, 5, 1, 
	1, 2, 2, 0, 2, 1, 0
};

static const char _ConnectionState_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0
};

static const char _ConnectionState_index_offsets[] = {
	0, 0, 2, 6, 10, 14, 18, 24, 
	26, 28, 31, 34, 35, 38, 40
};

static const char _ConnectionState_indicies[] = {
	1, 0, 2, 3, 3, 0, 4, 3, 
	3, 0, 5, 3, 3, 0, 6, 3, 
	3, 0, 7, 8, 9, 3, 3, 0, 
	10, 0, 11, 0, 12, 13, 0, 14, 
	15, 0, 16, 17, 17, 0, 18, 0, 
	0, 0
};

static const char _ConnectionState_trans_targs_wi[] = {
	0, 2, 3, 12, 4, 5, 6, 7, 
	10, 13, 8, 9, 6, 6, 11, 6, 
	12, 14, 6
};

static const char _ConnectionState_trans_actions_wi[] = {
	25, 31, 1, 27, 3, 5, 7, 15, 
	13, 9, 17, 19, 21, 23, 11, 37, 
	34, 29, 40
};

static const int ConnectionState_start = 1;
static const int ConnectionState_first_final = 14;
static const int ConnectionState_error = 0;

static const int ConnectionState_en_main = 1;
static const int ConnectionState_en_main_Connection_Aborting = 11;

#line 91 "hub/connection.rl"

int ConnectionState_init(ConnectionState *state)
{
  assert_mem(state);
  assert(state->next == NULL && "attempt to init an active state");

  
#line 99 "hub/connection.c"
	{
	 state->cs = ConnectionState_start;
	}
#line 98 "hub/connection.rl"
  
  return 1;
}

inline int ConnectionState_invariant(ConnectionState *state, HubEvent event)
{
  assert_mem(state);

  if ( state->cs == ConnectionState_error ) {
    log(ERROR, "Error in connection state with event '%c'", event);
    return -1;
  }

  if ( state->cs >= ConnectionState_first_final )
    return 1;
  
  return 0;
}

int ConnectionState_exec(ConnectionState *state, HubEvent event)
{
  assert_mem(state);

  const char *p = (const char *)&event;
  const char *pe = p+1;

  if(ConnectionState_done(state)) return -1; 

  
#line 133 "hub/connection.c"
	{
	int _klen, _ps;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _out;
	if (  state->cs == 0 )
		goto _out;
_resume:
	_keys = _ConnectionState_trans_keys + _ConnectionState_key_offsets[ state->cs];
	_trans = _ConnectionState_index_offsets[ state->cs];

	_klen = _ConnectionState_single_lengths[ state->cs];
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

	_klen = _ConnectionState_range_lengths[ state->cs];
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
	_ps =  state->cs;
	_trans = _ConnectionState_indicies[_trans];
	 state->cs = _ConnectionState_trans_targs_wi[_trans];

	if ( _ConnectionState_trans_actions_wi[_trans] == 0 )
		goto _again;

	_acts = _ConnectionState_actions + _ConnectionState_trans_actions_wi[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 4 "hub/connection.rl"
	{ 
    trc(begin,(_ps),( state->cs));
    state->recv_count = state->send_count = 1L;
    memset(&state->lock, 0, sizeof(QLock));
    state->next = NULL;
  }
	break;
	case 1:
#line 11 "hub/connection.rl"
	{
    trc(open,(_ps),( state->cs));
    ConnectionState_incoming(state);
  }
	break;
	case 2:
#line 16 "hub/connection.rl"
	{
    trc(key_prep,(_ps),( state->cs));
    CryptState *crypt = CryptState_create(state->hub->name, state->hub->key);
    crypt->data = state;
    state->peer = Peer_create(crypt, state->client->fd, ConnectionState_confirm_key);

    if(state->peer == NULL) {
      log(ERROR, "peer failed to create");
      { state->cs = 11; goto _again;}
    }
  }
	break;
	case 3:
#line 30 "hub/connection.rl"
	{
    trc(tune,(_ps),( state->cs));
    if(!Peer_establish_receiver(state->peer))  {
      log(ERROR, "peer failed to establish");
      { state->cs = 11; goto _again;} 
    }
  }
	break;
	case 4:
#line 38 "hub/connection.rl"
	{
    trc(register,(_ps),( state->cs));
    state->member = Member_login(&state->hub->members, state->peer);
    if(state->member == NULL) {
      log(ERROR, "member failed to login");
      { state->cs = 11; goto _again;}
    }
  }
	break;
	case 5:
#line 47 "hub/connection.rl"
	{
    trc(established,(_ps),( state->cs));
    taskcreate(ConnectionState_outgoing, state, HUB_DEFAULT_STACK);
  }
	break;
	case 6:
#line 52 "hub/connection.rl"
	{
    trc(recv,(_ps),( state->cs));
    if(!state->recv.hdr || !state->recv.body) {
      log(ERROR, "recv action called but hdr/body missing");
      { state->cs = 11; goto _again;}
    }
    state->recv.msg = Message_decons(state->recv.hdr, state->recv.body);
 
    if(!state->recv.msg) {
      log(ERROR, "failed to parse recv message");
      { state->cs = 11; goto _again;}
    }
    state->recv.msg->from = state->member;
  }
	break;
	case 7:
#line 67 "hub/connection.rl"
	{
    trc(clear_recv,(_ps),( state->cs));
    state->recv.msg = NULL;
    state->recv.hdr = NULL;
    state->recv.body = NULL;
  }
	break;
	case 8:
#line 74 "hub/connection.rl"
	{
    trc(clear_send,(_ps),( state->cs));
    state->send.msg = NULL;
  }
	break;
	case 9:
#line 79 "hub/connection.rl"
	{
    trc(service,(_ps),( state->cs));
    if(!Hub_service_message(state)) {
      { state->cs = 11; goto _again;}
    }
  }
	break;
	case 10:
#line 86 "hub/connection.rl"
	{
    trc(msg_ready,(_ps),( state->cs));
  }
	break;
	case 11:
#line 90 "hub/connection.rl"
	{
    trc(msgid_check,(_ps),( state->cs));
    if(state->recv.msg->msgid != state->recv_count) {
      Message_ref_inc(state->recv.msg);  // we own it now
      log(ERROR, "msg id is wrong, should be %ju but got %ju", (uintmax_t)state->recv_count, (uintmax_t)state->recv.msg->msgid);
      { state->cs = 11; goto _again;}
    }
    state->recv_count++;
  }
	break;
	case 12:
#line 100 "hub/connection.rl"
	{
    trc(aborted,(_ps),( state->cs));
  }
	break;
	case 13:
#line 104 "hub/connection.rl"
	{
    trc(sent,(_ps),( state->cs));
    Member_delete_msg(state->member);
  }
	break;
	case 14:
#line 109 "hub/connection.rl"
	{
    trc(hate_apply,(_ps),( state->cs));
  }
	break;
	case 15:
#line 113 "hub/connection.rl"
	{
    trc(hate_challenge,(_ps),( state->cs));
  }
	break;
	case 16:
#line 117 "hub/connection.rl"
	{
    trc(hate_paid,(_ps),( state->cs));
  }
	break;
	case 17:
#line 121 "hub/connection.rl"
	{
    trc(hate_valid,(_ps),( state->cs));
  }
	break;
	case 18:
#line 125 "hub/connection.rl"
	{
    trc(hate_invalid,(_ps),( state->cs));
  }
	break;
	case 19:
#line 129 "hub/connection.rl"
	{
    trc(error,(_ps),( state->cs)); 

    if(!ConnectionState_half_close(state)) {
      log(WARN, "Connection is already half closed.");
    }

    if(!ConnectionState_rest_close(state)) {
      log(WARN, "Connection is already fully closed.");
    }
  }
	break;
	case 20:
#line 141 "hub/connection.rl"
	{
    trc(half_close,(_ps),( state->cs));
    if(!ConnectionState_half_close(state)) {
      log(WARN, "Connection is already half closed.");
    }
  }
	break;
	case 21:
#line 148 "hub/connection.rl"
	{
    trc(rest_close,(_ps),( state->cs));
    if(!ConnectionState_rest_close(state)) {
      log(WARN, "Connection is already fully closed.");
    }
  }
	break;
#line 401 "hub/connection.c"
		}
	}

_again:
	if (  state->cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_out: {}
	}
#line 127 "hub/connection.rl"

  return ConnectionState_invariant(state, event);
}

int ConnectionState_finish(ConnectionState *state)
{
  assert_mem(state);

  
#line 422 "hub/connection.c"
#line 136 "hub/connection.rl"

  int rc = ConnectionState_invariant(state, 0);

  // destroy the message, or the hdr body if no message
  if(state->recv.msg) {
    Message_destroy(state->recv.msg);
  } else {
    if(state->recv.body) Node_destroy(state->recv.body);
    if(state->recv.hdr) Node_destroy(state->recv.hdr);
  }

  if(state->send.msg) {
    Message_destroy(state->send.msg);
  }

  // clear out the messages
  state->recv.msg = NULL;
  state->recv.hdr = NULL;
  state->recv.body = NULL;
  state->send.msg = NULL;

  if(state->peer) Peer_destroy(state->peer, 1);
  state->peer = NULL;

  return rc; 
}

int ConnectionState_done(ConnectionState *state)
{
  assert_mem(state);
  return state->cs == ConnectionState_error || state->cs == ConnectionState_first_final;
}
