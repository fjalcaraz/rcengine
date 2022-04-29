/**
 * @file single.cpp
 * @author Francisco Alcaraz
 * @brief Management of the MetaObj subclass Single, use to encapsulate simple Objects
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <stdio.h>
#include <string.h>

#include "engine.h"
#include "single.hpp"

#define STR(a) ((a == NULL) ? "(null)" : a)

Single Single::_null_single;

/**
 * @brief Compare two Singles. The comparison is made comparing the
 *        Object Memory Address that keep inside (_key)
 *
 * @param obj2 Meta Object (Single) to compare to
 * @return int
 */
int Single::compare(const MetaObj *obj2, va_list) const
{
  return obj2->single()->_key - _key;
}

/**
 * @brief Compare two single comparing its objects addresses
 *      The general way to compare this is ussing _key that store the memory addres of the final obj
 *      The object may be changed by a copy when managing the OLD state, so comparing _obj is not
 *      feasible.
 *
 * @param obj2
 * @param pos_offset
 * @param list
 * @return int
 */
int Single::compare_objs(const MetaObj *obj2, int pos_offset, va_list list) const
{
  int res;

  if (((char *)obj2->single()->_obj  - (char *)_obj)== 0)
    return 0;
  else
  if ((res = obj2->single()->_key - _key) == 0)
    return 0;
  else
  {
    // If th object is masked because it is going to be a SET return as found
    va_list copy;
    va_copy(copy, list);
    bool *masked = va_arg(copy, bool *);
    ULong *masks = va_arg(copy, ULong *);
    va_end (copy);
    if (masks && Node::getPatternMask(masks, pos_offset))
    {
      *masked = true;
      return 0;
    }
    else
      return res;
  }
}

/**
 * @brief MetaObj inherithed method to access to the MetaObj as array.
 *        n must be 0 due the Single as array has length 1 (as Sets do)
 *
 * @param n Index of teh MetaObj as array
 * @return MetaObj*
 */
MetaObj *
Single::operator[](const int n)
{
  if (n != 0)
    engine_fatal_err("Out of Range Single[]\n");

  return this;
}

/**
 * @brief MetaObj inherithed method to duplicate the MetaObj struct
 *
 */
MetaObj *
Single::duplicate_struct(int link_singles)
{

  // The Singles are not duplicated but it is linked to reflect the new reference
  if (link_singles)
    link();

  return this;
}

/**
 * @brief MetaObj inherithed method to delete a MetaObj struct
 *
 * @return this object if it has other links
 */
MetaObj *
Single::delete_struct(int unlink_singles, int only_minimize_rhs)
{
  int n = links();

  // The singles are not deleted, are unlinked
  if (unlink_singles)
  {
    unlink();
    return (n > 1) ? this : NULL;
  }

  if (only_minimize_rhs)
    return (n > 1) ? this : NULL;

  return this;
}

/**
 * @brief MetaObj inherithed method to get the address where a single item or set is stored
 *
 * @param dir Where this Single is stored
 * @param n Position. Must be 0
 * @param n_objs Number de objects. Must be 1
 * @return MetaObj** The address where stored (dir)
 */
MetaObj **
Single::meta_dir(MetaObj **dir, const int n, const int n_objs)
{
  if (n != 0 || n_objs != 1)
    engine_fatal_err("Out of range (n=%d, n_objs=%d) at Single::meta_dir\n",
                     n, n_objs);
  return dir;
}

/**
 * @brief Return how many items are stored in the MetaObject
 *
 * @return int = 1 unless _obj would be null (0 returned)
 */
int Single::n_objs() const
{
  return (_obj != NULL) ? 1 : 0;
}

/**
 * @brief MetaObj inherithed method to fill an array with all the objects in that MetaObj
 *
 * @param n Number of items returned
 * @param array Where to leave the objects
 */
void Single::fill_array(int &n, ObjectType *array[])
{
  if (_obj != NULL)
    array[n++] = _obj;
}

/**
 * @brief Change the status of he MetaObj to its old state OLD_ST (with the object before operation/tag)
 *        or to its new state NEW_ST after operation/tag. 
 *        In Singles, the data object _obj is switched between its old appearance and its new one that
 *        are stored in the Status
 *
 * @param ost Obj derired state
 * @param st Status where the old 
 * @param pos position
 */
void Single::set_state(ObjState ost, Status *st, int)
{
  // TODO: see if managing the position it is possible to manage the flip between NULL (nill single)
  // and an object (or viceversa), affecting th _key and making unnecessary null_single
  if ((void *)_key != NULL && _key == (long int)(st->_obj))
  {
    if (ost == OLD_ST)
      _obj = st->_old_obj;
    else
      _obj = st->_obj;
  }
}

/**
 * @brief Print in a FILE the object 
 * 
 * @param fp FILE to print
 * @param obj_print Function to print a data Object
 */
void Single::print(FILE *fp, void (*obj_print)(FILE *, const ObjectType *)) const
{
  if (trace >= 2)
  {
    fprintf(fp, "{ this=%lx links=%d t1=%ld key=%lx obj=(", (unsigned long int)this, _links, ((MetaObj *)this)->t1(), _key);
  }
  (*obj_print)(fp, _obj);
  if (trace >= 2) {
    fprintf(fp, ") key=(");
    (*obj_print)(fp, (ObjectType *)_key);
    fprintf(fp, ")}");
  }
}
