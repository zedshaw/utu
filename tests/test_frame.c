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
#include "protocol/peer.h"

void __CUT_BRINGUP__FrameSourceTest( void ) {
}


void __CUT__FrameSource_recv_write_msg()
{
  pool_t *pool = pool_create();
  FrameSource source;
  source.in = sbuf_create(pool, 1024);
  source.out = source.in;
  source.fd = 0;
  bstring test_header = bfromcstr("[ header\n");
  Node *test_msg = Node_cons("[w", "body");

  Node *header = NULL;
  bstring hbuf = NULL;
  int rc = 0;

  ASSERT(test_header != NULL, "failed to make test header");
  ASSERT(test_msg != NULL, "failed to parse test body");

  // write the message then read it back off
  rc = FrameSource_send(source, test_header, test_msg, 0);
  assert(rc && "failed to write message");

  // now try reading it out
  Node *msg = FrameSource_recv(source, &hbuf, &header);
  Node_dump(header, ' ', 1);
  Node_dump(msg, ' ', 1);
  ASSERT(msg != NULL, "failed to parse message body");
  ASSERT(header != NULL, "failed to parse header");
  ASSERT(hbuf != NULL, "didn't put header into bstring buffer");
  ASSERT(blength(hbuf) > 0, "hbuf didn't contain header data");

  pool_destroy(pool);
  bdestroy(hbuf);
  bdestroy(test_header);
  Node_destroy(test_msg);
  Node_destroy(msg);
  Node_destroy(header);
}



void __CUT_TAKEDOWN__FrameSourceTest( void ) {
}
