/**
 * @file confset.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the confset module. Definition of the ConflictSet class
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef ERROR
#define ERROR -1
#endif

#ifndef PUBLIC
#define PUBLIC
#define PRIVATE static
#endif

#include "metaobj.hpp"
#include "nodes.hpp"

#ifndef CONFSET_HH_INCLUDED
#define CONFSET_HH_INCLUDED

class ConflictSet
{
private:
  int _tag;
  int _inserted;
  Node *_prod_node;
  Compound *_prod_compound;
  ConflictSet *_prev;
  ConflictSet *_next;

public:
  ConflictSet(int tag, Node *prod_node, Compound *prod_compound);

  void insert(int cat);
  void remove(int cat);
  static int compare_cset(const ConflictSet *item1,
                          const ConflictSet *item2,
                          va_list list);
  Compound *prod_compound() { return _prod_compound; };
  int tag() { return _tag; };
  static ConflictSet *best_cset(int *categoria);
  void execute(void (Node::*f)(int, Compound *))
  {
    (_prod_node->*f)(_tag, _prod_compound);
  };
};

#endif
