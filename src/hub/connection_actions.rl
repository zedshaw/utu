%%{
  machine ConnectionState;

  action begin { 
    trc(begin,fcurs,ftargs);
    state->recv_count = state->send_count = 1L;
    memset(&state->lock, 0, sizeof(QLock));
    state->next = NULL;
  }

  action open {
    trc(open,fcurs,ftargs);
    ConnectionState_incoming(state);
  }

  action key_prep {
    trc(key_prep,fcurs,ftargs);
    CryptState *crypt = CryptState_create(state->hub->name, state->hub->key);
    crypt->data = state;
    state->peer = Peer_create(crypt, state->client->fd, ConnectionState_confirm_key);

    if(state->peer == NULL) {
      log(ERROR, "peer failed to create");
      fgoto Aborting;
    }
  }

  action key_check { trc(key_check,fcurs,ftargs);  }

  action tune {
    trc(tune,fcurs,ftargs);
    if(!Peer_establish_receiver(state->peer))  {
      log(ERROR, "peer failed to establish");
      fgoto Aborting; 
    }
  }

  action register {
    trc(register,fcurs,ftargs);
    state->member = Member_login(&state->hub->members, state->peer);
    if(state->member == NULL) {
      log(ERROR, "member failed to login");
      fgoto Aborting;
    }
  }

  action established {
    trc(established,fcurs,ftargs);
    taskcreate(ConnectionState_outgoing, state, HUB_DEFAULT_STACK);
  }

  action recv {
    trc(recv,fcurs,ftargs);
    if(!state->recv.hdr || !state->recv.body) {
      log(ERROR, "recv action called but hdr/body missing");
      fgoto Aborting;
    }
    state->recv.msg = Message_decons(state->recv.hdr, state->recv.body);
 
    if(!state->recv.msg) {
      log(ERROR, "failed to parse recv message");
      fgoto Aborting;
    }
    state->recv.msg->from = state->member;
  }

  action clear_recv {
    trc(clear_recv,fcurs,ftargs);
    state->recv.msg = NULL;
    state->recv.hdr = NULL;
    state->recv.body = NULL;
  }

  action clear_send {
    trc(clear_send,fcurs,ftargs);
    state->send.msg = NULL;
  }

  action service {
    trc(service,fcurs,ftargs);
    if(!Hub_service_message(state)) {
      fgoto Aborting;
    }
  }

  action msg_ready {
    trc(msg_ready,fcurs,ftargs);
  }

  action msgid_check {
    trc(msgid_check,fcurs,ftargs);
    if(state->recv.msg->msgid != state->recv_count) {
      Message_ref_inc(state->recv.msg);  // we own it now
      log(ERROR, "msg id is wrong, should be %ju but got %ju", (uintmax_t)state->recv_count, (uintmax_t)state->recv.msg->msgid);
      fgoto Aborting;
    }
    state->recv_count++;
  }

  action aborted {
    trc(aborted,fcurs,ftargs);
  }

  action sent {
    trc(sent,fcurs,ftargs);
    Member_delete_msg(state->member);
  }

  action hate_apply {
    trc(hate_apply,fcurs,ftargs);
  }

  action hate_challenge {
    trc(hate_challenge,fcurs,ftargs);
  }

  action hate_paid {
    trc(hate_paid,fcurs,ftargs);
  }

  action hate_valid {
    trc(hate_valid,fcurs,ftargs);
  }

  action hate_invalid {
    trc(hate_invalid,fcurs,ftargs);
  }

  action error {
    trc(error,fcurs,ftargs); 

    if(!ConnectionState_half_close(state)) {
      log(WARN, "Connection is already half closed.");
    }

    if(!ConnectionState_rest_close(state)) {
      log(WARN, "Connection is already fully closed.");
    }
  }

  action half_close {
    trc(half_close,fcurs,ftargs);
    if(!ConnectionState_half_close(state)) {
      log(WARN, "Connection is already half closed.");
    }
  }

  action rest_close {
    trc(rest_close,fcurs,ftargs);
    if(!ConnectionState_rest_close(state)) {
      log(WARN, "Connection is already fully closed.");
    }
  }
}%%

