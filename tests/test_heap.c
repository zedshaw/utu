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
#include "hub/heap.h"
#include <myriad/defend.h>
#include <myriad/bstring/bstrlib.h>


void __CUT_BRINGUP__HeapTest( void ) {
}

int heap_bstring_cmp(Heap *heap, void *x, void *y)
{
  int i = bstrcmp((bstring)x, (bstring)y);
  return i;
}

void __CUT__Heap_bstring_operations() 
{
  Heap *heap = Heap_create(heap_bstring_cmp);
  ASSERT(heap != NULL, "failed to make heap");
  bstring t[8] = {0};
  int test = 0;

  t[0] = bfromcstr("zzzz1");
  t[1] = bfromcstr("zzzz2");
  t[2] = bfromcstr("bbbbb");
  t[3] = bfromcstr("aaaaa");
  t[4] = bfromcstr("yaaaa");
  t[5] = bfromcstr("xaaaa");
  t[6] = bfromcstr("uaaaa");
  
  for(test = 0; t[test] != NULL; test++) {
    ASSERT(Heap_add(heap, t[test]), "add bstring failed");
  }

  Heap_destroy(heap);

  for(test = 0; t[test] != NULL; test++) {
    bdestroy(t[test]);
  }
}

void __CUT__Heap_void_ptr_operations() 
{
  Heap *heap = Heap_create(NULL);
  ASSERT(heap != NULL, "failed to make heap");

  ASSERT(Heap_add(heap, heap), "add failed");
  ASSERT(Heap_add(heap, heap), "add failed");
  ASSERT(Heap_add(heap, heap), "add failed");
  ASSERT(Heap_elem(heap, char *, 0) != NULL, "first was null");
  ASSERT(Heap_last(heap, char *) != NULL, "last was null");
  ASSERT(Heap_count(heap) == 3, "wrong count");

  HEAP_ITERATE(heap, indx, Heap *, elem, ASSERT(elem == heap, "heap not in the heap"));

  size_t f = Heap_find(heap, heap);
  ASSERT(Heap_valid(heap, f), "didn't find the heap");

  f = Heap_find(heap, (void *)0x84);
  ASSERT_EQUALS(f, Heap_count(heap), "wrong not found index");
  ASSERT(!Heap_valid(heap, f), "found but shouldn't");

  ASSERT(Heap_delete(heap, heap), "delete failed");
  ASSERT(Heap_delete(heap, heap), "delete failed");
  ASSERT(Heap_delete(heap, heap), "delete failed");
  ASSERT(Heap_count(heap) == 0, "wrong count");
  ASSERT(Heap_delete(heap, heap) == 0, "delete should be 0");

  ASSERT(heap->last == 0, "last should be 0");
  ASSERT(Heap_delete(heap, (void *)0x84) == 0, "delete should fail");

  memset(heap->members, 0, heap->last);

  for(f = 1; f < 1000; f++) {
    Heap_add(heap, (void *)f);
    assert(Heap_valid(heap, Heap_find(heap, (void *)f)) && "can't find after add");
  }

  ASSERT(Heap_count(heap) == 1000-1, "heap not right size");

  void *last = NULL;
  HEAP_ITERATE(heap, indx, Heap *, elem, ASSERT((size_t)last == (size_t)elem-1, "heap not ordered"); last = elem);

  f = 1;
  for(f = 1; f < 500; f++) {
    if(!Heap_delete(heap, (void *)f)) {
      HEAP_ITERATE(heap, indx, void *, elem, dbg("%zu: %p", indx, elem));
      assert(0 && "failed to delete");
    }
  }

  ASSERT_EQUALS(Heap_count(heap), 500, "wrong size after big delete");
  ASSERT(!Heap_is_empty(heap), "heap shouldn't be empty");
  ASSERT(!Heap_is_full(heap), "heap shouldn't be full (ever really)");

  last = NULL;
  HEAP_ITERATE(heap, indx, Heap *, elem, ASSERT((size_t)last < (size_t)elem, "heap not ordered"); last = elem);

  for(f = 500; f < 1000; f++) ASSERT(Heap_delete(heap, (void *)f), "not found in last delete");
  ASSERT_EQUALS(Heap_count(heap), 0, "heap not empty");
  ASSERT(Heap_is_empty(heap), "heap not empty");

  Heap_destroy(heap);
}

void __CUT_TAKEDOWN__HeapTest( void ) 
{
}
