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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cut.h"
#include "hub/member.h"
#include <myriad/myriad.h>

Member *global_members = NULL;

void __CUT_BRINGUP__Member( void )
{
}

void member_listener(void *data)
{
  int i = 0;
  Member *mb = (Member *)data;
  assert_not(mb, NULL);
  Message *msg = NULL;

  for(i = 0; i < 10; i++) {
    msg = Member_first_msg(mb);
    ASSERT(msg != NULL, "got a NULL msg");
    Member_delete_msg(mb);
  }
}

void member_sender(void *data) 
{
  int i = 0;
  Member *mb = (Member *)data;
  assert_not(mb, NULL);

  // load up 10 messages to make sure overload works
  for( i = 0; i < 10; i++) {
    Message *msg = Message_alloc(NULL, NULL);
    msg->msgid = i;
    ASSERT(Member_send_msg(mb, msg), "Failed to send msg");
  }

  taskcreate(member_listener, mb, 32*1024);
  taskyield();  // let the caller block

  Member_logout(&global_members, mb);
}

int simple_key_confirm(CryptState *state) 
{
  return 1;
}

void __CUT__Member_login_send_logout()
{
  bstring name = bfromcstr("testname");
  CryptState *state = CryptState_create(name, NULL);
  // fake this out so we can test with it
  state->them = state->me;

  Peer *peer = Peer_create(state, 0, simple_key_confirm);
  ASSERT(peer != NULL, "failed to make peer");

  Member *m1 = Member_login(&global_members, peer);
  ASSERT(m1 != NULL, "failed to login member");

  Member *m2 = sglib_Member_find_member(global_members, m1);
  ASSERT(m2 == m1, "failed to get the right member");

  taskdispatch(member_sender, m1, 32*1024);


  m2 = sglib_Member_find_member(global_members, m1);
  ASSERT(m2 == NULL, "found a member when we shouldn't");

  memset(&state->them, 0, sizeof(state->them));
  Peer_destroy(peer, 1);
  bdestroy(name);

  if(global_members) Member_destroy_map(&global_members);
  ASSERT(global_members == NULL, "global member map not destroyed");
}


void __CUT_TAKEDOWN__Member( void ) 
{
  global_members = NULL;
}
