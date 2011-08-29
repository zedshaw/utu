#ifndef utu_message_h
#define utu_message_h

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

#include <myriad/myriad.h>
#include "stackish/node.h"

struct Member;

/**
 * An Message consists of an unencrypted (but authenticated) header that
 * indicates the circuit to send a message on, and whether there are more
 * messages that make up this message, followed by an encrypted body with a
 * root name of msg, rpy, or err.
 *
 * The has circuit and msgid that is used to keep the msg/rpy synced between
 * the two peers.  A payload group that holds the actual data for the receiver.
 * No element is optional (but can be empty).
 *
 * ORDER IS REQUIRED to keep the message format canonical.
 */
typedef struct Message {
  uint64_t msgid;
  bstring type;
  time_t received_at;
  struct Member *from;

  Node *data;

  Node *hdr;
  Node *body;

  short ref_count;
} Message;

/**
 * Used mostly internally and in testing, but it creates
 * the initial raw memory for a Message, sets ref count = 0,
 * sets the time, and attaches the hdr and body
 * params.  The hdr and body params can be NULL in this
 * function.
 *
 * You must call Message_ref_inc on what's returned.
 *
 * @brief Raw message allocation method.
 * @param hdr The header to be used later, can be NULL.
 * @param body The body to be used later, can be NULL.
 * @return Initial message.
 */
Message *Message_alloc(Node *hdr, Node *body);

/** 
 * Deconstructs Nodes into an Message suitable for delivery to a Member.
 * It's still collectable by the caller until you put it on a MsgQueue
 * for delivery.  Once that's done it will be managed by that queue.
 *
 * Once you call this function hdr and body parameters are not owned by the
 * caller but by the message.  Even if the return is NULL.  In the case
 * of NULL the hdr and body are deleted for you, since this means they
 * were mangled.
 *
 * The caller must manually increment the ref count to maintain ownership.
 *
 * @param hdr Stackish frame header 
 * @param body Stackish frame body, decrypted.
 * @return The new Message suitable for putting on a queue.
 */
Message* Message_decons(Node *hdr, Node *body);

/**
 * Constructs elements required for an Message into a header and body node
 * suitable for transmission to a remote peer (or for using to build an
 * Message if you want).
 *
 * All elements are owned by the hdr and returned body node, so simply calling
 * Node_destroy() on them should be enough.
 *
 * @param hdr OUT parameter for the constructed header you can send.
 * @param msgid Some message id the receiver knows.
 * @param data The Stackish data structure to put inside this for delivery.
 * @param type The type of the root node for this ("msg", "rpy", etc.)
 * @return The frame body you can send.
 */
Node *Message_cons(Node **hdr, uint64_t msgid, Node *data, const char *type);

/**
 * Does a simplistic dump of the message for debugging.
 *
 * @param msg The message to dump.
 */
void Message_dump(Message *msg);

/**
 * Destroys the message and ALL its attached information.  When you called
 * Message_decons the new Message owned everything, even the hdr and body.
 * This will then clean it up.
 *
 * @param msg The Message to destroy.
 */
void Message_destroy(Message *msg);

/** 
 * @brief Constructs the right stackish header.
 * @param msgid : The internal expected id of the receiver.
 * @return a Node you can pass to Peer_send.
 * @see Peer_send
 */
Node *Message_cons_header(uint64_t msgid);

#define Message_ref_inc(M) ((M)->ref_count++)
#define Message_ref_dec(M) ((M)->ref_count--)
#endif
