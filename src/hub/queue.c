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


#include "queue.h"

MsgQueue *MsgQueue_create(size_t dim)
{
  MsgQueue *queue = calloc(1, sizeof(MsgQueue));
  assert_mem(queue);

  assert_not(dim, 0);

  queue->dim = dim;

  queue->messages = malloc(dim * sizeof(Message *));
  assert_mem(queue->messages);
  return queue;
}

int MsgQueue_add(MsgQueue *q, Message *message)
{
  assert_not(q, NULL);
  assert_not(message, NULL);
  if(MsgQueue_is_full(q)) {
    return 0;
  } else {
    Message_ref_inc(message);
    q->messages[q->j] = message;
    q->j = (q->j + 1) % q->dim;
    return 1;
  }
}

int MsgQueue_delete(MsgQueue *q)
{
  assert_not(q, NULL);

  if(MsgQueue_is_empty(q)) {
    return 0;
  } else if(q->messages[q->i]) {
    Message_destroy(q->messages[q->i]);
    q->messages[q->i] = NULL;
    q->i = (q->i + 1) % q->dim;
    return 1;
  } else {
    dbg("delete called, queue said not empty, but j=%zu with i=%zu was NULL", q->j, q->i);
    return 0;
  }
}

void MsgQueue_destroy(MsgQueue *q)
{
  assert_not(q, NULL);

  if(q->messages) {
    // go through all of them and do a delete so they dec ref count or are deleted
    while(!MsgQueue_is_empty(q)) MsgQueue_delete(q);
    free(q->messages);
    q->messages = NULL;
  }

  free(q);
}


Message *MsgQueue_first(MsgQueue *queue)
{
  assert_not(queue, NULL);

  if(MsgQueue_is_dead(queue)) return NULL;

  if(MsgQueue_is_empty(queue)) {
    MsgQueue_wait(queue);

    // if, when we wake up from this, the member is dead then we skip it
    if(queue->dead || MsgQueue_is_empty(queue)) {
      return NULL;
    }
  }

  return MsgQueue_get_first(queue);
}
