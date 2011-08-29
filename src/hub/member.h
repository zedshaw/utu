#ifndef utu_member_h
#define utu_member_h

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

#include "protocol/peer.h"
#include "hub/queue.h"
#include "hub/heap.h"


/**
 * The data structure used internally by the Hub to keep track of everyone.
 * It is implemented as an SGLIB red-black tree that keeps track of the
 * Member's peering information, and it's GC collectable so that it gets
 * cleaned up automatically when removed from the Hub.
 */
typedef struct Member {
  bstring key;
  Peer *peer;
  MsgQueue *queue;
  void *data;

  struct Member *left;
  struct Member *right;
  int color;

  Heap *routes;
} Member;


/** Fixed internal length of the message queue.
 * TODO: make this configurable.
 */
#define MEMBER_MSG_QUEUE_LENGTH 30

/** Used by SGLIB to compare red-black tree members. */
#define MEMBER_COMPARATOR(x, y) (memcmp((x)->key->data, (y)->key->data, MIN(blength((x)->key), blength((y)->key))) + (blength((x)->key) - blength((y)->key)))

SGLIB_DEFINE_RBTREE_PROTOTYPES(Member, left, right, color, MEMBER_COMPARATOR);

/** 
 * Finds the Member in the map who has the given pubkey. 
 *
 * @param map Main member map to search in.
 * @param pubkey The key to search for.
 * @return Member structure pointer or NULL if not found.
 */
Member *Member_find(Member *map, bstring pubkey);

/** 
 * Send a message to the member, putting it on their queue. 
 *
 * @param member Who to send it to.
 * @param msg The message to send.
 * @return 1 for good, 0 for bad.
 */
int Member_send_msg(Member *member, Message *msg);

/** 
 * Get the reference to the next message for this Member. 
 * It will block the requesting task until there is a message ready to process.
 * If this returns NULL then it means that the waiting task was
 * woken up during a shutdown or delete, and should not use the member
 * further.
 *
 * @param member The member with the queue of interest.
 * @return The first msg waiting or NULL if nothing.
 */
Message *Member_first_msg(Member *member);

/** 
 * Remove the message from the Member's queue front and return 0 if there
 * are no more, 1 if there are.
 *
 * @param member The member with the queue of interest.
 * @return 0 if no more, 1 if there are.
 */
int Member_delete_msg(Member *member);

/** 
 * @brief Creates a new member not in the map yet.
 * @param peer : The peer structure for this member's connection.
 * @return A constructed member.
 */
Member *Member_create(Peer *peer);

/** 
 * Add a member to the map based on their established Utu key, but don't
 * let them login more than once.
 *
 * @param map The member map to add this new member to.
 * @param peer The peer layer they are riding on.
 * @return The Member structure created or NULL if failure.
 */
Member *Member_login(Member **map, Peer *peer);


/** 
 * Removes the given member from the map, letting the GC clean 
 * them up when ready.
 *
 * @param map The member map.
 * @param member The member to remove from the structure.
 */
void Member_logout(Member **map, Member *member);

/** 
 * Called primarily by Member_logout or other error conditions, this
 * removes cleans up the Member's stuff clearing out all the
 * pending messages as well.
 *
 * @param mb Member to destroy.
 */
void Member_destroy(Member *mb);


/**
 * Destroys an entire Member map by going through each
 * one and calling Member_destroy().
 *
 * It sets the member map to NULL when it's done.
 *
 * @param map IN/OUT The map to destroy.
 */
void Member_destroy_map(Member **map);


#define Member_name(M) ((M)->peer->state->them.name)
#endif
