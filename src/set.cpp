/**
 * @file set.cpp
 * @author Francisco Alcaraz
 * @brief Methods of the Set class, a kind of MetaObj to store multiple objects that verify a pattern
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <limits.h>
#include <string.h>

#include "engine.h"

#include "btree.hpp"
#include "single.hpp"
#include "set.hpp"
#include "nodes.hpp"

#ifndef FLT_MAX
#define FLT_MAX 3.40282347e+38
#endif

#ifndef FLT_MIN
#define FLT_MIN 1.17549435e-38
#endif

//
// SET CLASS METHODS
//

/**
 * @brief Compare function between two sets. Compares their memory addresses
 *
 * @param other_set The other Set
 * @return int <0, 0, >0
 */
int Set::compare(const MetaObj *other_set, va_list) const
{
  return ((char *)other_set->set() - (char *)this);
}

/**
 * @brief Compare function between an object and a this Set.
 *        In case the set has some internal conditions there are several sets
 *        and this function must find the set where the object must be stored
 *
 * @param obj1 input MetaObj
 * @param pos_offset Position of the object in the set (not used)
 * @param list Variable argument list
 * @return int <0, 0, >0
 */
int Set::compare_objs(const MetaObj *obj1, int pos_offset, va_list list) const
{
  va_list copy;

  va_copy(copy, list);
  bool *masked = va_arg(copy, bool *);
  ULong *masks = va_arg(copy, ULong *);
  ULong *code_p = va_arg(copy, ULong *);
  ULong *end_of_code = va_arg(copy, ULong *);
  va_end (copy);

  if (obj1->class_type() == SET)
    return ((char *)obj1->set() - (char *)this);

  // The comparison between the SET an the Single is done by the internal code of the SET node
  if (end_of_code > code_p && n_objs() > 0 &&
      (obj1->class_type() != SINGLE || obj1->single()->obj() != NULL))
  {
    const MetaObj *item = first_item_of_set();
    int res;

    res = Node::execute_cmp(code_p, end_of_code, (MetaObj *)obj1, (MetaObj *)item);

    return res;
  }

  /* else */
  return 0; // If there is no code, there are no discrimination and the item belongs to the set
}

/**
 * @brief Return how many items are stored in the set
 *
 * @return int
 */
int Set::n_objs() const
{
  return _mem.numItems();
}

/**
 * @brief MetaObj inherithed method to access to the MetaObj as array.
 *        n must be 0 due the SET as array has length 1 (as Singles do)
 *
 * @param n Index of teh MetaObj as array
 * @return MetaObj*
 */
MetaObj *
Set::operator[](const int n)
{
  if (n != 0)
    engine_fatal_err("Out or range in Set[]\n");

  return this;
}

/**
 * @brief MetaObj inherithed method to duplicate the MetaObj struct
 *
 */
MetaObj *
Set::duplicate_struct(int /* link_singles */)
{
  // The sets are not duplicated but it is linked to reflect the new reference

  link();
  return this;
}

/**
 * @brief MetaObj inherithed method to delete a MetaObj struct
 *
 * @return this object if it has other links
 */
MetaObj *
Set::delete_struct(int /* unlink_singles */, int /* only_minimize_rhs */)
{
  // The sets are not deleted, are unlinked

  int n = links();

  unlink();
  return (n > 1) ? this : NULL;
}

/**
 * @brief MetaObj inherithed method to get the address where a single item or set is stored
 *
 * @param dir Where this set is stored
 * @param n Position. Must be 0
 * @param n_objs Number de objects. Must be 1
 * @return MetaObj** The address where stored (dir)
 */
MetaObj **
Set::meta_dir(MetaObj **dir, const int n, const int n_objs)
{
  if (n != 0 || n_objs != 1)
    engine_fatal_err("Out of range (n=%d, n_objs=%d) at Set::meta_dir\n",
                     n, n_objs);
  return dir;
}

/**
 * @brief MetaObj inherithed method to fill an array with all the objects in that MetaObj
 *
 * @param n Number of items returned
 * @param array Where to leave the objects
 */
void Set::fill_array(int &n, ObjectType *array[])
{
  MatchCount *counter;
  BTState state = _mem.getIterator();
  while (counter = (MatchCount *)BTree::Walk(state))
    counter->item->fill_array(n, array);
}

/**
 * @brief Sum all the integer values of some attribute for all the items stored in this Set
 *
 * @param n_attr Number of attribute
 * @return long the resulting sum
 */
long Set::sum_set_n(int n_attr)
{
  long res = 0;
  MatchCount *counter;
  BTState state = _mem.getIterator();

  while (counter = (MatchCount *)BTree::Walk(state))
    res += counter->item->single()->obj()->attr[n_attr].num;

  return res;
}

/**
 * @brief Sum all the float values of some attribute for all the items stored in this Set
 *
 * @param n_attr Number of attribute
 * @return float the resulting sum
 */
float Set::sum_set_f(int n_attr)
{
  float res = 0;
  MatchCount *counter;
  BTState state = _mem.getIterator();

  while (counter = (MatchCount *)BTree::Walk(state))
    res += counter->item->single()->obj()->attr[n_attr].flo;

  return res;
}

/**
 * @brief Multiply all the integer values of some attribute for all the items stored in this Set
 *
 * @param n_attr Number of attribute
 * @return long the resulting product
 */
long Set::prod_set_n(int n_attr)
{
  long res = 1;
  MatchCount *counter;
  BTState state = _mem.getIterator();

  while (counter = (MatchCount *)BTree::Walk(state))
    res *= counter->item->single()->obj()->attr[n_attr].num;

  return res;
}

/**
 * @brief Multiply all the float values of some attribute for all the items stored in this Set
 *
 * @param n_attr Number of attribute
 * @return float the resulting product
 */
float Set::prod_set_f(int n_attr)
{
  float res = 1;
  MatchCount *counter;
  BTState state = _mem.getIterator();

  while (counter = (MatchCount *)BTree::Walk(state))
    res *= counter->item->single()->obj()->attr[n_attr].flo;

  return res;
}

/**
 * @brief Calculate the minimum integer value of some attribute for all the items stored in this Set
 *
 * @param n_attr Number of attribute
 * @return long The minimum found
 */
long Set::min_set_n(int n_attr)
{
  long res = LONG_MAX;
  MatchCount *counter;
  BTState state = _mem.getIterator();

  while (counter = (MatchCount *)BTree::Walk(state))
  {
    if (res > counter->item->single()->obj()->attr[n_attr].num)
      res = counter->item->single()->obj()->attr[n_attr].num;
  }

  return res;
}

/**
 * @brief Calculate the minimum float value of some attribute for all the items stored in this Set
 *
 * @param n_attr Number of attribute
 * @return float The minimum found
 */
float Set::min_set_f(int n_attr)
{
  float res = FLT_MAX;
  MatchCount *counter;
  BTState state = _mem.getIterator();

  while (counter = (MatchCount *)BTree::Walk(state))
  {
    if (res > counter->item->single()->obj()->attr[n_attr].flo)
      res = counter->item->single()->obj()->attr[n_attr].flo;
  }

  return res;
}

/**
 * @brief Calculate the minimum string value of some attribute for all the items stored in this Set
 *
 * @param n_attr Number of attribute
 * @return char * The minimum found
 */
char *
Set::min_set_a(int n_attr)
{
  char *res = NULL;
  MatchCount *counter;
  BTState state = _mem.getIterator();

  while (counter = (MatchCount *)BTree::Walk(state))
  {
    char *str;
    str = counter->item->single()->obj()->attr[n_attr].str.str_p;
    if (str != NULL && (res == NULL || strcmp(str, res) < 0))
      res = str;
  }

  if (res == NULL)
    return NULL;
  else
    return strdup(res);
}

/**
 * @brief Calculate the maximum integer value of some attribute for all the items stored in this Set
 *
 * @param n_attr Number of attribute
 * @return long The maximum found
 */
long Set::max_set_n(int n_attr)
{
  long res = LONG_MIN;
  MatchCount *counter;
  BTState state = _mem.getIterator();

  while (counter = (MatchCount *)BTree::Walk(state))
  {
    if (res < counter->item->single()->obj()->attr[n_attr].num)
      res = counter->item->single()->obj()->attr[n_attr].num;
  }

  return res;
}

/**
 * @brief Calculate the maximum float value of some attribute for all the items stored in this Set
 *
 * @param n_attr Number of attribute
 * @return float The maximum found
 */
float Set::max_set_f(int n_attr)
{
  float res = FLT_MIN;
  MatchCount *counter;
  BTState state = _mem.getIterator();

  while (counter = (MatchCount *)BTree::Walk(state))
  {
    if (res < counter->item->single()->obj()->attr[n_attr].flo)
      res = counter->item->single()->obj()->attr[n_attr].flo;
  }

  return res;
}

/**
 * @brief Calculate the maximum string value of some attribute for all the items stored in this Set
 *
 * @param n_attr Number of attribute
 * @return char * The maximum found
 */
char *
Set::max_set_a(int n_attr)
{
  char *res = NULL;
  MatchCount *counter;
  BTState state = _mem.getIterator();

  while (counter = (MatchCount *)BTree::Walk(state))
  {
    char *str;
    str = counter->item->single()->obj()->attr[n_attr].str.str_p;
    if (str != NULL && (res == NULL || strcmp(str, res) > 0))
      res = str;
  }

  if (res == NULL)
    return NULL;
  else
    return strdup(res);
}

/**
 * @brief Concatenate the string values of some attribute for all the items stored in this Set
 *
 * @param n_attr Number of attribute
 * @param sep String separator between values
 * @return char * The result of concatenation
 */
char *
Set::concat(int n_attr, char *sep)
{
  int len, len_sep, len_str, init;
  char *buffer;
  int n_chunks;
  MatchCount *counter;
  BTState state;

  buffer = NULL;
  len = 0;
  len_sep = strlen(sep);
  n_chunks = 0;

  state = _mem.getIterator();
  init = TRUE;
  while (counter = (MatchCount *)BTree::Walk(state))
  {
    char *str;

    str = counter->item->single()->obj()->attr[n_attr].str.str_p;
    len_str = (str != NULL) ? strlen(str) : 0;

    if (len + len_sep + len_str + 1 > n_chunks * 100)
      buffer = (char *)realloc(buffer, sizeof(char) * 100 * ++n_chunks);

    if (!init)
    {
      strcpy(buffer + len, sep);
      len += len_sep;
    }
    if (str != NULL)
      strcpy(buffer + len, str);
    len += len_str;
    init = FALSE;
  }
  return buffer;
}

/**
 * @brief Get the first item of set
 *        Indeed it is just a sample, not the first in order
 *
 * @return MetaObj*
 */
MetaObj *
Set::first_item_of_set() const
{
  MetaObj *item;

  item = (Single *)((MatchCount *)_mem.getElement())->item;
  return item;
}

/**
 * @brief Change the status of he MetaObj to its old state OLD_ST (with the object before operation/tag)
 *        or to its new state NEW_ST after operation/tag. The set to OLD_ST means to invert the current tag
 *        that means that the OLD_ST of an INSERT_TAG is a RETRACT_TAG and viceversa
 *        In modification the tag inversion does nothing but the data object is switched between its old appearance and its new one
 *        See setOp in hdrs/set.hdd to indicate the desired_tag before to be performed
 *
 * @param ost
 * @param st
 * @param pos
 */
void Set::set_state(ObjState ost, Status *st, int pos)
{
  int desired_tag;

  if (_last_op != 0)
  {
    _last_obj->set_state(ost, st, pos); // This is the unique done on MODIFY

    desired_tag = _last_op;

    if (ost == OLD_ST)
      desired_tag ^= 0x3; // Tag inversion (last 2 bits inversion)

    if (_state != desired_tag)
    {
      if (desired_tag == INSERT_TAG)
        add_elem(_last_obj);

      if (desired_tag == RETRACT_TAG)
        delete_elem(_last_obj);
    }
  }
}

/**
 * @brief Add an element to a Set
 *
 * @param elem Element to be added
 * @return MetaObj* The item inserted
 */
MetaObj *
Set::add_elem(MetaObj *elem)
{
  MatchCount **res;

  if (elem == Single::null_single())
    return elem;

  res = (MatchCount **)_mem.Insert(elem, Node::cmp_count_with_object);

  if (((MetaObj *)*res) == elem && !BTree::WasFound())
    *res = new MatchCount(elem);
  else
    (*res)->count++;

  _state = INSERT_TAG;
  _last_obj = elem;
  (*res)->item->link();

  return (*res)->item;
}

/**
 * @brief Find an element
 *
 * @param elem The element to find
 * @return MetaObj* The element found
 */
MetaObj *
Set::find_elem(MetaObj *elem)
{
  MatchCount *res;

  if (elem == Single::null_single())
    return elem;

  res = (MatchCount *)_mem.Find(elem, Node::cmp_count_with_object);
  if (!res)
    return NULL;
  else
    return res->item;
}

/**
 * @brief Delete an element in the Set
 *
 * @param elem The element to delete
 * @return MetaObj* the object found (or NULL if wasn't)
 */
MetaObj *
Set::delete_elem(MetaObj *elem)
{
  MatchCount *res;

  if (elem == Single::null_single())
    return elem;

  res = (MatchCount *)_mem.Find(elem, Node::cmp_count_with_object);
  if (res)
  {
    _last_obj = res->item;

    if (res->count == 0)
    {
      res = (MatchCount *)_mem.Delete(elem, Node::cmp_count_with_object);
      delete (res);
    }
    else
      res->count--;

    _state = RETRACT_TAG;
    _last_obj->unlink();
    return _last_obj;
  }
  else
    return NULL;
}

/**
 * @brief Print in a FILE the object 
 * 
 * @param fp FILE to print
 * @param obj_print Function to print a data Object
 */
void Set::print(FILE *fp, void (*obj_print)(FILE *, const ObjectType *)) const
{
  MatchCount *counter;

  fprintf(fp, "SET (nobj=%d Time=%ld){\n", n_objs(), ((MetaObj *)this)->t1());
  BTState state = _mem.getIterator();

  while (counter = (MatchCount *)BTree::Walk(state))
  {
    fprintf(fp, "\t COUNT=%d ", counter->count);
    counter->item->print(fp, obj_print);
    fprintf(fp, "\n");
  }
  fprintf(fp, "}");
}

/**
 * @brief Function to get the Set memory (BTree)
 * 
 * @return BTree* The tree of the Set
 */
BTree *
Set::get_tree()
{
  return &_mem;
}
