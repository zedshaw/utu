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
#include "protocol/message.h"

void __CUT_BRINGUP__MessageTest( void ) {
}


void __CUT__Message_cons_decons()
{
  Node *hdr = NULL;
  Node *data = Node_cons("[bbw", bfromcstr("data1"), bfromcstr("data2"), "chat.speak");
  Node *body = Message_cons(&hdr, 4L, data, "msg");

  Message *msg1 = Message_decons(hdr, body);
  Message_ref_inc(msg1);
  ASSERT(msg1 != NULL, "failed msg1");

  if(msg1) {
    Message_dump(msg1);
    Message_destroy(msg1);
  }
}



void __CUT_TAKEDOWN__MessageTest( void ) {
}
