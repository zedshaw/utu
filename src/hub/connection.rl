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
  machine ConnectionState;
  access state->;
  include ConnectionState "hub/connection_actions.rl";


  open='O';
  key_present='P';
  fail='0';
  pass='1';
  svc_recv='s';
  msg_sent='e';
  msg_queued='Q';
  msg_recv='R';
  hate_apply='H';
  hate_challenge='h';
  hate_paid='b';
  hate_valid='V';
  hate_invalid='v';
  read_close='r';
  write_close='w';
  close = (read_close | write_close);

### state chart
  Connection = (
    start: ( open @open -> Accepting ),

    Accepting: ( key_present @key_prep -> KeyCheck | close @half_close -> Closing),

    KeyCheck: ( pass @tune -> Tuning | close @half_close -> Closing),
    Tuning: ( pass @register -> Registering | close @half_close -> Closing),
    Registering: ( pass @established -> Idle | close @half_close -> Closing),

    Idle: (
      msg_recv @recv -> Delivering |
      msg_queued @msg_ready -> Sending |
      hate_apply @hate_apply -> Hated |
      close @half_close -> Closing
    ),

    Hated: (
        start: ( hate_challenge @hate_challenge ->  Awaiting),
        Awaiting: ( hate_paid @hate_paid -> Validating ),
        Validating: ( 
          hate_valid @hate_valid | hate_invalid @hate_invalid 
        ) -> final
    ) -> Idle,

    Closing: (close @rest_close -> final),

    # during the aborting operation anything we get should just force a
    # shutdown immediately rather than hanging around
    Aborting: (
        any @half_close -> Closing
    ) >aborted,

    Delivering: (
        svc_recv @service -> Idle
    ) >msgid_check @clear_recv,

    Sending: ( 
        msg_sent @sent -> Idle |
        fail -> Aborting
    ) @clear_send

  ) >begin $!error;

  main := Connection;
}%%

%% write data;

int ConnectionState_init(ConnectionState *state)
{
  assert_mem(state);
  assert(state->next == NULL && "attempt to init an active state");

  %% write init;
  
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

  %% write exec;

  return ConnectionState_invariant(state, event);
}

int ConnectionState_finish(ConnectionState *state)
{
  assert_mem(state);

  %% write eof;

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
