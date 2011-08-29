#ifndef utu_hub_heap_h
#define utu_hub_heap_h

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


#include <stdlib.h>

struct Heap;
typedef int (*Heap_compare_fn)(struct Heap *heap, void *x, void *y);

typedef struct Heap {
  size_t last;
  size_t size;
  void **members;
  Heap_compare_fn compare;
} Heap;

#define HEAP_SIZE 8

/** Creates a heap with an initial size of HEAP_SIZE.  If compare is NULL
 * then the heap uses the raw pointers for its comparison.  This is useful
 * for just finding things by their pointers, but if you want to find by
 * some element then register a different comparison function.
 */
Heap *Heap_create(Heap_compare_fn compare);

/** Resize a heap either up or down in size, size cannot be 0. */
void Heap_resize(Heap *heap, size_t size);

/** Gets the number of filled elements (it's count). */
#define Heap_count(heap) ((heap)->last)

/** Tells you if the Heap is empty or not. */
#define Heap_is_empty(heap) (Heap_count(heap) == 0)

/** Tells you if the Heap is full. */
#define Heap_is_full(heap) ((heap)->last == (heap)->size)

/** Used to get an element from the heap at a certain index of a certain type. */
#define Heap_elem(heap, type, at) ((type)(heap)->members[(at)])

/** Gets the first element of a certain type. */
#define Heap_first(heap,type) Heap_elem(heap,type,0)

/** Gets the last element of a certain type. */
#define Heap_last(heap,type) Heap_elem(heap,type,(heap)->last-1)

/** Tells you if this index is valid (used after Heap_find). */
#define Heap_valid(heap, at) ((at) < (heap)->last)


/** 
 * Adds an element to the heap, returning its location.  It
 * will dynamically resize the allocated internal array if it
 * needs.
 */
size_t Heap_add(Heap *heap, void *elem);

/**
 * Deletes the element from the heap, returning 0 if it wasn't in 
 * there or 1 if it was.  It will reduce the size of the heap if
 * there's a bunch of wasted space.
 */
int Heap_delete(Heap *heap, void *elem);

/** Destroys the whole heap. */
void Heap_destroy(Heap *heap);

/** Clears everything out of the heap by just setting last = 0. */
#define Heap_clear(heap) ((heap)->last = 0)

/** 
 * Finds the element in the Heap returning the location.  If it returns
 * something that is !Heap_valid(heap, at) then it wasn't found.  This
 * is also where a new element can be inserted.
 */
size_t Heap_find(Heap *heap, void *elem);

/**
 * Macro to make iterating through the current entries in a heap easier.
 * It will create a local variable named after var of the type so that you
 * don't need to cast var in the command.  While it's running there
 * will be a local variable named after the indx parameter and then the
 * var parameter.
 */
#define HEAP_ITERATE(heap, indx, type, var, command)  { size_t indx = 0; for((indx) = 0; (indx) < (heap)->last; (indx)++) {\
  type var = (heap)->members[indx];\
  { command; }\
  } }

#endif
