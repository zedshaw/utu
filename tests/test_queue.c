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
#include "hub/queue.h"
#include <myriad/myriad.h>

MsgQueue *global_queue = NULL;

void __CUT_BRINGUP__MsgQueue( void )
{
}


void __CUT__MsgQueue_base_operations()
{
  int i = 0;
  global_queue = MsgQueue_create(10);
  Message *msg = NULL;

  assert_mem(global_queue);

  ASSERT(MsgQueue_is_empty(global_queue), "should be empty");
  ASSERT(!MsgQueue_is_full(global_queue), "should not be full");
   
  for(i = 0; i < 9; i++) {
    msg = Message_alloc(NULL, NULL);
    ASSERT(MsgQueue_add(global_queue, msg), "add failed");
  }

  ASSERT(!MsgQueue_add(global_queue, msg), "add should fail");
  ASSERT(!MsgQueue_is_empty(global_queue), "queue should not be empty");
  ASSERT(MsgQueue_is_full(global_queue), "queue should be full");

  for(i = 0; i < 9; i++) {
    ASSERT(!MsgQueue_is_empty(global_queue), "should have contents");
    ASSERT(MsgQueue_delete(global_queue), "delete should work");
  }

  ASSERT(!MsgQueue_delete(global_queue), "delete from empty should fail");
  ASSERT(MsgQueue_is_empty(global_queue), "should be empty");
  ASSERT(!MsgQueue_is_full(global_queue), "should not be full");

  // final test is to add and delete in sets
  int j = 0;
  for(i = 0; i < 9; i++) {
    for(j = 9; j > 0; j--) {
      msg = Message_alloc(NULL, NULL);
      if(!MsgQueue_add(global_queue, msg)) {
        Message_ref_inc(msg);
        Message_destroy(msg);
        break;
      }
    }

    for(j = i; j > 0; j--) {
      if(!MsgQueue_delete(global_queue)) break;
    }
  }

  ASSERT(!MsgQueue_is_empty(global_queue), "queue should not be empty");

  // clear it out manually
  while(!MsgQueue_is_empty(global_queue)) {
    MsgQueue_delete(global_queue);
  }

  ASSERT(!MsgQueue_delete(global_queue), "delete from empty should fail");
  ASSERT(MsgQueue_is_empty(global_queue), "should be empty");
  ASSERT(!MsgQueue_is_full(global_queue), "should not be full");

  MsgQueue_destroy(global_queue);
}


void __CUT_TAKEDOWN__MsgQueue( void ) {
  global_queue = NULL;
}
