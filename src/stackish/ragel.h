#ifndef ragel_h
#define ragel_h

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


#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define LEN(AT, FPC) (FPC - buffer - parser->AT)
#define MARK(M,FPC) (parser->M = (FPC) - buffer)
#define PTR_TO(F) (buffer + parser->F)

#define RAGEL_INIT(name, code) int name##_init(name *parser)  {\
  int cs = 0;\
  memset(parser, 0, sizeof(name));\
  { code }\
  parser->cs = cs;\
  return(1);\
}

#define RAGEL_DEFINE_FUNCTIONS(name, exec, eof) size_t name##_execute(name *parser, const char *buffer, size_t len, size_t off)  {\
  const char *p, *pe;\
  int cs = parser->cs;\
\
  assert(off <= len && "offset past end of buffer");\
\
  p = buffer+off;\
  pe = buffer+len;\
\
  assert(*pe == '\0' && "pointer does not end on NUL");\
  assert((size_t)(pe - p) == len - off && "pointers aren't same distance");\
\
  { exec };\
\
  parser->cs = cs;\
  parser->nread = p - buffer;\
\
  assert(p <= pe && "buffer overflow after parsing execute");\
  assert(parser->nread <= len+1 && "nread longer than length");\
  return(parser->nread);\
}\
\
int name##_has_error(name *parser) {\
  return parser->cs == name##_error;\
}\
\
int name##_is_finished(name *parser) {\
  return parser->cs == name##_first_final;\
}\
\
int name##_finish(name *parser)\
{\
  int cs = parser->cs;\
\
  { eof }\
\
  parser->cs = cs;\
\
  if (name##_has_error(parser) ) {\
    return -1;\
  } else if (name##_is_finished(parser) ) {\
    return 1;\
  } else {\
    return 0;\
  }\
}

#endif
