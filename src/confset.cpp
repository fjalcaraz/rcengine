/**
 * @file confset.cpp
 * @author Francisco Alcaraz
 * @brief Class that manage the Conflict Sets, these are the rules that, as a result of an event propagations, has
 *    verified their conditions (LHS of rule) and they are ready to be triggered (execution of RHS of rule)
 *    The conflict set order the rules according to 3 levels of priority: 0 HIGH, 1 NORMAL, 2 LOW
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "compound.hpp"
#include "confset.hpp"
#include "nodes.hpp"

// Three levels of priority
PRIVATE ConflictSet *conflict_set[3] = {NULL};

/**
 * @brief Construct a new Conflict Set:: Conflict Set object
 * 
 * @param tag Propagated event type (insert, modification, delete)
 * @param prod_node Production Node in the net that has been reached
 * @param prod_compound Compound created that will maintain, at its left, the matching element of teh LHS of rule),
 *       and, at its right, the elements of the RHS of rule
 */
ConflictSet::ConflictSet(int tag, Node *prod_node, Compound *prod_compound)
{
  _tag = tag;
  _prod_node = prod_node;
  _prod_compound = prod_compound;
  _prev = NULL;
  _next = NULL;
  _inserted = FALSE;
 
}

/**
 * @brief Insert this ConflictSet in a certain priority queue. 
 *        The queues are LIFO to go deeper in the propagation of the current event and its consequences
 * 
 * @param cat Category == Priority of the rule
 */
void ConflictSet::insert(int cat)
{
  _next = conflict_set[cat];
  _prev = NULL;

  if (_next != NULL)
    _next->_prev = this;

  conflict_set[cat] = this;
  _inserted = TRUE;
}

/**
 * @brief Remove this ConflictSet of the queue of a certain priority (or category)
 * 
 * @param cat Category == Priority of the rule
 */
void ConflictSet::remove(int cat)
{
  if (_inserted)
  {
    if (_prev != NULL)
      _prev->_next = _next;
    else
      conflict_set[cat] = _next;
 
    if (_next != NULL)
      _next->_prev = _prev;
  }
  delete this;
}

/**
 * @brief Simple compare function based in rod_nodes memory address and secondary in comparing left sides
 * 
 * @param cset1 
 * @param cset2 
 * @param list Variable list of additional arguments to be passed to the compare method
 * @return int 
 */
int ConflictSet::compare_cset(const ConflictSet *cset1, 
                              const ConflictSet *cset2, va_list list)
{
  int res;

  if ((res = (char *)cset1->_prod_node - (char *)cset2->_prod_node) == 0)
    res = cset2->_prod_compound->left()->compare(cset1->_prod_compound->left(), list);
  return res;
}

/**
 * @brief Gets the next ConflictSet to be executed according to category (priority) and position in the queue
 *        The ConflictSet is not removed form tha queue
 * 
 * @param categoria Where to store the category chosen 
 * @return ConflictSet* 
 */
ConflictSet *ConflictSet::best_cset(int *categoria)
{
  int cat;
  ConflictSet *cs;

  for(cat=0; cat < 3 && (cs=conflict_set[cat]) == NULL; cat++);
  (*categoria) = cat;
  return cs;
}

