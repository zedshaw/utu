#ifndef stackish_h
#define stackish_h

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
#include "stackish/ragel_declare.h"

/**
 * Represents a parser for the Stackish data description language.
 * Stackish originally started as a joke alternative to XML that built
 * s-expressions but using a forth-like stack as the mechanism instead.
 * Everything you can do with s-expressions is available with stackish,
 * but it is easier to parse and process and is lightning fast.
 *
 * A Stackish structure is nothing more than a series of single
 * lexical elements that operate on a given stack.  At the end of
 * their processing, the stack represents the data structure
 * as a tree of Node structures.  Each Node can be either one of a
 * set of determined types, but can be structured in any way you like.
 *
 * A sample stackish structure might be:
 *
 * <code>
 *   [ '5:hello' 0.1234 567 "yes" sample
 * </code>
 *
 * The above is a structure named 'sample' which has:
 *
 * - a string "yes"
 * - a number (unsigned integer) 567
 * - a float 0.1234
 * - a blob (binary string) with 'hello'
 *
 * Remember it's a stack parsed language so the nodes are put on
 * in reverse order.
 *
 * The available types for stackish structures are:
 *
 * - number : Any unsigned digit not larger than 32 bits in size.
 * - float  : A floating point number which can be negative and must have a . in it.
 * - string : A double quoted string that can have anything but a double quote in it.
 * - start  : A [ character which indicates the start of a structure or child structure.
 * - word   : A sequence of alpha-numeric, '-', '_', '.', or ':' chars that name the current structure on the stack.  [ stuff is the same as <stuff></stuff> with 'stuff' being a word.
 * - group  : A ] character that ends a group just like a word, but without a name.
 * - blob   : A binary string that starts with '\'', has a prefixed length, a ':', followed by the payload and a closing '\'' char.  Example is '5:hello'.  This lets you efficiently transport binary data or even safely wrap other stackish structures.
 * - attrib : A word that starts with @ which names the current object on the stack.  this is how you apply attributes in a stackish structure.
 *
 * Stackish allows you to encode s-expressions or XML structures in an efficient and
 * still readable format, and it's language and ranting agnostic.  Since nobody cares
 * about Stackish, it's safe to use and avoid the arguments.  Stackish can also be
 * converted to and from XML and s-expressions easily.  For example the above in XML
 * is:
 *
 * <pre>
 *   <sample>567 0.1234 <![CDATA[hello]]> yes</sample>
 * </pre>
 *
 * Yet, even this isn't entirely correct.  For example, stackish gives you
 * strict and implicit typing, so you know what is a number, float, string and
 * blob (cdata).  The above to XML looks like either a bunch of text children or
 * one text child depending on the parser.  Also, the CDATA section really can't
 * contain full binary data the same way a Stackish blob can.  Another encoding
 * could be like:
 *
 * <pre>
 *   <sample>
 *     <number>567</number>
 *     <float>0.1234</float>
 *     <blob><![CDATA[hello]]></blob>
 *     <string>yes</string>
 *   </sample>
 * </pre>
 *
 * A final advantage of Stackish is that it has a very strict canonical format
 * which makes it better suited to encrypted data payloads.  The canonical format
 * is defined as whatever Node_bstr(node, 1) produces.  This function canonicalizes
 * in the following way:
 *
 * - A single space separates each node representation.
 * - A single \n ends the final node output (the root's name).
 * - All attributes are done in the same order as given in the node.  Attributes should be order dependent for canonical purposes, so what comes in goes out or it's not the same.
 * - No conversion is done on any node.  If it's a float it stays a float even if it could be represented as a number.
 *
 * With this it's then possible to hash the bytes of a serialized Stackish structure
 * and compare it another for hashing or decryption purposes.  If the two don't match
 * then they aren't canonicalized and are different.  When in doubt, Node_bstr() is
 * definitive, even if it has a bug.
 *
 * Most of the parsing functions around this type are designed for internal use
 * only.  You should use Node_parse() and Node_parse_seq() to parse input
 * strings into Stackish structures.  If you want to use the stackish raw
 * functions then consider reading the code for the Node parsing functions.
 *
 * @see Node
 * @see Node_parse
 * @see Node_parse_seq
 */
typedef struct stackish_parser {
  int cs;
  size_t more;
  size_t nread;
  size_t mark;
  Node *root;
  Node *current;
} stackish_parser;

RAGEL_DECLARE_FUNCTIONS(stackish_parser);

void stackish_node_clear(stackish_parser *context);

void stackish_node_destroy(stackish_parser *context);

#define stackish_node_complete(P) ((P)->root->name && (P)->root == (P)->current)

#define stackish_node_root(P) ((P)->root)

#define stackish_more(P) ((P)->more)

#endif
