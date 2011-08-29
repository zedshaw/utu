#ifndef utumendicant_proxy_h
#define utumendicant_proxy_h

#include "io.h"
#include "errors.h"

/**
 * Maintains state for a proxied connection to one hub from one
 * client.  In the future this means you'll be able to use one
 * mendicant to connect to multiple Hubs.  This is important in
 * the near future to allow the mendicant to handle all the nuances
 * of the clustering design.
 */
typedef struct Proxy {
  struct {
    /** Client's name as a string. */
    bstring name;
    /** Client's configured private key (public key only sent). */
    bstring key;
    /** File descriptor to process for the client socket. */
    int read_fd;
    int write_fd;
  } client;

  struct {
    /** The IP address/domain name of the host server. */
    bstring host;
    /** Port to connect to on the host server. */
    bstring port;
    /** The name of the server (all Utu peers have names). */
    bstring name;
    /** The expected public key of the server. */
    bstring key;
    /** File descriptor of the socket after connect. */
    int fd;
  } host;

  /** Internal cryptographic state. */
  CryptState *state;

  /** Cached key that was given back by the host. */
  bstring host_given_key;

} Proxy;

typedef enum { PROXY_CLIENT_NONE, PROXY_CLIENT_DOMAIN, PROXY_CLIENT_FIFO, PROXY_CLIENT_STDIO } ProxyClientType;

/** 
 * @function Proxy_listen
 * @brief Starts listening on 
 * @param conn : 
 * @param sock_name : 
 * @return int :
 */
int Proxy_listen(Proxy *conn, ProxyClientType, bstring name);

/** 
 * @function Proxy_configure
 * @brief Reads the two stackish configuration nodes from the listener.
 * @param conn : Connection to use.
 * @return int : true/false for pass/fail.
 */
int Proxy_configure(Proxy *conn);

/** 
 * @function Proxy_connect
 * @brief Connects to the Hub after the proxy is configured.
 * @param conn : Connection to use.
 * @return int : true/false for pass/fail.
 */
int Proxy_connect(Proxy *conn);

/** 
 * @function Proxy_event_loop
 * @brief After everything is configured and connected this starts things in motion.
 * @param conn : Connection to use.
 * @return int : true/false for pass/fail.
 */
int Proxy_event_loop(Proxy *conn);

/** 
 * @function Proxy_dump_invalid_state
 * @brief Dumps out debug info about the proxy state.
 * @param conn : Connection to use.
 */
void Proxy_dump_invalid_state(Proxy *conn);

/** 
 * @function Proxy_destroy
 * @brief Destroys the mendicant information.
 * @param conn : Connection to use.
 */
void Proxy_destroy(Proxy *conn);

/** 
 * Writes an error in stackish format to the stderr so the controlling
 * process can read it.
 */
void Proxy_error(Proxy *conn, UtuMendicantErrorCode code, bstring message);

/** 
 * @function Proxy_listener_send
 * @brief Sends the header and node to the client listener.
 * @param conn : Proxy connection to use.
 * @param header : A valid header stackish node.
 * @param body : A valid stackish body.
 * @return int : Whether it worked or not.
 */
int Proxy_listener_send(Proxy *conn, Node *header, Node *body);

/** 
 * @function Proxy_listener_recv
 * @brief Receives on header/body message from the listener.
 * @param conn : Connection to use.
 * @param header : OUT parameter for the header. 
 * @return Node *: Body returned, NULL if failed.
 */
Node *Proxy_listener_recv(Proxy *conn, Node **header);

#endif
