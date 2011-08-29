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

#include "stackish/ragel.h"
#include "stackish/stackish.h"
#include <myriad/defend.h>

#define push(T,M,F) handle_push(parser, TYPE_##T, PTR_TO(M), LEN(M, F)) 

inline int handle_push(stackish_parser *parser, enum NodeType type, const char *start, size_t length)
{
  assert_not(parser, NULL);
  assert_not(start, NULL);

  Node *current = parser->current;

  check(current, "parsing failure, no current node");
  check(parser->root, "parsing failure, no root node");
  check(current->type == TYPE_GROUP, "parsing failure, pushing onto a node that's not a group");

  Node *node = Node_from_str(current, type, start, length);

  return node != NULL;
  on_fail(return 0);
}


/** Returns false if the document isn't done, otherwise true and -1 if there's a failure.*/
inline int handle_word(stackish_parser *parser, const char *start, size_t length)
{
  assert_not(parser, NULL);

  Node *current = parser->current;

  check(parser->root, "parsing error, root node not ready");
  check(parser->current, "parsing error, parser doesn't have an active current node");

  if(start == NULL) {
    if(current->name != NULL) bdestroy(current->name);
    current->name = NULL;
  } else {
    Node_name(current, blk2bstr(start, length));
  }

  // either quit if we're at the root or move up to the parent
  if(current == parser->root) {
    return 1;
  } else {
    parser->current = current->parent;
    return 0;
  }

  on_fail(return -1);
}


/** Similar to handle_word this returns true when the document is done, false
  otherwise. */
inline int handle_group(stackish_parser *parser) 
{
  return handle_word(parser, NULL, 0);
}


inline int handle_attr(stackish_parser *parser, const char *start, size_t length)
{
  assert_not(parser, NULL);

  Node *current = parser->current->child;
  check(current, "parsing failure, attempting to set an attribute of a node with no children");

  Node_name(current, blk2bstr(start, length));

  return 1;
  on_fail(return 0);
}


inline void handle_start(stackish_parser *parser)
{
  assert_not(parser, NULL);

  // new group will ignore root if it is NULL, and attach it otherwise
  Node *mark = Node_new_group(parser->current);

  // first node, so set the root and current to it
  if(parser->root == NULL) {
    parser->root = mark;
  } 

  // now we set the current to the emark
  parser->current = mark;
}


/** machine **/
%%{
  machine stackish_parser;

  action mark { MARK(mark, fpc); }
  action number { if(!push(NUMBER, mark, fpc)) fbreak; }
  action float { if(!push(FLOAT, mark, fpc)) fbreak; }
  action string { if(!push(STRING, mark, fpc)) fbreak; }
  action start { handle_start(parser); }
  action blob { if(!push(BLOB, mark, fpc)) fbreak; else parser->more = 0;}
  action word {
    if(handle_word(parser, PTR_TO(mark), LEN(mark, fpc))) {
      // all done, stop processing
      fbreak;
    }
  }
  action group {
    if(handle_group(parser)) {
      // all done, stop processing
      fbreak;
    }
  }
  action more { 
    char *end = NULL; 
    parser->more = strtoul(PTR_TO(mark), &end, 10); 
    assert(end == fpc && "buffer overflow processing blob length");
    MARK(mark, fpc+1);
    if(parser->more > 0) {
      // we only need to break if there needs to be a chunk to process
      fbreak;
    }
  }
  action attrib { if(!handle_attr(parser, PTR_TO(mark), LEN(mark, fpc))) fbreak; }

  number =  digit+;
  float  =  ('-' | '+')? digit+ "." digit+;
  string =  '"' %mark (any -- '"')* '"' @string;
  start  = "[";
  word   =  alpha+ (alnum | '-' | '_' | '.' | ':')*;
  blob   =  "'" digit+ >mark ":" @more "'" @blob;
  group  =  "]";
  attrib =  "@" word;

  stackish = (space+ 
      | number >mark %number 
      | float >mark %float 
      | string 
      | start @start
      | word >mark %word
      | blob 
      | group @group
      | attrib >mark %attrib)**;

  main := stackish;
}%%

/** Data **/
%% write data;

RAGEL_INIT(stackish_parser, {
    %% write init;
})

RAGEL_DEFINE_FUNCTIONS(stackish_parser, {
    %% write exec;
    }, 
    {
    %%write eof;
    });


void stackish_node_clear(stackish_parser *parser) {
  assert_not(parser, NULL);

  if(parser->root) {
    Node_destroy(parser->root);
    parser->root = NULL;
    parser->current = NULL;
  }
}

void stackish_node_destroy(stackish_parser *parser) {
  if(parser->root != NULL) Node_destroy(parser->root);
}

