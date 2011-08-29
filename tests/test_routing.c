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
#include "cut.h"
#include "hub/routing.h"


void __CUT_BRINGUP__RoutingTest( void ) 
{
}

void __CUT__Routing_operations()
{
  Route *routes = Route_create_root("root");
  bstring tests[5];
  size_t i = 0;
  Member *member = calloc(1, sizeof(Member));
  member->routes = Heap_create(NULL);
  Node *node = NULL;
  int rc = 0;

  tests[0] = bfromcstr("[ [ \"now\" when shutdown ");
  tests[1] = bfromcstr("[ [ [ [ test ] @attr from [ 2 to [ \"stuff\" about chat.speak ");
  tests[2] = bfromcstr("[ [ 2 to [ \"stuff\" about chat.speak ");
  tests[3] = bfromcstr("[ [ apples [ oranges [ bananas chat.speak ");
  tests[4] = bfromcstr("[ chat.speak ");

  for(i = 0; i < 5; i++) {
    node = Node_parse(tests[i]);
    dbg("REGISTERING:");
    Node_dump(node, ' ', 1);
    ASSERT(node != NULL, "node failure");

    rc = Route_register(routes, node, member);
    ASSERT(rc, "failed to register");
    dbg("TREE after register: %s", bdata(tests[i]));
    Route_dump(routes, 1);

    Route *route = Route_find(routes, node);
    ASSERT(route != NULL, "failed to find it again");
    ASSERT_EQUALS(Heap_count(route->members), 1, "failed to add member");
    ASSERT_EQUALS(Heap_count(member->routes), i+1, "failed to add member");

    Node_destroy(node);
  }

  // don't do last one to test destroy
  for(i = 0; i < 4; i++) {
    node = Node_parse(tests[i]);
    ASSERT(node != NULL, "node failure");

    rc = Route_unregister(routes, node, member);
    ASSERT(rc, "failed to unregister");

    dbg("TREE after unregister: %s", bdata(tests[i]));
    Route_dump(routes, 1);

    Route *route = Route_find(routes, node);
    // remember, routes don't go away, just the members diminish
    ASSERT(route != NULL, "failed to find it again");
    ASSERT_EQUALS(Heap_count(route->members), 0, "failed to add member");

    Node_destroy(node);
    bdestroy(tests[i]);
  }

  // one more for validation purposes
  node = Node_parse(tests[i]);
  rc = Route_register(routes, node, member);
  Route_unregister_all(routes, member);
  Node_destroy(node);

  // should be one last one still
  Heap_destroy(member->routes);
  free(member);
  bdestroy(tests[4]);
  Route_destroy(routes);
}



void __CUT_TAKEDOWN__RoutingTest( void ) 
{
}

