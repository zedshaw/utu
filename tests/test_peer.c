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

static int key_was_confirmed;

int key_confirm_test(CryptState *state)
{
  key_was_confirmed = 1;
  return 1;
}

void __CUT_BRINGUP__PeerTest( void ) 
{
  key_was_confirmed = 0;
  CryptState_init();
}

void __CUT__Peer_initialize_destroy()
{
  bstring name = bfromcstr("zed");
  CryptState *crypt = CryptState_create(name, NULL);
  ASSERT(crypt != NULL, "failed creating crypt state");

  Peer *peer = Peer_create(crypt, 0, key_confirm_test);
  ASSERT(peer != NULL, "failed creating peer");
  
  Peer_destroy(peer, 1);
  bdestroy(name);
}


void __CUT_TAKEDOWN__PeerTest( void ) {

}
