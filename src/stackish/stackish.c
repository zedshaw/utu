#line 1 "stackish/stackish.rl"
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
#line 160 "stackish/stackish.rl"


/** Data **/

#line 114 "stackish/stackish.c"
static const char _stackish_parser_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 5, 1, 6, 1, 
	7, 1, 8, 1, 9, 2, 0, 3, 
	2, 1, 0, 2, 1, 4, 2, 1, 
	7, 2, 2, 0, 2, 2, 4, 2, 
	2, 7, 2, 6, 0, 2, 6, 4, 
	2, 6, 7, 2, 9, 0, 2, 9, 
	4, 2, 9, 7
};

static const char _stackish_parser_key_offsets[] = {
	0, 0, 1, 2, 4, 7, 8, 10, 
	13, 15, 19, 35, 51, 69, 87
};

static const char _stackish_parser_trans_keys[] = {
	34, 34, 48, 57, 58, 48, 57, 39, 
	48, 57, 46, 48, 57, 48, 57, 65, 
	90, 97, 122, 32, 34, 39, 43, 45, 
	64, 91, 93, 9, 13, 48, 57, 65, 
	90, 97, 122, 32, 34, 39, 43, 45, 
	64, 91, 93, 9, 13, 48, 57, 65, 
	90, 97, 122, 32, 34, 39, 43, 64, 
	91, 93, 95, 9, 13, 45, 46, 48, 
	58, 65, 90, 97, 122, 32, 34, 39, 
	43, 64, 91, 93, 95, 9, 13, 45, 
	46, 48, 58, 65, 90, 97, 122, 32, 
	34, 39, 43, 45, 46, 64, 91, 93, 
	9, 13, 48, 57, 65, 90, 97, 122, 
	0
};

static const char _stackish_parser_single_lengths[] = {
	0, 1, 1, 0, 1, 1, 0, 1, 
	0, 0, 8, 8, 8, 8, 9
};

static const char _stackish_parser_range_lengths[] = {
	0, 0, 0, 1, 1, 0, 1, 1, 
	1, 2, 4, 4, 5, 5, 4
};

static const char _stackish_parser_index_offsets[] = {
	0, 0, 2, 4, 6, 9, 11, 13, 
	16, 18, 21, 34, 47, 61, 75
};

static const char _stackish_parser_trans_targs_wi[] = {
	10, 2, 10, 2, 4, 0, 5, 4, 
	0, 10, 0, 7, 0, 8, 7, 0, 
	11, 0, 12, 12, 0, 10, 1, 3, 
	6, 6, 9, 10, 10, 10, 14, 13, 
	13, 0, 10, 1, 3, 6, 6, 9, 
	10, 10, 10, 11, 13, 13, 0, 10, 
	1, 3, 6, 9, 10, 10, 12, 10, 
	12, 12, 12, 12, 0, 10, 1, 3, 
	6, 9, 10, 10, 13, 10, 13, 13, 
	13, 13, 0, 10, 1, 3, 6, 6, 
	8, 9, 10, 10, 10, 14, 13, 13, 
	0, 0
};

static const char _stackish_parser_trans_actions_wi[] = {
	21, 1, 7, 0, 1, 0, 17, 0, 
	0, 11, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	1, 1, 1, 9, 15, 0, 1, 1, 
	1, 0, 5, 5, 5, 33, 33, 33, 
	36, 39, 5, 0, 33, 33, 0, 19, 
	19, 19, 51, 51, 54, 57, 0, 19, 
	0, 0, 0, 0, 0, 13, 13, 13, 
	42, 42, 45, 48, 0, 13, 0, 0, 
	0, 0, 0, 3, 3, 3, 24, 24, 
	0, 24, 27, 30, 3, 0, 24, 24, 
	0, 0
};

static const int stackish_parser_start = 10;
static const int stackish_parser_first_final = 10;
static const int stackish_parser_error = 0;

static const int stackish_parser_en_main = 10;

#line 164 "stackish/stackish.rl"

RAGEL_INIT(stackish_parser, {
    
#line 203 "stackish/stackish.c"
	{
	cs = stackish_parser_start;
	}
#line 167 "stackish/stackish.rl"
})

RAGEL_DEFINE_FUNCTIONS(stackish_parser, {
    
#line 212 "stackish/stackish.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _out;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _stackish_parser_trans_keys + _stackish_parser_key_offsets[cs];
	_trans = _stackish_parser_index_offsets[cs];

	_klen = _stackish_parser_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _stackish_parser_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += ((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	cs = _stackish_parser_trans_targs_wi[_trans];

	if ( _stackish_parser_trans_actions_wi[_trans] == 0 )
		goto _again;

	_acts = _stackish_parser_actions + _stackish_parser_trans_actions_wi[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 110 "stackish/stackish.rl"
	{ MARK(mark, p); }
	break;
	case 1:
#line 111 "stackish/stackish.rl"
	{ if(!push(NUMBER, mark, p)) goto _out; }
	break;
	case 2:
#line 112 "stackish/stackish.rl"
	{ if(!push(FLOAT, mark, p)) goto _out; }
	break;
	case 3:
#line 113 "stackish/stackish.rl"
	{ if(!push(STRING, mark, p)) goto _out; }
	break;
	case 4:
#line 114 "stackish/stackish.rl"
	{ handle_start(parser); }
	break;
	case 5:
#line 115 "stackish/stackish.rl"
	{ if(!push(BLOB, mark, p)) goto _out; else parser->more = 0;}
	break;
	case 6:
#line 116 "stackish/stackish.rl"
	{
    if(handle_word(parser, PTR_TO(mark), LEN(mark, p))) {
      // all done, stop processing
      goto _out;
    }
  }
	break;
	case 7:
#line 122 "stackish/stackish.rl"
	{
    if(handle_group(parser)) {
      // all done, stop processing
      goto _out;
    }
  }
	break;
	case 8:
#line 128 "stackish/stackish.rl"
	{ 
    char *end = NULL; 
    parser->more = strtoul(PTR_TO(mark), &end, 10); 
    assert(end == p && "buffer overflow processing blob length");
    MARK(mark, p+1);
    if(parser->more > 0) {
      // we only need to break if there needs to be a chunk to process
      goto _out;
    }
  }
	break;
	case 9:
#line 138 "stackish/stackish.rl"
	{ if(!handle_attr(parser, PTR_TO(mark), LEN(mark, p))) goto _out; }
	break;
#line 344 "stackish/stackish.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_out: {}
	}
#line 171 "stackish/stackish.rl"
    }, 
    {
    
#line 359 "stackish/stackish.c"
#line 174 "stackish/stackish.rl"
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

