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
#include <stdlib.h>
#include <string.h>
#include "cut.h"
#include "stackish/stackish.h"

void __CUT_BRINGUP__StackishTest( void ) {
}


void __CUT__Stackish_parsing()
{
  size_t nread = 0;
  char *buffer[] = {
    "["
      "[ \"test this\" good\n"
      "[ 1234 @an:integer 345.78 @a-float \"like yeah\" @a_string test\n"
      "[ \t\t\t\n\n\n\t [ 1 2 3 4 5 ] [ 1.1 2.2 3.3 4.4 5.5 ] [ \"test\" strings nesting\n"
      "doc\n",
    "[ [ [ [ [ [ [ [ one two three four five six seven eight\n",
    "[ 1 [ 2 [ 3 [ 4 [ 5 [ 6 six five four three two one\n",
    NULL
  };
  int nbuf;

  stackish_parser parser;
  memset(&parser, 0, sizeof(parser));

  for(nbuf = 0, nread = 0; buffer[nbuf] != NULL; nread = 0, nbuf++)
  {
    stackish_parser_init(&parser);

    size_t len = strlen(buffer[nbuf]);
    nread = stackish_parser_execute(&parser, buffer[nbuf], len, 0);

    if(stackish_more(&parser)) {
      // move it up by four and parse it again
      nread = stackish_parser_execute(&parser, buffer[nbuf], len, nread+stackish_more(&parser)+1);
    }

    ASSERT(parser.root != NULL, "root was NULL after parsing");
    ASSERT(stackish_node_complete(&parser), "document isn't complete");
    ASSERT(stackish_parser_is_finished(&parser), "shouldn't finish parsing");
    ASSERT(!stackish_parser_has_error(&parser), "parsing error");

    stackish_node_clear(&parser);
  }

  ASSERT_EQUALS(nbuf, 3, "didn't process all of the sample docs");
  ASSERT(buffer[nbuf] == NULL, "not at last doc");

  stackish_node_clear(&parser);
}

void __CUT__Stackish_node_with_blobs()
{
  bstring noblob = bfromcstr("[ \"test\" 123434 0.34555 test ");
  bstring blob = bfromcstr("[ '10:0123456789' '20:01234567890123456789' '1:0' '0:' test ");
  ASSERT(blob != NULL, "failed to make blob test");

  Node *res = Node_parse(noblob);
  ASSERT(res != NULL, "failed to parse non-blob string");

  Node *res2 = Node_parse(blob);
  ASSERT(res2 != NULL, "failed to parse BLOB string");

  bstring b1, b2, b3, b4;
  ASSERT(Node_decons(res2, 0, "[bbbbw", &b1, &b2, &b3, &b4, "test"), "failed to deconstruct");
  ASSERT_EQUALS(blength(b1), 0, "fourth blob wrong size");
  ASSERT_EQUALS(blength(b2), 1, "third blob wrong size");
  ASSERT_EQUALS(blength(b3), 20, "second blob wrong size");
  ASSERT_EQUALS(blength(b4), 10, "first blob wrong size");

  bdestroy(blob); bdestroy(noblob);
  Node_destroy(res);
  Node_destroy(res2);
}


void __CUT__Stackish_multi_in_bstring()
{
  size_t from = 0;
  bstring multi = bfromcstr("[header\n[ \"stuff\" next\n");

  Node *header = Node_parse_seq(multi, &from);
  ASSERT(header != NULL, "failed to parse first header");
  ASSERT(from < (size_t)blength(multi), "shouldn't do whole string");
  Node_dump(header, ' ', 1);

  Node *next = Node_parse_seq(multi, &from);
  ASSERT(next != NULL, "failed to parse next");
  ASSERT_EQUALS(from, (size_t)blength(multi), "didn't parse the whole string");
  Node_dump(next, ' ', 1);

  Node_destroy(next);
  Node_destroy(header);
  bdestroy(multi);
}

void __CUT_TAKEDOWN__StackishTest( void ) {
}
