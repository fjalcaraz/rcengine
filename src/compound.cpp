/**
 * @file compound.cpp
 * @author Francisco Alcaraz
 * @brief Compound Objects a subclass of MetaObjs. 
 *    These objects are able to store two meta objects, one at the right and one at the left. 
 *    Are used as tuples of two Singles meta-objects that made matching in the left hand part of some rule. 
 *    In case of needing to link a third meta object, a new compound on top will link the existing Compound tuple by the left
 *    and that third object by its right, so the order is maintained 
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "engine.h"
#include "single.hpp"
#include "compound.hpp"
#include "load.hpp"

//
// CLASS COMPOUND
//

/**
 * @brief Construct a new Compound:: Compound object
 * 
 * @param left MetaObject at the left
 * @param right MetaObject at the right
 * @param t1 Lower boundary of the time window of the couple
 * @param t2 Higher boundary of the time window of the couple
 */
Compound::Compound(MetaObj *left, MetaObj *right, long t1, long t2)
{
  _type = COMPOUND;
  _links = 1;
  _left   = left;
  _right  = right;

  _t1 = t1;
  _t2 = t2;
 
  if (_left->class_type() != COMPOUND)
    _n_left = 1;
  else
    _n_left = _left->compound()->_n_left +
              _left->compound()->_n_right;
 
  if (_right->class_type() != COMPOUND)
    _n_right=1;
  else
    _n_right =_right->compound()->_n_left +
              _right->compound()->_n_right;

  if (trace >= 2)
    fprintf(trace_file, "### new Comp %lx\n", (unsigned long int)this);

}

/**
 * @brief Construct a new Compound:: Compound object with no right item
 * 
 * @param left MetaObj at the left
 */
Compound::Compound(MetaObj *left)
{
  _type = COMPOUND;
  _links = 1;
  _left   = left;
  _right  = NULL;
 
  _t1 = _left->compound()->_t1;
  _t2 = _left->compound()->_t2;

  if (_left->class_type() != COMPOUND)
    _n_left=1;
  else
    _n_left = _left->compound()->_n_left +
              _left->compound()->_n_right;
 
  _n_right=0;

  if (trace >= 2)
    fprintf(trace_file, "### new Comp %lx\n", (unsigned long int)this);
}
  
/**
 * @brief Construct a new Compound:: Compound object from another Compound (Copy Construct)
 * 
 * @param other The original Compound
 */
Compound::Compound(Compound &other)
{
  _type = COMPOUND;

  _links= 1;

  _t1   = other._t1;
  _t2   = other._t2;

  _n_left = other._n_left;
  _n_right= other._n_right;

  _right  = other._right;
  _left   = other._left;

  if (trace >= 2)
    fprintf(trace_file, "### new Comp %lx\n", (unsigned long int)this);
}

/**
 * @brief Creeate a new compound made by the union of this by left and another MetaObj by right
 * 
 * @param right_to_join The MetaObj that will go to the right
 * @return MetaObj* New compound created
 */
MetaObj *
Compound::join_by_right_untimed(MetaObj *right_to_join)
{

  if (this == NULL)
    return right_to_join;

  else
    return new Compound(this, right_to_join);
}

/**
 * @brief Basic compare function among Compounds based on the memory addresses of the elements at left and right
 * 
 * @param c_obj2 The other Compound that is compared with this
 * @return int <0 , 0 or >0
 */
int 
Compound::compare(const MetaObj *c_obj2, va_list) const
{
  int res;

  if ((res = (char *)c_obj2->compound()->_left - (char *)_left) == 0)
    res = (char *)c_obj2->compound()->_right - (char *)_right;
  return (res);
}

/**
 * @brief Compare function that go deeper to compare the addresses of the final objects
 *    As is suppose that similar MetaObjs structures are compared this Compound is compared with other compound
 *    This compare_objs function is a inherited abstract method from MetaObj so is implemented in every subclass
 * 
 * @param c_obj2 the compound of teh other structire being compared with
 * @param pos_offset Offset in the position of the objects
 * @param list Variable list of arguments to be passed to the compare_objs function
 * @return int <0, 0, or > 0
 */
int
Compound::compare_objs(const MetaObj *c_obj2, int pos_offset, va_list list) const
{
  int res;

  if ((res = _left->compare_objs(c_obj2->compound()->_left, pos_offset, list)) == 0)
    res = _right->compare_objs(c_obj2->compound()->_right, pos_offset+_n_left, list);
  return res;
}
/**
 * @brief Compare function of obj1 with the left of ojs2
 * 
 * @param obj1 Object to be compere with left part of obj2
 * @param obj2
 * @param list Variable list of arguments to be passed to the compare function
 * @return int 
 */
// static
int 
Compound::compare_with_left(const MetaObj *obj1, const MetaObj *obj2, va_list list)
{
  return obj2->compound()->_left->compare(obj1, list);
}

/**
 * @brief Compare function of the left of obj1 with the left of ojs2
 * 
 * @param obj1 
 * @param obj2
 * @param list Variable list of arguments to be passed to the compare function
 * @return int 
 */
// static
int 
Compound::compare_left_with_left(const MetaObj *obj1, const MetaObj *obj2, va_list list)
{
  return obj2->compound()->_left->compare(obj1->compound()->_left, list);
}

/**
 * @brief Operator [] to manage an structure of nested compounds as an array
 * 
 * @param n Index of the element into the array
 * @return MetaObj* The MetaObj (Single/Set) found at that position
 */
MetaObj *
Compound::operator[](const int n)
{

  if (n>=_n_left)
    return (*_right)[n - _n_left];
  else
    return (*_left)[n];
}

/**
 * @brief Inherited abstract method that fills an structure with the values of an array
 * 
 * @param n the counter of the filled elements
 * @param array of element to fill the structure with
 */
void 
Compound::fill_array(int &n, ObjectType *array[])
{
   if (_n_left > 0)
      _left->fill_array(n, array);
   if (_n_right > 0)
      _right->fill_array(n, array);
}

/**
 * @brief Inherited abstract method that counts all the objects in the structure (that made matching in the rule)
 * 
 * @return int number of objects
 */
int
Compound::n_objs() const
{
  int n_objs = 0;

  if (_n_left > 0)
      n_objs += _left->n_objs();
  if (_n_right > 0)
      n_objs += _right->n_objs();
  return n_objs;
}

/**
 * @brief Inherited abstract method that duplicates an MetaObj struct
 * 
 * @param link_singles Whether just link (true) or duplicate also (false) the singles found
 * @return MetaObj* The duplicated struct
 */
MetaObj*
Compound::duplicate_struct(int link_singles)
{
  Compound *res;
  res = new Compound(*this);
  res->_left = _left->duplicate_struct(link_singles);
  res->_right= _right->duplicate_struct(link_singles);
  return res;
}

/**
 * @brief Inherited abstract method that delete a struct (unlink = reduce #links in 1 and free memory in case #links reach 0)
 * 
 * @param unlink_singles If the singles must be unlinked to or just the Compounds
 * @param only_minimize_rhs: This is done to free the RHS of a rule (the action taken)
 * @return MetaObj* the same compound if was nor free (had other links) or null
 */
MetaObj *
Compound::delete_struct(int unlink_singles, int only_minimize_rhs)
{
  if (_left != NULL)
     _left = _left ->delete_struct(unlink_singles, only_minimize_rhs);

  if (_right!= NULL)
     _right = _right->delete_struct(unlink_singles, only_minimize_rhs);
  
  if (! only_minimize_rhs || (_left == NULL && _right == NULL)) {
    int n = links();
    unlink();
    return (n>1) ? this : NULL;
  }
  else return(this);
}

/**
 * @brief Inherited abstract method that return where the single at a certain position is strored
 * 
 * @param dir Where is stored this Compound (in a superior Compound)
 * @param n the index found
 * @param n_objs Total umber of objects in the MetaObj structure
 * @return MetaObj** The address where the searched MetaObj is stored
 */
MetaObj **
Compound::meta_dir(MetaObj **dir, const int n, const int n_objs)
{
  if (n_objs < _n_left + _n_right)
  {
    if (n >= _n_left)      
      return _right->meta_dir(&_right, n - _n_left, n_objs);
    else
      return _left->meta_dir(&_left, n, n_objs);
  }
  else return dir;
}

/**
 * @brief Inherited abstract method that allow to have two versions or the struct OLD_ST y NEW_ST. 
 *    It is especially required in modification of a Single object
 * 
 * @param ost State wanted
 * @param st Status where the changes in Single is stored
 * @param pos Position (considering the struct as an array) where the modified Single is
 */
void
Compound::set_state(ObjState ost, Status *st, int pos)
{
  _left->set_state(ost, st, pos);
  pos -= _n_left;

  if (pos == 0 && _n_right == 1 && 
     (st->_old_single == Single::null_single() || st->_single == Single::null_single()))
  {
      if (ost == OLD_ST)
        _right = st->_old_single;
      else if (ost == NEW_ST)
        _right = st->_single;
  }

  _right->set_state(ost, st, pos);
}

/**
 * @brief Print an struct
 * 
 * @param fp FILE struct where to print it
 * @param obj_print Print function for the final ObjectTypes
 */
void 
Compound::print(FILE *fp, void (*obj_print)(FILE *, const ObjectType *)) const
{
  fprintf(trace_file, "[");
  _left->print(fp, obj_print);
  fprintf(trace_file, "]-[");
  if (_right != NULL)
    _right->print(fp, obj_print);
  fprintf(trace_file, "]");
}


