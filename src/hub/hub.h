#ifndef utu_hub_h
#define utu_hub_h

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


#include "hub/routing.h"

#define HUB_DEFAULT_STACK (32*1024)

struct Hub;

/**
 * Events that are valid for both Hub and ConnectionState
 * Ragel state machines.  The settings in this enum have to
 * match the defined events in the hub.rl and connection.rl
 * files or it won't work.
 */
typedef enum HubEvent { 
  UEv_OPEN='O',
  UEv_CLOSE='C',
  UEv_KEY_PRESENT='P',
  UEv_PASS='1',
  UEv_FAIL='0',
  UEv_SVC_RECV='s',
  UEv_MSG_SENT='e',
  UEv_MSG_QUEUED='Q',
  UEv_MSG_RECV='R',
  UEv_HATE_APPLY='H',
  UEv_HATE_CHALLENGE='h',
  UEv_HATE_PAID='b',
  UEv_HATE_VALID='V',
  UEv_HATE_INVALID='v',
  UEv_LISTEN='L',
  UEv_GEN_KEY='K',
  UEv_GEN_KEY_DONE='k',
  UEv_KEY_FILE_LOAD='F',
  UEv_DONE='D',
  UEv_READ_CLOSE='r',
  UEv_WRITE_CLOSE='w'
} HubEvent;

/** 
 * ConnectionState manages the state for each connection to the Hub and is
 * mostly an internal datastructure.  It encapsulates the main logic for Utu
 * using a Ragel statechart style statemachine that maintains the state for
 * connected peers.
 *
 * The Hub keeps two lists of ConnectionState objects that control which
 * connections are newly formed (opened) and which are closed and can be reused.
 * Each ConnectionState can only be exclusively in the Hub.opened or
 * Hub.closed lists but not both.
 *
 * \image html connection.png
 *  
 * As the Hub processes connections it creates ConnectionState objects and
 * hands them to two tasks to manage incoming and outgoing processing.  The
 * tasks are responsible for managing the Hub.opened and Hub.closed lists to
 * communicate with the hub the state of connections they're managing.  The
 * task is responsible for finalizing the ConnectionState before giving it back
 * to the Hub.
 *
 * To reduce the memory cycling a Hub will try to reuse a pending closed
 * ConnectionState before making a new one.
 *
 * @brief The state machine for connections.
 * @see Hub
 * @see ConnectionState_init
 */
typedef struct ConnectionState {
  int cs;
  uint64_t recv_count;
  uint64_t send_count;
  MyriadClient *client;
  struct Hub *hub;
  struct ConnectionState *next;
  Peer *peer;
  Member *member;
  QLock lock;

  /** Passes information about an active message to the machine. */
  struct {
    Node *body;
    Node *hdr;
    Message *msg;
  } recv;

  /** Passes information about a message being sent to the machine. */
  struct {
    Message *msg;
  } send;

} ConnectionState;

/** 
 * Initializes the ConnectionState machine used to coordinate
 * connections properly.
 *
 * @brief Initializes an inactive state for use.
 * @param state A new initialized state
 * @return 0 for fail, 1 for good.
 */
int ConnectionState_init(ConnectionState *state);

/**
 * Executes this one event on the state machine.
 *
 * @brief Executes the HubEvent.
 * @param state A properly initialized state.
 * @param event A UtutHubEvent that the machine should process.
 * @return 0 for more, 1 for finished, -1 for failure.
 */
int ConnectionState_exec(ConnectionState *state, HubEvent event);

/**
 * Finalizes a ConnectionState if it needs to be finalized.
 * This runs all the shutdown states and properly exits the
 * machine.  If the machine wasn't ready to exit then you get
 * an error.
 *
 * @brief Finalizes the state.
 * @param state A properly initialized state.
 * @return 0 if not done, 1 if finalized, -1 if there was an error.
 */
int ConnectionState_finish(ConnectionState *state);


/** 
 * Used to cleanup a ConnectionState no matter what.  It calls
 * ConnectionState_finish(state) before destroy it.
 *
 * @brief Destroys and finalizes a ConnectionState
 * @param state : what to destroy
 */
void ConnectionState_destroy(ConnectionState *state);


/** 
 * @brief Determines if the connection is in error or final states.
 * @param state : state to process.
 * @return int : 0 == yes, 1 == no.
 */
int ConnectionState_done(ConnectionState *state);

/** Locks the machine so you can do mulitple operations without interference. */
#define ConnectionState_lock(C) qlock(&(C)->lock);

/** Unlocks. */
#define ConnectionState_unlock(C)  qunlock(&(C)->lock); taskyield()

/** 
 * @brief Takes a message from this members queue.
 * @param conn : state in process
 * @return int : 1 for success, 0 for fail
 */
int ConnectionState_dequeue_msg(ConnectionState *conn);

/** 
 * @brief Writes the message on the wire.
 * @param conn : state to use
 * @return int : 1 for success, 0 for fail
 */
int ConnectionState_send_msg(ConnectionState *conn);

/** 
 * @brief Called by the crypto.c to confirm if this key is valid.
 * @param state : CryptState (not ConnectionState!) to use.
 * @return int : 1 for valid key, 0 for invalid
 */
int ConnectionState_confirm_key(CryptState *state);

/** 
 * @brief MyriadClient handler that processes outgoing messages.
 * @param data : myriad data (ConnectionState)
 */
void ConnectionState_outgoing(void *data);

/** 
 * @brief Reads a message off the wire.
 * @param conn : state to process.
 * @return int : 1 for success, 0 for fail
 */
int ConnectionState_read_msg(ConnectionState *conn);

/** 
 * @brief MyriadClient handler that reads incoming messages and data.
 * @param data : myriad data (ConnectionState)
 */
void ConnectionState_incoming(void *data);


/** 
 * This is used to close one half of the connection so that the reader/writer
 * pair of tasks can coordinate the graceful shutdown.  It is called by the
 * error, half_closed actions.
 *
 * @brief Closes one half of the connection.
 * @param state : ConnectionState to process.
 * @return int : 0 means it's already been closed, 1 means it worked.
 */
int ConnectionState_half_close(ConnectionState *state);


/** 
 * Called after the half_close operation this will do the final cleanup.
 *
 * @brief Closes the rest of the connection and finally exits.
 * @param state : ConnectionState to process.
 * @return int : 0 means it's already been closed, 1 means it worked.
 */
int ConnectionState_rest_close(ConnectionState *state);

/**
 * The main structure that controls an Utu server.  The hub
 * is created to listen on a given port and then uses ConnectionState
 * structures to track all of the connected members.
 *
 * \image html hub.png
 */
typedef struct Hub
{
  int cs;
  bstring host;
  bstring port;
  bstring name;
  bstring key;
  MyriadServer *server;
  Member *members;
  ConnectionState *closed;
  Route *routes;
} Hub;


/** 
 * Sets up the internal global variables and other
 * one time initializations needed for creating
 * Hubs.  Must be the first function called.
 *
 * @brief Called once to initialize the underlying systems.
 * @param argv_0 The name of the program for diagnostic purposes
 * @return 0 for failure or 1 for success
 */
int Hub_init(char *argv_0);

/**
 * Starts a receiver listening in on the host:port given with the name for 
 * cryptography situations.  The key can be NULL and it'll be generated. 
 * There's only one receiver for each process (you only call this once).
 *
 * @brief Creates a hub to accept clients.
 * @param host The host to bind to, usually 0.0.0.0
 * @param port Port to bind and listen on.
 * @param name This server's Utu name.
 * @param key The server's private key (not given out).
 * @return A hub ready to be configured and run with Hub_start
 * @see Hub_start
 * @see Hub_destroy
 */
Hub *Hub_create(bstring host, bstring port, bstring name, bstring key);

/**
 * Starts a created hub so that it begins listening on the port
 * and processing clients.
 *
 * @brief Start listening on the socket.
 * @param hub A hub created.
 * @see Hub_create
 * @see Hub_destroy
 */
void Hub_listen(Hub *hub);

/**
 * Executes this one event on the state machine.
 *
 * @brief Runs an HubEvent.
 * @param state A properly initialized state.
 * @param event An HubEvent the machine should process.
 * @return 0 for more, 1 for finished, -1 for failure.
 * @see HubEvent
 */
int Hub_exec(Hub *state, HubEvent event);

/**
 * Shutsdown the hub and destroys anything in memory.  It's violent so doesn't wait
 * for connected clients to exit.
 *
 * @brief Destroys the hub.
 * @param hub What to destroy.
 */
int Hub_destroy(Hub *hub);


/** 
 * Uses a list of recently closed and recently opened clients to
 * reuse memory and then pass this client to the Hub.  It 
 * spawns the hub using a UEv_OPEN event and then eventually returns.
 *
 * The unusual thing about this function is that the process of
 * queuing a client connection starts the ConnectionState to operate,
 * so that the current task gets taken over.
 *
 * @brief Puts this client on the hub's queue of newly opened clients.
 * @param hub : hub that should process this client.
 * @param client : client connection to process.
 */
void Hub_queue_conn(Hub *hub, MyriadClient *client);

/** 
 * Sets up the proper client and calls Hub_queue_conn to do
 * the work.  This task function doesn't return until the ConnectionState
 * is finished processing.
 *
 * @brief Used internally to service connections from new clients.
 * @param data : Used by libtask to pass the MyriadClient struct.
 */
void Hub_connection_handler(void *data);

/** 
 * Goes through the entire list of active members and delivers this message
 * to them.
 *
 * @brief Delivers the message to everyone on the hub.
 * @param hub : Hub to deliver on.
 * @param msg : Message to send.
 */
void Hub_broadcast(Hub *hub, Message *msg);

#if 1
#define trc(A,S,N) dbg("%d -> %d : %s, '%c'",S,N,"" # A, *p)
#else
#define trc(A,S,N)
#endif

#endif
