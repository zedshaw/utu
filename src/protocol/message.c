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


#include "message.h"
#include <time.h>

void *message_test_calloc()
{
  return calloc(1, sizeof(Message));
}

Message* Message_alloc(Node *hdr, Node *body) 
{
  Message *msg = message_test_calloc();
  assert_mem(msg);

  msg->hdr = hdr;
  msg->body = body;

  // mark the time received
  msg->received_at = time(NULL);

  if(body) {
    msg->type = body->name;
  }

  return msg;
}

Message* Message_decons(Node *hdr, Node *body)
{
  Message *msg = NULL;
  int rc = 0;

  assert_not(hdr, NULL);
  assert_not(body, NULL);

  msg = Message_alloc(hdr, body);
  Message_ref_inc(msg); // defaults to being incremented

  check_then(msg->type, "body's type was NULL", Node_dump(body, ' ', 1));

  rc = Node_decons(hdr, 0, "[nw", &msg->msgid, "header");
  check(rc, "failed to deconstruct node");

  // don't process the word name but instead take it directly off the root
  rc = Node_decons(body, 0, "[G.", &msg->data);
  check_then(rc, "failed to deconstruct body", Node_dump(body, ' ', 1));

  Message_ref_dec(msg);  // don't need it anymore, it's the caller's problem
  return msg;

  on_fail(if(msg) Message_destroy(msg); return NULL);
}

inline Node *Message_cons_header(uint64_t msgid)
{
  return Node_cons("[nw", msgid,  "header");
}

Node *Message_cons(Node **hdr, uint64_t msgid, Node *data, const char *type)
{
  assert_not(hdr, NULL);
  assert_not(data, NULL);
  assert_not(type, NULL);

  *hdr = Message_cons_header(msgid);

  Node *body = Node_cons("[Gw", data, type);

  return body;
}

void Message_destroy(Message *msg)
{
  assert_not(msg, NULL);

  Message_ref_dec(msg);
  assert(msg->ref_count >= 0 && "reference count decremented too far");

  if(msg->ref_count == 0) {
    if(msg->hdr) Node_destroy(msg->hdr);
    if(msg->body) Node_destroy(msg->body);
    free(msg);
  }
}


void Message_dump(Message *msg)
{
  fprintf(stderr, "Message: { msgid: %ju, type: %s received_at: %u, data: ", 
      (uintmax_t)msg->msgid, bdata(msg->type), (unsigned int)msg->received_at);

  if(msg->data != NULL) {
    bstring n = Node_bstr(msg->data, 0);
    fprintf(stderr, "%s", bdata(n));
    bdestroy(n);
  } else {
    printf("(null)");
  }

  fprintf(stderr,"BODY: ");
  if(msg->body) Node_dump(msg->body, ' ', 0);
  fprintf(stderr, "HEADER: ");
  if(msg->body) Node_dump(msg->hdr, ' ', 0);
  fprintf(stderr, "}\n");
}
