#ifndef utu_hub_routing_h
#define utu_hub_routing_h

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

#include "hub/member.h"
#include "hub/heap.h"

#define ROUTE_MAX_PATH 30

struct Route;
struct ConnectionState;

/** 
 * Called for the internal routings that the Hub uses to let people
 * perform operations on it.  The Hub is responsible for properly calling
 * this method at the right time when such a routing is found.
 */
typedef int (*Route_internal_callback)(struct ConnectionState *conn, Member *from, Node *message);

/**
 * Route implements a tree of children that make up registered paths of stackish
 * structures Members are interested in.
 *
 * All of the algorithms are implemented using recursive function calls which means
 * that there is a probability of stack smashing.  Future versions of Utu will use
 * non-recursive versions or potentially entirely different structures.
 *
 * The goal of Utu routing is to allow people to register for particular messages
 * by their shallow structure.  Members declare interest in a structure by sending
 * the Hub an abbreviated version the messages they want:
 *
 * <pre>
 *   [[ from [ to [ about chat.speak
 * </pre>
 *
 * The only elements used for the routing are words (groups with names).  Everything
 * else is skipped.  This structure is then turned into a route path using a breadth
 * first search:
 *
 * <pre>
 *   [ chat.speak, from, to, about]
 * </pre>
 *
 * This path is then registered into the Route root with no duplicates like this:
 *
 * <pre>
 *  0: chat.speak
 *   0: from
 *    0: to
 *     0: about
 * </pre>
 *
 * What you get is resolving a path later involves searching the root with a
 * series of binary searches.  At the end of this search path is the member list
 * for who should be notified (based on the registration).
 *
 * All of the memory created in the routing table are created using hmalloc library.
 * This means that when you delete a route it's children and everything it contains
 * is deleted too.  You probably should be grabbing the pointers inside because of this,
 * or plan on keeping the Route structures around.
 */
typedef struct Route {
  bstring name;

  int is_internal_callback;
  Route_internal_callback callback;

  Heap *members;
  Heap *children;
} Route;

/** 
 * You must call this to start your routing table off.
 *
 * @brief Called to create a root routing table.
 * @param name : The initial name (usually "root").
 * @return A routing table you can use with the other functions.
 */
Route *Route_create_root(const char *name);

/** 
 * You use this directly rather than Route_find and Route_add_member
 * indirectly.
 *
 * @brief Registers this member as interested in the according_to route.
 * @param routes :  Route mapping to follow.
 * @param according_to : Node structure to route messages according to.
 * @param member : Member to process for removal.
 * @return int : Whether it worked or not.
 */
int Route_register(Route *routes, Node *according_to, Member *member);

int Route_register_callback(Route *routes, Node *according_to, Route_internal_callback callback);

/** 
 * @brief Removes this member from the given according_to route.
 * @param routes :  Route mapping to follow.
 * @param according_to : Node structure to route to remove member from.
 * @param member : Member to process for removal.
 * @return int : Whether it worked or not.
 */
int Route_unregister(Route *routes, Node *according_to, Member *member);

/** 
 * @brief Removes the member from all the routes they are registered in.
 * @param routes : Route mapping to follow.
 * @param member : Member to process for removal.
 * @return int : Whether it worked or not.
 */
int Route_unregister_all(Route *routes, Member *member);


/** 
 * @brief Dumps the whole tree from parent to stderr.
 * @param parent : Root of tree.
 * @param indent : call with 1.
 */
void Route_dump(Route *parent, int indent);

/** 
 * @brief Finds the Route structure that matches this point.
 * @param parent : The root to start from.
 * @param data : The data structure that needs to be matched.
 * @return The route or NULL if not found.
 */
Route *Route_find(Route *parent, Node *according_to);

/** 
 * @brief Destroys a route and all the stuff in it.
 * @param routes : The routes tree to destroy.
 */
void Route_destroy(Route *routes);

/** 
 * @brief Adds a new member to the list of people interested.
 * @param route : Route to add to.
 * @param member : Member to add.
 */
int Route_add_member(Route *route, Member *member);

/** 
 * @brief Builds the path in the official way from the given node structure.
 * @param path : An array of Node path[max].
 * @param from : What to build the path from.
 * @param max : When to stop building a path.
 * @return size_t : Length of the final path.
 */
size_t Route_build_path(Node **path, Node *from, size_t max);


/** 
 * @brief Finds the given route matching this node element in the parent's children.
 * @param parent : Route to search.
 * @param element : Node name to search for.
 * @return the route or NULL if no match.
 */
Route *Route_find_child(Route *parent, Node *element);

#define Route_add_child(parent, element) Route_alloc((parent), (const char *)bdata((element)->name))

/** 
 * @brief Given a found route, send it to all the registered members.
 * @param route : Where to send it.
 * @param msg : Message to send.
 * @return ssize_t : -1 if there's a failure, otherwise the number of deliveries.
 */
ssize_t Route_deliver(Route *route, Message *msg);

#endif
