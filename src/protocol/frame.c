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


#include "frame.h"
#include <myriad/myriad.h>

#define MAX_READ_ATTEMPTS 50

inline int io_ensure_sbuf(sbuf *buf, int fd, size_t length)
{
  int rc = 1;
  size_t ntrans = 0;

  assert_not(buf, NULL);
  assert(isvalidsock(fd) && "not a valid socket");

  // make sure there's enough for the payload
  while((size_t)sbuf_data_avail(buf) < length) {
    // need to read more
    rc = io_read_sbuf(fd, buf, &ntrans);
    if(!rc || errno) {
      if(errno && errno != EBADF) {
        fail("unexpected error reading from socket");
      }
      errno = 0;
      break;
    }
    check(ntrans != 0, "attempted read returned 0 length");

    if(ntrans < length) {
      dbg("attempted read returned %zu rather than %zu with %jd avail", ntrans, length, (intmax_t)sbuf_data_avail(buf));
    }
  }

  ensure(return rc);
}

Node *FrameSource_recv(FrameSource frame, bstring *hbuf, Node **header)
{
  Node *msg = NULL;
  unsigned short length = 0;
  int rc = 0;
  size_t from = 0;
  bstring data = NULL;

  assert_not(header, NULL);

  *hbuf = NULL; *header = NULL;

  rc = io_ensure_sbuf(frame.in, frame.fd, sizeof(uint16_t));
  if(!rc) return NULL;  // got closed, nothing to do

  rc = sbuf_get_ntohs(frame.in, &length);
  check(rc, "length read failure");
  check(length > 0, "zero length message");

  rc = io_ensure_sbuf(frame.in, frame.fd, length);
  check(rc, "io read failure");

  data = sbuf_read_bstr(frame.in, length);
  check(data, "failed to read a single bstr");
  check(blength(data) == length, "didn't get what we asked for from the sbuf");

  *header = Node_parse_seq(data, &from);
  check(*header, "failed to parse a header");
  // carve out the hbuf bstring for comparison
  *hbuf = bmidstr(data, 0, from);

  if(from < length) {
    // more to read
    msg = Node_parse_seq(data, &from);
    check(msg, "failed to read body");
  }
  check(from == length, "trailing data violates");

  ensure(bdestroy(data); return msg);
}

int FrameSource_send(FrameSource frame, bstring header, Node *msg, int flush)
{
  int rc = 0;
  bstring data = Node_bstr(msg, 1);
  uint16_t msg_len = 0;
  size_t nout;

  assert_not(header, NULL);
  assert_not(msg, NULL);

  // convert the msg to a bstring to find out the full size
  check(data, "failed to build bstring from msg Node");

  // make sure the sbuf has enough space
  msg_len = blength(header) + blength(data);

  if(sbuf_space_avail(frame.out) < msg_len) {
    rc = io_flush_sbuf(frame.fd, frame.out, &nout);
    check(rc, "failed to flush sbuf to make room");
    check_then(sbuf_space_avail(frame.out) >= msg_len, "insufficient space available in sbuf", rc = 0);
  }

  // write the length, then header, then msg
  rc = sbuf_put_htons(frame.out, msg_len);
  check(rc, "failed to write length");

  rc = sbuf_write_bstr(frame.out, header);
  check(rc, "failed to write header");

  rc = sbuf_write_bstr(frame.out, data);
  check(rc, "failed to write body");
 
  if(flush) {
    rc = io_flush_sbuf(frame.fd, frame.out, &nout);
    check(rc, "failed to write to frame.fd");
  }

  ensure(bdestroy(data); return rc);
}


