#ifndef node_h
#define node_h

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

#include <myriad/bstring/bstrlib.h>
#include <stdint.h>

/** Determines what is contained in the Node.value union. */
typedef enum NodeType { 
  TYPE_NUMBER, TYPE_STRING, TYPE_BLOB, TYPE_FLOAT, TYPE_INVALID, TYPE_GROUP
} NodeType;


/** 
 * A Node contains the data for this element of the Stackish tree,
 * and contains three linked lists: parent, sibling, child.  They 
 * end in a NULL reference when you're at the end of the chain and
 * you use the regular SGLIB macros to work with them.
 *
 * @see stackish_parser
 */
typedef struct Node {
  /** 
   * The name of the node, if a TYPE_GROUP and named then it's a word.
   * If any other type and named then it was given an attribute.
   * Otherwise it's NULL.
   */
  bstring name;

  /** Determines the type of the union internally. */
  NodeType type;

  /** Holds the actual data depending on the type.  node.string 
   * holds both TYPE_STRING and TYPE_BLOB.*/
  union {
    uint64_t number;
    bstring string;
    double floating;
  } value;

  /** The parent of this structure, NULL means this is the root. */
  struct Node *parent;

  /** The first sibling of this Node. */
  struct Node *sibling;

  /** The first child of this Node. */
  struct Node *child;
} Node;

/**
 * Dumps the node to stderr for debugging, converting an non-printable
 * characters to something printable (so you can see blobs).  The output
 * isn't exactly the same, but works to debug formats and data being passed
 * around.
 *
 * @param d The node to dump.
 * @param sep Separator char between nodes (canonical requires ' ' ASCII 32).
 * @param follow_sibs Whether to follow siblings of this node.
 */
void Node_dump(Node *d, char sep, int follow_sibs);


/**
 * Concats the stackish node to the end of the bstring, but DOESN'T
 * add a final sep char to finish off the structure.
 *
 * @param str String to concat this dumped node to.  MUST EXIST.
 * @param d The node to dump.
 * @param sep Separator char between nodes (canonical requires ' ' ASCII 32).
 * @param follow_sibs Whether to follow siblings of this node.
 */
void Node_catbstr(bstring str, Node *d, char sep, int follow_sibs);

/**
 * The most common way to make a bstring out of a stackish struct.
 * It DOES append the sep char since it's most common to just use one
 * bstring as a message.
 *
 * This function produces the "canonical" form of any Stackish structure
 * and should be the reference implementation.  It calls 
 * Node_catbstr(outstr, d, ' ', 1) to produce the string.  It then
 * call bconchar(outstr, '\n') to complete the string.
 *
 * @param d The node to dump.
 * @param follow_sibs Whether to follow siblings of this node.
 * @return Fully formed bstring with the node in serialized form.
 */
bstring Node_bstr(Node *d, int follow_sibs);

/**
 * Destroys a node and all things under it.
 *
 * @param root Node to start with, includes siblings.
 */
void Node_destroy(Node *root);

/** 
 * Constructs a new node that represents a string, attaching to parent if not NULL. 
 *
 * @param parent Node to attach to.
 * @param data Data to use (you don't own this anymore).
 * @return Newly attached node for further processing.
 */
Node *Node_new_string(Node *parent, bstring data);

/** Constructs a new node that represents a blob, attaching to parent if not NULL. */
Node *Node_new_blob(Node *parent, bstring data);

/** Constructs a new node that represents a number, attaching to parent if not NULL. */
Node *Node_new_number(Node *parent, uint64_t data);

/** Constructs a new node that represents a float, attaching to parent if not NULL. */
Node *Node_new_float(Node *parent, double data);

/** Constructs a new node that represents a group, attaching to parent if not NULL. */
Node *Node_new_group(Node *parent);

/** 
 * Names a node.  Named groups are normal, naming anything else makes an attribute, but
 * attributes should start with a '@'.  So calling Node_name(blob_node, "stuff") is 
 * wrong because a TYPE_BLOB should have a name of "@stuff" to signify it's an attribute.
 */
void Node_name(Node *node, bstring name);


/**
 * Constructs a Node tree according to a Stackish format.  This lets you build
 * nested structures in nearly the same format as Stackish, except that the data 
 * nodes are indicated by characters and then passed as variable arguments.
 *
 * '[' -- Start a new group, just like Stackish.
 * 'n' -- A number.  Argument is an uint64_t.
 * 'f' -- A float. Argument is a double.
 * 'b' -- A blob. Argument is a bstring.
 * 's' -- A string.  Argument is a bstring.
 * ']' -- An anonymous group. No argument needed.
 * '@' -- An attribute, include the @ in the name.  Argument is a const char *.
 * 'w' -- A word (to name a group). Argument is a const char *.
 * 'G' -- A group that you've previously constructed will be inserted here.
 *
 * It takes over all of the memory you pass in, so you can build the bstrings
 * inside the parameters and then when the Node is destroyed the memory 
 * will be freed.
 *
 * @param format A format string with the above format.
 * @return A node matching the format structure, or NULL if there's an error.
 */
Node *Node_cons(const char *format, ...);

/**
 * Deconstructs a stackish node by going back through it's children and filling
 * given pointers with the data it finds.  The 'w' and 'W' are not filled in,
 * but used as structure markers, so if they don't match then it returns 0 to
 * indicate the structure isn't valid.
 *
 * Node_decons(node, "[[bw[bw[bww", &header, "header", &payload, "payload", &tag, "tag", "msg");
 *
 * Remember that this is done in "pop order", so you basically have to reverse
 * things from when you did Node_cons.
 *
 * You can use a '.' in the spec to skip a node when you don't care about what type it is
 * and don't want to extract it.
 *
 * @param node The node to start deconstructing from.
 * @param copy Whether to copy the data or not.
 * @param format The deconstruct format.  Remember it's got to be reversed from the cons format used.
 * @return 0 for failure, 1 for success.  On failure the results are not to be trusted.
 */
int Node_decons(Node *node, int copy, const char *format, ...);

/**
 * Adds a child to the parent.
 */
void Node_add_child(Node *parent, Node *child);

/**
 * Adds a sibling to the front of sib1's sibling list.
 */
void Node_add_sib(Node *sib1, Node *sib2);


/**
 * Given a string that contains a *complete* stackish structure this
 * will parse it and return the Node representation.  It's required that
 * you end the buf with a ' ', '\n', or '\t', so that the structure is
 * complete.
 *
 * It returns the amount of the input buffer that was read so you can parse
 * out more.
 *
 * It actually calls Node_parse_seq() to do the real parse, and is only
 * useful if you're expecting the buffer to have one whole stackish structure
 * in it.
 *
 * If you need to parse multiple stackish structures then you'll need to use
 * Node_parse_seq() to get the full effect.
 *
 * @param buf A stackish string sitting in the buffer that needs to be parsed.
 * @return A fully formed Node from the parse operation, NULL if there was an error.
 */
Node *Node_parse(bstring buf);

/**
 * Used to parse a string that contains a series of stackish structures
 * in it.  The from parameter is an in-out parameter.  You pass in where
 * the parsing should start "from", and then the function changes it to
 * where to do the next parsing.  This means that you can set it 0 at
 * first and keep passing it in to parse successive documents.
 *
 * If NULL is returned then there was a parsing error, and from will
 * say where the error happened.
 *
 * @param buf A stackish structure that has some stackish in it.
 * @param from IN/OUT parameter that says where to start, and where the parsing ended inside buf.
 * @return A fully formed Node for the amount of work done, or NULL if there was a failure.
 */
Node *Node_parse_seq(bstring buf, size_t *from);

/**
 * Given a type and a string this will do the proper conversion to create the
 * right node.  It returns NULL if there's an error of any kind.
 */
Node *Node_from_str(Node *parent, enum NodeType type, const char *start, size_t length);

#endif

