#include "commands.h"
#include "info.h"

/** 
 * @function send_response
 * @brief Builds the response and puts it in the member's queue.
 * @param member : Who to deliver to.
 * @param response : The response message body node.
 * @param type : The final root type (rpy or err)
 */
inline void send_response(Member *member, Node *response, const char *type)
{
  // deliver message to whoever this is from
  Node *hdr = NULL;
  Node *body = Message_cons(&hdr, 0, response, type);
  Message *msg = Message_alloc(hdr, body);
  Member_send_msg(member, msg);
}

/** 
 * @function extract_message
 * @brief Takes the base message out of the service message wrapper.
 * @param message : Where to extract from.
 * @return inline Node *: Finaly internal message.
 */
inline Node *extract_message(Message *message)
{
  Node *msg = message->data->child;

  return msg ? msg->child : NULL;
}


static int Hub_route_register_cb(struct ConnectionState *conn, Member *from, Node *message)
{
  return Route_register(conn->hub->routes, message, from);
}

static int Hub_route_unregister_cb(struct ConnectionState *conn, Member *from, Node *message)
{
  return Route_unregister(conn->hub->routes, message, from);
}


static int Hub_route_members_cb(struct ConnectionState *conn, Member *from, Node *message)
{
  trace();
  Route *target = Route_find(conn->hub->routes, message);
  Node *response = Node_cons("[b@w", Node_bstr(message, ' '), "@path", "members");

  if(target) {
    // construct response nodes
    HEAP_ITERATE(target->members, indx, Member *, member, 
        Node_new_string(response, bstrcpy(Member_name(member))));
  } else {
    Node_new_string(response, bfromcstr("Requested route does not exist."));
  }
  
  Node_dump(response, ' ', 1);

  send_response(from, response, target ? "rpy" : "err");

  return 1;
}

static int Hub_route_children_cb(struct ConnectionState *conn, Member *from, Node *message)
{
  trace();
  Route *target = Route_find(conn->hub->routes, message);
  Node *response = Node_cons("[b@w", Node_bstr(message, ' '), "@path", "children");

  if(target) {
    HEAP_ITERATE(target->children, indx, Route *, route,
        Node_new_string(response, bstrcpy(route->name)));
  }

  Node_dump(response, ' ', 1);
  send_response(from, response, "rpy");

  return 1;
}

static int Hub_member_register_cb(struct ConnectionState *conn, Member *from, Node *message)
{
  trace();
  Node *response = Node_cons("[w", "registered");

  // save it and then they're invited
  send_response(from, response, "rpy");

  return 1;
}


static int Hub_member_invite_cb(struct ConnectionState *conn,Member *from, Node *message)
{
  trace();
  Node *response = Node_cons("[w", "invited");

  Node_dump(message, ' ', 1);

  send_response(from, response, "rpy");

  return 1;
}

static int Hub_member_mendicate_cb(struct ConnectionState *conn, Member *from, Node *message)
{
  trace();
  Node *response = Node_cons("[w", "mendicated");

  send_response(from, response, "rpy");

  return 1;
}

static int Hub_member_send_cb(struct ConnectionState *conn,  Member *from, Node *message)
{
  trace();
  Node *response = Node_cons("[w", "sent");

  Node_dump(message, ' ', 1);

  send_response(from, response, "rpy");

  return 1;
}

static int Hub_system_ping_cb(struct ConnectionState *conn, Member *from, Node *message)
{
  trace();
  check(message->type == TYPE_NUMBER, "Ping didn't have a number for the time.");

  Node *response = Node_cons("[nw", time(NULL) - message->value.number, "pong");

  send_response(from, response, "rpy");

  return 1;
  on_fail(return 0);
}

static int Hub_info_generic(struct ConnectionState *conn, Node *message, Member *from, const char *operation, bstring (*info_op)(bstring path, bstring *error))
{
  bstring info_name = NULL;
  Node *response = NULL;
  bstring error = NULL;
  bstring data = NULL;

  check(Node_decons(message, 0, "[sw", &info_name, "name"), "Invalid info request.");
  check(blength(info_name) > 0, "Zero length info name request, killing client.");

  data = info_op(info_name, &error);

  if(!error) {
    assert(data && "Invalid response from info, data can't be NULL.");
    response = Node_cons("[[sw[bww", bstrcpy(info_name), "name", data, operation, "info");
  } else {
    if(data) bdestroy(data);
    response = Node_cons("[[sw[sww", bstrcpy(info_name), "name", error, "error", "info");
  }

  send_response(from, response, error ? "err" : "rpy");

  return 1;
  on_fail(return 0);
}

static int Hub_info_tag_cb(struct ConnectionState *conn, Member *from, Node *message)
{
  trace();
  return Hub_info_generic(conn, message, from, "tag", Info_tag); 
}

static int Hub_info_get_cb(struct ConnectionState *conn, Member *from, Node *message)
{
  trace();
  return Hub_info_generic(conn, message, from, "get", Info_get); 
}

static int Hub_info_list_cb(struct ConnectionState *conn, Member *from, Node *message)
{
  trace();
  return Hub_info_generic(conn, message, from, "list", Info_list); 
}

/**
 * A Registration is used internally to just record
 * which callbacks are assigned to which messages.
 */
typedef struct Registration {
  const char *child;
  const char *root;
  Route_internal_callback callback;
} Registration;

Registration registrations[] = {
  // routing control functions
  {"register","route", Hub_route_register_cb },
  {"unregister","route", Hub_route_unregister_cb },
  {"members","route", Hub_route_members_cb },
  {"children","route", Hub_route_children_cb },

  // membership control functions
  {"register","member", Hub_member_register_cb },
  {"mendicate","member", Hub_member_mendicate_cb },
  {"invite","member", Hub_member_invite_cb },

  // member to member messaging
  {"send","member", Hub_member_send_cb },

  // generic information operations
  {"get","info", Hub_info_get_cb },
  {"list","info", Hub_info_list_cb },
  {"tag","info", Hub_info_tag_cb },

  // system level commands
  {"ping","system", Hub_system_ping_cb },
  { NULL, NULL, NULL}
};

int Hub_commands_register(Hub *hub) 
{
  Registration *reg = registrations;
  assert_mem(hub);
  assert_mem(hub->routes);

  // TODO: for some weird reason the compiler says using a for here has no effect
  while(reg->root != NULL) {
    log(INFO, "Registering '%s/%s' to Hub", reg->root, reg->child);

    Node *node = Node_cons("[[ww", reg->child, reg->root);
    check(node, "Failed to parse registration.");

    check(Route_register_callback(hub->routes, node, reg->callback), "failed to register");
    Node_destroy(node);

    reg++;
  }

  return 1;
  on_fail(return 0);
}

int Hub_service_message(struct ConnectionState *state)
{
  Route *target = Route_find(state->hub->routes, state->recv.msg->data);
  check(target, "Routing failure: invalid routing request.");

  if(target->is_internal_callback) {
    // this is a callback that only targets the Hub, call it
    Node *message = extract_message(state->recv.msg);
    check(message, "Invalid service message format, must have at least one internal node.");
    check(target->callback(state, state->recv.msg->from, message), "Callback returned false so aborting connection.");
  } else {
    // looks like a regular delivery, send it on
    int count = Route_deliver(target, state->recv.msg);
    dbg("Routed to %s with %d deliveries", bdata(target->name), count);
  }

  return 1;

  on_fail(Message_ref_inc(state->recv.msg); return 0);
}
