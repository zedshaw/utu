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
#include <myriad/defend.h>
#include <stdlib.h>
#include "stackish/stackish.h"
#include <ctype.h>
#include "node_algo.h"

void Node_dump(Node *d, char sep, int follow_sibs) 
{
  int i = 0;
  bstring str = Node_bstr(d, follow_sibs);

  assert_not(d, NULL);

  // go through and convert non-printables to printable, but not the last \n
  for(i = 0; i < blength(str)-1; i++) {
    if(!isprint(str->data[i])) {
      str->data[i] = str->data[i] % (126-32) + 32;
    }
  }

  fwrite(str->data, blength(str), 1, stderr);

  if(bchar(str,blength(str)-1) != '\n') {
    fprintf(stderr, "\n");
  }

  bdestroy(str);
}

bstring Node_bstr(Node *d, int follow_sibs)
{
  int rc = 0;
  bstring temp = bfromcstr("");
  assert_mem(temp);

  assert_not(d, NULL);

  Node_catbstr(temp, d, ' ', follow_sibs);
  
  rc = bconchar(temp, '\n');
  assert(rc == BSTR_OK && "failed to append separator char");

  return temp;
}

void Node_catbstr(bstring str, Node *d, char sep, int follow_sibs) 
{
  int rc = 0;

  assert_not(str, NULL);

  if(d == NULL) return;

  if(d->sibling != NULL && follow_sibs) {
    Node_catbstr(str, d->sibling, sep, follow_sibs);
  }

  if(d->type == TYPE_GROUP) {
    rc = bformata(str, "[%c", sep);
    assert(rc == BSTR_OK);
  }

  if(d->child != NULL) {
    // we always follow siblings on the children
    Node_catbstr(str, d->child, sep, 1);
  }

  switch(d->type) {
    case TYPE_BLOB:
      bformata(str, "'%zu:" , blength(d->value.string));
      bconcat(str, d->value.string);
      bformata(str, "\'%c", sep);
      break;
    case TYPE_STRING:
      bformata(str, "\"%s\"%c" , bdata(d->value.string), sep);
      break;
    case TYPE_NUMBER:
      bformata(str, "%llu%c", d->value.number, sep);
      break;
    case TYPE_FLOAT:
      bformata(str, "%f%c", d->value.floating, sep);
      break;
    case TYPE_GROUP: 
      if(!d->name || bchar(d->name, 0) == '@') {
        rc = bformata(str, "]%c", sep);
        assert(rc == BSTR_OK);
      }
      break;
    case TYPE_INVALID: // fallthrough
    default:
      assert(!"invalid type for node");
      break;
  }

  if(d->name != NULL) {
    rc = bformata(str, "%s%c", bdata(d->name), sep); 
    assert(rc == BSTR_OK);
  }
}

inline void Node_intern_destroy(Node *d)
{
  if(d->type == TYPE_BLOB || d->type == TYPE_STRING) {
    bdestroy(d->value.string);
  }

  if(d->name != NULL) {
    bdestroy(d->name);
  }

  free(d);
}

void Node_destroy(Node *root) 
{
  BIN_TREE_MAP_ON_ELEMENTS_POSTORDER(Node, root, d, sibling, child, 
       Node_intern_destroy(d)); 
}

void *node_test_calloc()
{
  return calloc(1,sizeof(Node)); 
}

inline Node *Node_create(Node *parent, enum NodeType type) 
{
  Node *node = node_test_calloc();
  assert_mem(node);

  node->type = type;

  // now just add the new node on to the current's children
  if(parent) {
    node->parent = parent;
    LIST_ADD(Node, parent->child, node, sibling);
  }

  return node;
}


#define DEFINE_NEW_NODE_FUNC(name,val_name,node_type,param_type) Node *Node_new_##name(Node *parent, param_type data)\
{\
  Node *node = Node_create(parent, node_type);\
  node->value.val_name = data;\
  return node;\
}

DEFINE_NEW_NODE_FUNC(string, string, TYPE_STRING, bstring);
DEFINE_NEW_NODE_FUNC(blob, string, TYPE_BLOB, bstring);
DEFINE_NEW_NODE_FUNC(number, number, TYPE_NUMBER, uint64_t);
DEFINE_NEW_NODE_FUNC(float, floating, TYPE_FLOAT, double);

Node *Node_new_group(Node *parent)
{
  return Node_create(parent, TYPE_GROUP);
}


void Node_name(Node *node, bstring name) 
{
  assert_not(node, NULL);
  assert_not(name, NULL);

  if(node->name) {
    bdestroy(node->name);
  }

  node->name = name;
}



Node *Node_cons(const char *format, ...) {
  const char *cur = format;
  va_list args;
  va_start (args, format);
  Node *root = NULL;
  Node *cur_node = NULL;

  assert_not(format, NULL);

#define UP() { cur_node = cur_node->parent; if(cur_node == NULL) break; }

  for(; *cur != '\0'; cur++) {
    switch(*cur) {
      case '[':
        cur_node = Node_new_group(cur_node);
        if(root == NULL) {
          root = cur_node;
        }
        break;
      case 'n': {
                  uint64_t number = va_arg(args, uint64_t);
                  Node *n = Node_new_number(cur_node, number);
                  assert_not(n,NULL);
                  break;
                }
      case 'f': {
                  double floating = va_arg(args, double);
                  Node *n = Node_new_float(cur_node, floating);
                  assert_not(n,NULL);
                  break;
                }
      case 'b': {
                  bstring blob = va_arg(args, bstring);
                  check(blob != NULL, "NULL given for blob");
                  Node *n = Node_new_blob(cur_node, blob);
                  assert_not(n,NULL);
                  break;
                }
      case 's': {
                  bstring string = va_arg(args, bstring);
                  check(string != NULL, "NULL given for string");
                  Node *n = Node_new_string(cur_node, string);
                  check(n != NULL, "Failed to create new string node");
                  break;
                }
      case ']': {
                  UP();
                  break;
                }
      case '@': {
                  const char *attr = va_arg(args, const char *);
                  check(attr != NULL, "NULL given for attr");
                  Node_name(cur_node->child, bfromcstr(attr));
                  break;
                }
      case 'w': {
                  const char *word = va_arg(args, const char *);
                  check(word != NULL, "NULL given for word name");
                  Node_name(cur_node, bfromcstr(word));
                  UP();
                  break;
                }
      case 'G': {
                  Node *group = va_arg(args, Node *);
                  check(group != NULL, "NULL given for group to add");
                  LIST_ADD(Node, cur_node->child, group, sibling);
                  break;
                }
      case ' ':
      case '\t':
      case '\n': {
                   break;
                 }

      default: {
                 fail("invalid character in spec");
                 break;
               }
    }
  }

  assert(*cur == '\0');
  assert(cur_node == NULL);
  va_end(args);

  return root;
  on_fail(return NULL);
}


#define check_type(V,T,N) check(V != NULL, "NULL given for " # T); \
                  if(N->type != TYPE_##T) {\
                    log(ERROR, "expecting " # T "(#%d), but got type #%d data:", TYPE_##T, N->type);\
                    Node_dump(N, ' ', 0);\
                    fail("couldn't deconstruct Node");\
                  }

int Node_decons(Node *root, int copy, const char *format, ...) {
  const char *cur = format;
  va_list args;
  va_start (args, format);
  Node *cur_node = root;

  assert_not(root, NULL);
  assert_not(format, NULL);

#define UP() { cur_node = cur_node->parent; if(cur_node == NULL) break; }
#define NEXT() { if(cur_node->sibling) cur_node = cur_node->sibling; if(cur_node == NULL) break; }

  for(; *cur != '\0'; cur++) {
    if(cur_node == NULL) {
      log(ERROR, "deconstruct ran out at char #%jd (%c) in the spec: %s", (intmax_t)(cur-format), *cur, format);
      break;
    }

    switch(*cur) {
      case '[':
        cur_node = cur_node->child;
        break;
      case 'n': {
                  uint64_t *number = va_arg(args, uint64_t *);
                  check_type(number, NUMBER, cur_node);
                  *number = cur_node->value.number;
                  NEXT();
                  break;
                }
      case 'f': {
                  double *floating = va_arg(args, double *);
                  *floating = cur_node->value.floating;
                  check_type(floating, FLOAT, cur_node);
                  NEXT();
                  break;
                }
      case 's': {
                  bstring *str = va_arg(args, bstring *);
                  check_type(str, STRING, cur_node);
                  if(copy)
                    *str = bstrcpy(cur_node->value.string);
                  else
                    *str = cur_node->value.string;
                  NEXT();
                  break;
                }
      case 'b': {
                  bstring *blob = va_arg(args, bstring *);
                  check_type(blob, BLOB, cur_node);
                  if(copy)
                    *blob = bstrcpy(cur_node->value.string);
                  else
                    *blob = cur_node->value.string;
                  NEXT();
                  break;
                }
      case ']': {
                  UP();
                  NEXT();
                  break;
                }
      case '@': {
                  const char *attr = va_arg(args, const char *);
                  check(attr != NULL, "NULL for given for ATTR name");
                  check_then(cur_node->name && biseqcstr(cur_node->name, attr), 
                      "wrong attribute name at:", Node_dump(cur_node, ' ', 1));
                  // just confirm but don't go to the next one
                  break;
                }
      case 'w': {
                  UP();
                  const char *word = va_arg(args, const char *);
                  check(word != NULL, "NULL given for word");
                  check_then(cur_node->name && biseqcstr(cur_node->name, word), 
                      "wrong word name at:", Node_dump(cur_node, ' ', 1));
                  NEXT();
                  break;
                }
      case 'G': {
                  Node **group = va_arg(args, Node **);
                  check(group != NULL, "NULL given for group");
                  check(*group == NULL, "Output var wasn't NULL, must be NULL.");
                  *group = cur_node;
                  NEXT();
                  break;
                }
      case '.': NEXT(); break;

      default: {
                 fail("invalid char in spec");
                 break;
               }
    }
  }
  
  va_end(args);

  check(*cur == '\0', "didn't process whole specification");

  return 1;
  on_fail(return 0);
}


void Node_add_child(Node *parent, Node *child) 
{
  assert_not(parent, NULL);
  assert_not(child, NULL);

  LIST_ADD(Node, parent->child, child, sibling);
  child->parent = parent;
}

void Node_add_sib(Node *sib1, Node *sib2)
{
  assert_not(sib1, NULL);
  assert_not(sib2, NULL);

  LIST_ADD(Node, sib1, sib2, sibling);
  sib2->parent = sib1->parent;
}

Node *Node_parse(bstring buf)
{
  size_t from = 0;

  assert_not(buf, NULL);

  return Node_parse_seq(buf, &from);
}

Node *Node_parse_seq(bstring buf, size_t *from)
{
  size_t nread = *from;
  stackish_parser parser;
  stackish_parser_init(&parser);
  char last = bchar(buf, blength(buf) - 1);

  assert_not(buf, NULL);
  assert_not(from, NULL);

  // make sure that the string ends in at least one space of some kind for the parser
  check(last == ' ' || last == '\n' || last == '\t', "buffer doesn't end in either ' \\n\\t'");

  nread = stackish_parser_execute(&parser, (const char *)bdata(buf), blength(buf), nread);
  check(!stackish_parser_has_error(&parser), "parsing error on stackish string");

  while(stackish_more(&parser) && !stackish_parser_is_finished(&parser) && nread < (size_t)blength(buf)) {
    assert(nread+stackish_more(&parser)+1 < (size_t)blength(buf) && "buffer overflow");
    // there is a blob that we have to pull out in order to continue
    nread = stackish_parser_execute(&parser, (const char *)bdata(buf), blength(buf), 
        nread+stackish_more(&parser)+1);
    check(!stackish_parser_has_error(&parser), "parsing error after BLOB");
  }

  *from = nread + 1;

  // skip over a last trailing newline
  while(bchar(buf, *from) == '\n') (*from)++;

  return parser.root;

  on_fail(dbg("failed parsing after %zu bytes", nread);
      stackish_node_clear(&parser); 
      *from = nread + 1;
      return NULL);
}

Node *Node_from_str(Node *parent, enum NodeType type, const char *start, size_t length) 
{
  char *end = NULL;

  switch(type) {
    case TYPE_BLOB:
      return Node_new_blob(parent, blk2bstr(start, length));
      break;
    case TYPE_STRING:
      return Node_new_string(parent, blk2bstr(start, length));
      break;
    case TYPE_NUMBER:
      end = NULL;
      uint64_t number = strtoull(start, &end, 10);
      check(end == start + length, "malformed number on input");
      return Node_new_number(parent, number);
      break;
    case TYPE_FLOAT:
      end = NULL;
      double floating = strtod(start, &end);
      check(end == start + length, "malformed float on input");
      return Node_new_float(parent, floating);
      break;
    default:
      fail("invalid node type given");
  }

  // every branch of the above should return a value
  ensure(return NULL);
}
