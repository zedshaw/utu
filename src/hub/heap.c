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
 *
 * Code some code taken from the fantastic SGLIB by Marian Vittek and available
 * as LGPL from  http://www.xref-tech.com/sglib.
 */

#include "heap.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <myriad/defend.h>
#include <myriad/sglib.h>

inline int Heap_void_ptr_compare(Heap *heap, void *x, void *y)
{
  return (long)x - (long)y; 
}

Heap *Heap_create(Heap_compare_fn compare)
{
  Heap *heap = calloc(1, sizeof(Heap));
  assert(heap && "Failed to allocate heap.");

  heap->members = calloc(HEAP_SIZE, sizeof(void *));
  assert(heap->members && "Failed to allocate members for heap.");
  heap->compare = compare == NULL ? Heap_void_ptr_compare : compare;

  heap->size = HEAP_SIZE;

  return heap;
}

#define HEAP_COMPARE(x,y) (heap->compare(heap, (x), (y)))

void Heap_sort(Heap *heap) {
  SGLIB_ARRAY_HEAP_SORT(void *, heap->members, (int)heap->last, HEAP_COMPARE, SGLIB_ARRAY_ELEMENTS_EXCHANGER);
}

size_t Heap_add(Heap *heap, void *elem)
{
  assert(heap && "heap is NULL");
  assert(elem && "elem is NULL");

  if(Heap_is_full(heap)) {
    // simple doubling is probably bad
    Heap_resize(heap, heap->size * 2);
  }

  heap->members[heap->last] = elem;
  heap->last++;
  // couldn't find an efficient insert algorithm that was reliable so i just sort each time
  Heap_sort(heap);

  return heap->last;
}

void Heap_resize(Heap *heap, size_t size)
{
  assert(size > 0 && "Cannot resize to 0.");

  heap->members = realloc(heap->members, size * sizeof(void *));
  assert(heap->members != NULL && "Failed to realloc new Heap.");

  // now we have to clear the tail end since realloc doesn't do that
  if(size > heap->size) {
    size_t diff = (size < heap->size ? heap->size - size : size - heap->size);
    memset(heap->members + heap->size, 0, diff * sizeof(void *));
  }

  heap->size = size;
}

size_t Heap_find(Heap *heap, void *elem)
{
  size_t result_index = 0;
  register int kk = 0, cc = 0, ii= 0, jj = heap->last-1, ff=0;

  assert(heap && "heap is NULL");
  assert(elem && "elem is NULL");

  while (ii <= jj && ff==0) {
    kk = (jj+ii)/2;
    cc = heap->compare(heap, heap->members[kk], elem);
    if (cc == 0) {
      result_index = kk;    
      ff = 1;
    } else if (cc < 0) {
      ii = kk+1;
    } else {
      jj = kk-1;
    }
  }
  if (ff == 0) {
    // not found, return heap->last to indicate that it's outside realm
    return heap->last;
  }

  return result_index;
}

int Heap_delete(Heap *heap, void *elem)
{
  assert(heap && "heap is NULL");
  assert(elem && "elem is NULL");

  size_t i = Heap_find(heap, elem);

  if(Heap_valid(heap, i)) {
    // simplest way to delete is just move the rest down by one
    memmove(heap->members + i, heap->members + i + 1, (heap->last - i - 1) * (sizeof(void *)));

    heap->last--;
    // just make sure the last one is null always
    heap->members[heap->last] = NULL;

    // don't bother dropping the size below HEAP_SIZE
    if(heap->last > HEAP_SIZE && heap->last < heap->size / 2) {
      // it's dropped to less than half the size, cut it down
      Heap_resize(heap, heap->size / 2);
    }

    return 1;
  } else {
    return 0;
  }
}

void Heap_destroy(Heap *heap)
{
  assert(heap && heap->members && "invalid heap destroyed");
  free(heap->members);
  heap->members = NULL;
  free(heap);
}

