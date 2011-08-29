#ifndef utu_node_aglo_h
#define utu_node_aglo_h

/** This is the minimum algorithms necessary to make the Node structures
 * work.  They were taken from SGLIB.
 */

#define LIST_ADD(type, list, elem, next) {\
  (elem)->next = (list);\
  (list) = (elem);\
}

#define MAX_TREE_DEEP 128

#define __BIN_TREE_MAP_ON_ELEMENTS(type, tree, iteratedVariable, order, left, right, command) {\
  /* this is non-recursive implementation of tree traversal */\
  /* it maintains the path to the current node in the array '_path_' */\
  /* the _path_[0] contains the root of the tree; */\
  /* the _path_[_pathi_] contains the _current_element_ */\
  /* the macro does not use the _current_element_ after execution of command */\
  /* command can destroy it, it can free the element for example */\
  type *_path_[MAX_TREE_DEEP];\
  type *_right_[MAX_TREE_DEEP];\
  char _pass_[MAX_TREE_DEEP];\
  type *_cn_;\
  int _pathi_;\
  type *iteratedVariable;\
  _cn_ = (tree);\
  _pathi_ = 0;\
  while (_cn_!=NULL) {\
    /* push down to leftmost innermost element */\
    while(_cn_!=NULL) {\
      _path_[_pathi_] = _cn_;\
      _right_[_pathi_] = _cn_->right;\
      _pass_[_pathi_] = 0;\
      _cn_ = _cn_->left;\
      if (order == 0) {\
        iteratedVariable = _path_[_pathi_];\
        {command;}\
      }\
      _pathi_ ++;\
      if (_pathi_ >= MAX_TREE_DEEP) assert(0 && "the binary_tree is too deep");\
    }\
    do {\
      _pathi_ --;\
      if ((order==1 && _pass_[_pathi_] == 0)\
          || (order == 2 && (_pass_[_pathi_] == 1 || _right_[_pathi_]==NULL))) {\
        iteratedVariable = _path_[_pathi_];\
        {command;}\
      }\
      _pass_[_pathi_] ++;\
    } while (_pathi_>0 && _right_[_pathi_]==NULL) ;\
    _cn_ = _right_[_pathi_];\
    _right_[_pathi_] = NULL;\
    _pathi_ ++;\
  }\
}

#define BIN_TREE_MAP_ON_ELEMENTS(type, tree, _current_element_, left, right, command) {\
  __BIN_TREE_MAP_ON_ELEMENTS(type, tree, _current_element_, 1, left, right, command);\
}

#define BIN_TREE_MAP_ON_ELEMENTS_PREORDER(type, tree, _current_element_, left, right, command) {\
  __BIN_TREE_MAP_ON_ELEMENTS(type, tree, _current_element_, 0, left, right, command);\
}

#define BIN_TREE_MAP_ON_ELEMENTS_POSTORDER(type, tree, _current_element_, left, right, command) {\
  __BIN_TREE_MAP_ON_ELEMENTS(type, tree, _current_element_, 2, left, right, command);\
}

#endif
