#ifndef utu_frame_h
#define utu_frame_h

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



#include "stackish/node.h"
#include <myriad/sbuf.h>

/**
 * A small FrameSource structure that is intended to be part of another
 * structure (rather than a pointer).  It is initialized
 * with a working sbuf and the fd to read/write from.
 * You then call the read/send methods on it to 
 * get your information.
 */
typedef struct FrameSource {
  sbuf *out;
  sbuf *in;
  int fd;
} FrameSource;


/**
 * Reads a body, header, and the raw header buffer from the
 * given FrameSource.  The FrameSource is passed in by-value since
 * it's generally easier to work with.
 *
 * The returned hbuf is the exact header read from the input
 * buffer so that encryption/decription will function
 * correctly.  The header is an out parameter that gets the
 * Node.  The returned Node is the body.  You're expected to
 * Node_destroy() the return value and the header.
 *
 * Returns NULL on error and sets hbuf and header to NULL as
 * well.
 *
 * @param frame FrameSource structure to read frames from.
 * @param hbuf OUT parameter for the header's raw data.
 * @param header OUT parameter for the resulting header stackish node.
 * @return The resulting body node.
 */
Node *FrameSource_recv(FrameSource frame, bstring *hbuf, Node **header) ;


/**
 * Takes a bstring header and a msg body and sends it on.  It will
 * flush the FrameSource.buf if it's told to, otherwise it just accumulates
 * the results and flushes the buffer when it's full.
 *
 * @param frame The FrameSource to put frames into.
 * @param header The raw bstring for the header to send.
 * @param msg The message body to send.
 * @param flush Whether to flush it immediately or not.
 * @returns 0 (FALSE) on failure and 1 (TRUE) on success.
 */
int FrameSource_send(FrameSource frame, bstring header, Node *msg, int flush);

#endif
