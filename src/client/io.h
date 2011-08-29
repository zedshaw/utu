#ifndef utu_io_h
#define utu_io_h

/**
 * The majority of these functions are modified from the book
 * Effective TCP/IP Programming by Jon Snader found at 
 * http://home.netcom.com/~jsnader/ and it's a great TCP/IP
 * book.
 */

#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <myriad/defend.h>
#include <myriad/bstring/bstrlib.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/un.h>
#include "../protocol/crypto.h"
#include "../stackish/node.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

/** 
 * @brief Fills bp with exactly len amount of data from fd, len must > 0.
 * @param fd : file descriptor to read from.
 * @param bp : base pointer to write stuff into.
 * @param len : length of data to read.
 * @return int : amount read or -1 if it failed.
 */
int readn(int fd, char *bp, size_t len);

/** 
 * @brief Reads a u_int16_t size specified vrec frame from 
 * @param fd : file descriptor to read from.
 * @return bstring : The raw frame or NULL if fail.
 */
bstring readframe(int fd);

/** 
 * @brief Reads the header and body nodes from the fd as a frame.
 * @param fd : valid filedescriptor to read from.
 * @param header : OUT parameter that will have the header.
 * @return Node *: The body result.
 */
Node *readnodes(int fd, Node **header);

/** 
 * @brief Higher level function to write a stackish header/body frame to fd.
 * @param fd : file descriptor to write to.
 * @param header : stackish node for the header.
 * @param body : stackish node for the body.
 * @return int : amount written (always greater than 0), or -1 on fail.
 */
int writenodes(int fd, Node *header, Node *body);

/** 
 * @brief Writes a bstring for header and then a stackish body to the fd.
 * @param fd : file descriptor to write to.
 * @param header : raw bstring version of the header.
 * @param body : stackish node for the body.
 * @return int : amount written (always greater than 0), or -1 on fail.
 */
int write_header_node(int fd, bstring header, Node *body);

/** 
 * @brief Sets up a timeout callback that will be run inside tselect later.
 * @param func : Function to call.
 * @param arg : argument to pass to this function.
 * @param ms : miliseconds to wait.
 * @return unsigned int : An id to use with untimeout, -1 if it fails.
 */
unsigned int timeout(void (*func)(void *), void *arg, int ms);

/** 
 * @brief Cancels a pending timeout by the given id.
 * @param id  : An id returned from timeout.
 */
void untimeout( unsigned int id );

/** 
 * @brief A special select that includes a timeout callback mechanism.
 * @param maxp1 : Max file descriptor to search.
 * @param re : Read fd_set to wait for read events.
 * @param we : Write fd_set to wait for write events.
 * @param ee  : Error fd_set to wait for error events.
 * @return int : The number of file descriptors ready, -1 on fail, 0 if nothing.
 * @see timeout
 * @see untimeout
 */
int tselect( int maxp1, fd_set *re, fd_set *we, fd_set *ee );


/** 
 * @brief Starts a TCP socket connection to the given host on the service name.
 * @param hname : host to connect with.
 * @param sname  : service (either port or string name).
 * @return int : file descriptor, or -1 if failed.
 */
int tcp_client( char *hname, char *sname );

/** 
 * @brief Connects to the given unix domain socket returning the fd.
 * @param name : socket name file name.
 * @return int : file descriptor id, or -1 if it failed.
 */
int unix_client(bstring name);

#define isvalidsock(s)	( ( s ) >= 0 )

#endif
