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


#include "member.h"


SGLIB_DEFINE_RBTREE_FUNCTIONS(Member, left, right, color, MEMBER_COMPARATOR);


Member *Member_find(Member *map, bstring pubkey)
{
  Member *e = NULL, temp = {.key = pubkey};
  assert_not(pubkey, NULL);

  // find the member this is destined for
  e = sglib_Member_find_member(map, &temp);

  return e;
}


Message *Member_first_msg(Member *member)
{
  assert_not(member, NULL);
  return MsgQueue_first(member->queue);
}


int Member_delete_msg(Member *member)
{
  assert_not(member, NULL);

  return MsgQueue_delete(member->queue);
}

int Member_send_msg(Member *member, Message *msg)
{
  int rc = 0;
  assert_not(member, NULL);
  assert_not(msg, NULL);

  check(!MsgQueue_is_full(member->queue), "member's queue is full");
  rc = MsgQueue_add(member->queue, msg);
  check(rc, "failed to add to member's queue");

  MsgQueue_wake_all(member->queue);

  return 1;
  on_fail(return 0);
}

Member *Member_create(Peer *peer)
{
  assert_not(peer, NULL);
  assert_not(peer->state, NULL);

  // fresh meat, hook them up with a registration
  Member *e = calloc(1,sizeof(Member));
  assert_mem(e);
  e->peer = peer;
  e->key = CryptState_export_key(peer->state, CRYPT_THEIR_KEY, PK_PUBLIC);
  e->queue = MsgQueue_create(MEMBER_MSG_QUEUE_LENGTH);
  e->routes = Heap_create(NULL);

  return e;
}

Member *Member_login(Member **map, Peer *peer)
{
  Member *e = NULL;

  e = Member_create(peer);
  check(e, "Failed to create new member from peer.");

  // all good, log them in
  sglib_Member_add(map, e);

  return e;
  on_fail(if(e) Member_destroy(e); return NULL);
}


void Member_logout(Member **map, Member *member)
{
  assert_not(map, NULL);
  assert_not(*map, NULL);
  assert_not(member, NULL);

  Member *el = NULL;

  MsgQueue_mark_dead(member->queue);

  // wake everyone up so they get the message that we're done
  MsgQueue_wake_all(member->queue);
  taskyield(); // need to yield so they run

  // result ignored
  sglib_Member_delete_if_member(map, member, &el);

  // now we're all done
  Member_destroy(member);
}


void Member_destroy(Member *mb)
{
  if(mb->key) bdestroy(mb->key); mb->key = NULL;
  if(mb->queue) MsgQueue_destroy(mb->queue);
  if(mb->routes) Heap_destroy(mb->routes);
  free(mb);
}

void Member_destroy_map(Member **map)
{
  Member *te = NULL;
  struct sglib_Member_iterator  it;

  assert_not(map, NULL);
  assert_not(*map, NULL);

  for(te=sglib_Member_it_init(&it,*map); te!=NULL; te=sglib_Member_it_next(&it)) {
    Member_destroy(te);
  }

  if(*map) {
    free(*map);
    *map = NULL;
  }
}
