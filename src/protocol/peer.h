#ifndef utu_peer_h
#define utu_peer_h

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



#include "crypto.h"
#include "frame.h"

#define PEER_DEFAULT_IO_BUF_SIZE (32 * 1024)

/**
 * Controls the communications between two peers
 * in a communication setting.  It is primarily
 * used by the Hub to send/recv communications from
 * itself and others, but it can also be used directly
 * if you want to layer communications (you'd implement
 * that though).
 */
typedef struct Peer {
  CryptState *state;
  pool_t *pool;
  FrameSource source;
  CryptState_key_confirm_cb key_confirm;
} Peer;


/** 
 * Creates an Peer with the given initialized state and target socket
 * to use for Framing.   You should pass in a function that confirms
 * the key with the user or in a database of known keys.
 *
 * @param state A constructed and ready to use CryptState.
 * @param fd A ready to use socket.
 * @param key_confirm Called by the crypto system to get a key confirm before continuing.
 * @return A ready to use Peer.
 */
Peer *Peer_create(CryptState *state, int fd, CryptState_key_confirm_cb key_confirm);


/**
 * Destroys the Peer, if crypt_too is 1 then it'll 
 * also call CryptState_destroy on the internal CryptState.
 * Sometimes you don't want this destroyed, especially if
 * you're reusing them for later.
 *
 * @param peer The peer to get rid of.
 * @param crypt_too Whether to destroy the enclosed crypto as well.
 */
void Peer_destroy(Peer *peer, int crypt_too);


/**
 * The Peer API's job is to encode/decode encrypted
 * messages of the most basic type using crypto and frame
 * APIs.  What is does is call Framing_recv to get a 
 * message, then it decrypts it to create the rhdr out
 * parameter and returned Node body.
 *
 * @param peer The peer to recv from.
 * @param rhdr OUT parameter that will have the header to send.
 * @return NULL on failure and sets rhdr to NULL, otherwise decrypted body.
 * @see FrameSource for how this stuff is framed.
 * @see CryptState for how it's decrypted.
 */
Node *Peer_recv(Peer *peer, Node **rhdr);


/**
 * The inverse of Peer_recv() this takes a header
 * and payload and sends it.  The header is *not* encrypted
 * but the payload is.  The header is included in the AAD
 * so if it is tampered with then the receiver will know.
 *
 * @param peer The peer to send to.
 * @param header The header node to send.
 * @param payload The unencrypted payload to send (will be encrypted).
 * @return 0 on failure and 1 on success.
 */
int Peer_send(Peer *peer, Node *header, Node *payload);


/**
 * Establishes the communications for an initiator.
 * The peer takes over the socket communication but
 * uses the Myriad Task functions so you can have
 * other stuff happen at the same time.
 *
 * TODO: Gotta allow for a callback to let them confirm the
 * key.
 *
 * @param peer Who to establish with as an initiator.
 * @return 1 if there's no problems establishing, an 0 if anything doesn't work cryptographically.
 *
 */
int Peer_establish_initiator(Peer *peer);

/**
 * The analog to the Peer_establish_initiator() function,
 * this is used by the receiver to setup the communication
 * and confirms their communication.
 *
 * TODO: Also needs a confirm key callback.
 *
 * @param peer Who to establish with as a receiver.
 * @return 1 if there's no problems and 0 if there's failure just like the initiator function.
 *
 */
int Peer_establish_receiver(Peer *peer);

#endif
