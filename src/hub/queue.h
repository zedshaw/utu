#ifndef utu_queue_h
#define utu_queue_h

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



#include <myriad/sglib.h>
#include "protocol/message.h"

/** 
 * Implements a simple message queue structure for Messages.  It is just a bare
 * Queue (actually more of a ring buffer) that cycles through a fixed length
 * array of pointers to pending Messages.  Messages are not cleaned up
 * immediately after being taken off the queue, but are left to the GC to
 * handle.
 *
 * When you use the structure you leave the first message on the queue while
 * you work with it, then when you are done using it you call
 * MsgQueue_delete() to free that spot and start working on the next
 * message.
 *
 * The Queue really only works with a single consumer and multiple producers,
 * but if coordinated correctly it could allow for multiple consumers to
 * recieve interleaved messages (but maybe not the same message).
 *
 * One trick though is you can declare one consumer the "killer" and all the
 * other consumers regular.  The killer is the only one that calls
 * Queue_delete, and since the GC is used to clean up, each regular consumer
 * can just temporarily retain, or use GC pausing push/pop to keep the message
 * around.  This is probably way more complex than needed though for most
 * usages.
 */

typedef struct MsgQueue {
  Message **messages;
  Rendez read_wait;
  int dead;

  size_t i;
  size_t j;
  size_t dim;
} MsgQueue;

/** 
 * Creates a GC managed MsgQueue ready for use with the allowed
 * length. You need to either gc_retain() the returned value or attach
 * it to another so it gets marked properly.
 *
 * You actually only get length-1 messages allowed in the queue, with one
 * record used as an "overlap" sentinel.
 *
 * @param length The queue length (-1).
 * @return The new queue, or NULL if failed.
 */
MsgQueue *MsgQueue_create(size_t length);

/** Tells you if the MsgQueue is empty. */
#define MsgQueue_is_empty(Q) ((Q)->i == (Q)->j)

/** Tells you if the MsgQueue is full. */
#define MsgQueue_is_full(Q) (((Q)->i)==(((Q)->j)+1)%((Q)->dim))

/** Returns a pointer to the first message ready in the queue.  Does not remove it.*/
#define MsgQueue_get_first(Q) ((Q)->messages[(Q)->i])

/** Blocks until a message is available or the queue is marked dead. */
Message *MsgQueue_first(MsgQueue *queue);

/** Puts a new message on the end of the queue.
 *
 * @param q The queue to add the message to.
 * @param message The message to add.
 * @return returning 1 if it worked and 0 if not. 
 */
int MsgQueue_add(MsgQueue *q, Message *message);

/** 
 * Removes the first message from the queue so you can use MsgQueue_first() to 
 * get the next message.  
 *
 * @param q The queue to delete from.
 * @return 1 if there are still messages and 0 if not.
 */
int MsgQueue_delete(MsgQueue *q);

/** Marks the queue dead so that processors will stop working on it. */
#define MsgQueue_mark_dead(Q) (Q)->dead = 1

/** Determines if the queue is actually dead. */
#define MsgQueue_is_dead(Q) (Q)->dead

#define MsgQueue_wake_all(Q) taskwakeupall(&((Q)->read_wait))

#define MsgQueue_wait(Q) tasksleep(&(Q)->read_wait)

/** 
 * Destroys the queue and all the messages that are pending in it.
 *
 * @param q The queue to destroy.
 */
void MsgQueue_destroy(MsgQueue *q);

/** Clears the queue by simply setting the indices to the front. */
#define MsgQueue_clear(Q) if(Q) (Q)->i = (Q)->j = 0;

#endif
