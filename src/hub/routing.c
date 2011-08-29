#include "hub/routing.h"
#include <myriad/pool/halloc.h>
#include <myriad/bstring/bstrlib.h>

int Route_children_compare(Heap *heap, void *x, void *y)
{
  return bstrcmp(((Route *)x)->name, ((Route *)y)->name);
}

Route *Route_alloc(Route *parent, const char *name)
{
  Route *r = h_calloc(1, sizeof(Route));
  assert_mem(r);

  r->name = bfromcstr(name);
  assert_mem(r->name);

  r->members = Heap_create(NULL);
  r->children = Heap_create(Route_children_compare);

  // add it to the children mapping
  if(parent) {
    hattach(r, parent);
    Heap_add(parent->children, r);
  }

  return r;
}

Route *Route_create_root(const char *name)
{
  return Route_alloc(NULL, name);
}


void Route_dump(Route *parent, int indent)
{
  int i = 0;
  int x = 0;

  if(indent == 1) {
    fprintf(stderr, "ELEMENTS in parent '%s'\n", bdata(parent->name));
  }

  HEAP_ITERATE(parent->children, indx, Route *, r, 
    for(x = 0; x < indent; x++) fprintf(stderr, " ");
    fprintf(stderr, "(%p) %d: %s [%zu]\n", r, i++, bdata(r->name), Heap_count(r->members));
    Route_dump(r, indent+1);
  );
}

Route *Route_find_child(Route *parent, Node *element)
{
  if(parent == NULL || element == NULL) {
    return NULL;
  }

  Route child = {.name = element->name};
  size_t i = Heap_find(parent->children, &child);
 
  if(Heap_valid(parent->children, i)) {
    return Heap_elem(parent->children, Route *, i); 
  } else {
    return NULL;
  }
}

Route *Route_add(Route *parent, Node **path, size_t remaining)
{

  Node *element = path[0];
  Route *r = NULL;

  // abort with an error if the path runs out incorrectly
  if(element == NULL) return NULL;

  if((r = Route_find_child(parent, element)) == NULL) {
    // this child doesn't exist, add it
    r = Route_add_child(parent, element);
  }

  assert_not(r, NULL);

  if(remaining-1 > 0) {
    // not the last path, keep adding
    return Route_add(r, path+1, remaining-1);
  } else {
    // last one we added, they should use this one
    return r;
  }
}

int Route_add_member(Route *route, Member *member)
{ 
  assert_not(route, NULL);

  if(route->is_internal_callback) {
    log(ERROR, "Member %s attempted to register for %s route that's internal.", 
        bdata(Member_name(member)), bdata(route->name));
    return 0;
  }

  Heap_add(route->members, member);
  Heap_add(member->routes, route);

  return 1;
}

Route *Route_extend_and_find(Route *routes, Node *according_to)
{
  assert_not(routes, NULL);
  assert_not(according_to, NULL);

  size_t path_i = 0;
  Node *path[ROUTE_MAX_PATH] = {NULL};

  path[path_i++] = according_to;

  SGLIB_LIST_MAP_ON_ELEMENTS(Node, according_to->child, n, sibling,
      check(path_i < ROUTE_MAX_PATH, "path too long");
      if(n->type == TYPE_GROUP && n->name && bchar(n->name,0) != '@') path[path_i++] = n);

  Route *point = Route_add(routes, path, path_i);

  return point;
  on_fail(return NULL);
}

int Route_register(Route *routes, Node *according_to, Member *member)
{
  Route *point = Route_extend_and_find(routes, according_to);
  check(point, "Invalid Routing structure.");
  check(Route_add_member(point, member), "Failed to add member to requested routing.");
  return 1;
  on_fail(return 0);
}

int Route_register_callback(Route *routes, Node *according_to, Route_internal_callback callback)
{
  Route *point = Route_extend_and_find(routes, according_to);
  check(point, "Invalid Routing structure.");

  point->is_internal_callback = 1;
  point->callback = callback;

  return 1;
  on_fail(return 0);
}

int Route_unregister(Route *routes, Node *according_to, Member *member)
{
  assert_not(routes, NULL);
  assert_not(according_to, NULL);

  Route *r = Route_find(routes, according_to);

  if(r != NULL) {
    Heap_delete(r->members, member);
    Heap_delete(member->routes, r);
    return 1;
  } else {
    // didn't find them, goodbye
    return 0;
  }
}

int Route_unregister_all(Route *routes, Member *member)
{
  assert_not(routes, NULL);
  assert_not(member, NULL);

  HEAP_ITERATE(member->routes, i, Route *, r, Heap_delete(r->members, member));
  Heap_clear(member->routes);

  return 1;
}

Route *Route_find(Route *parent, Node *according_to)
{
  size_t i = 0;
  Route *r = Route_find_child(parent, according_to);

  if(r == NULL) return NULL; // don't bother if root isn't even found

  // go through the root node's children
  SGLIB_LIST_MAP_ON_ELEMENTS(Node, according_to->child, n, sibling,
      check(i++ < ROUTE_MAX_PATH, "Routing path too long.");
      if(n->type == TYPE_GROUP && n->name && bchar(n->name,0) != '@') r = Route_find_child(r, n);
      if(r == NULL) return NULL);

  return r;
  on_fail(return NULL);
}

inline void Route_destroy_children(Route *routes)
{
  if(routes->children) {
    HEAP_ITERATE(routes->children, indx, Route *, r, 
      bdestroy(r->name);
      Heap_destroy(r->members);
      Route_destroy_children(r);
      Heap_destroy(r->children);
    );
  }
}

void Route_destroy(Route *routes)
{
  Route_destroy_children(routes);
  bdestroy(routes->name);
  Heap_destroy(routes->members);
  Heap_destroy(routes->children);
  h_free(routes);
}


ssize_t Route_deliver(Route *route, Message *msg)
{
  ssize_t count = 0;

  assert_not(route, NULL);
  assert_not(msg, NULL);

  HEAP_ITERATE(route->members, i, Member *, m, 
      check(Member_send_msg(m, msg), "delivery failed"); count += 1);

  return count;
  on_fail(return -1);
}

