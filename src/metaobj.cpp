/**
 * @file metaobj.cpp
 * @author Francisco Alcaraz
 * @brief Superclass of the objects that go through the nodes net and that carry the final user objects
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>

#include "engine.h"

#include "btree.hpp"
#include "single.hpp"
#include "compound.hpp"
#include "set.hpp"
#include "load.hpp"

/**
 * @brief Calculates the final time window when joining this MetaObj to another and having into account if this 
 *    or the other object are timed or not in the current rule
 * 
 * @param other The other object
 * @param t1 The final lower limit of the time window
 * @param t2 The final upper limit of the time window
 * @param w_this If this MetaObj must be considered timed
 * @param w_other If other MetaObj must be considered timed
 */
void
MetaObj::comp_window(MetaObj *other, long &t1, long &t2, int w_this, int w_other)
{

  long t1_this, t2_this, t1_other, t2_other;
 
  
  t1_this = (w_this ? _t1 : LONG_MAX);
  t2_this = (w_this ? _t2 : 0);
  t1_other= (w_other ? other->_t1 : LONG_MAX);
  t2_other= (w_other ? other->_t2 : 0);
 
  t1 = min(t1_this, t1_other);
  t2 = max(t2_this, t2_other);

}

/**
 * @brief Top level compare function that will compare two MetaObjs by the 
 *        memory addresses of singles or Set (simple meta objects)
 * 
 * @param c_obj1 The first object 
 * @param c_obj2 The other object
 * @param list variable list of arguments to be passed o the compare function
 * @return int <0, 0, or >0
 */
// static
int
MetaObj::metacmp(const void *c_obj1, const void *c_obj2, va_list list)
{
   // Segun la funcion de comparacion del obj en el arbol (c_obj2)
   return ((MetaObj *)c_obj2)->compare((MetaObj *&)c_obj1, list);
}

/**
 * @brief Top level compare function that will compare two MetaObjs by the 
 *        memory addresses of the final user objects (ObjType)
 * 
 * @param c_obj1 The first object 
 * @param c_obj2 The other object
 * @param list variable list of arguments to be passed o the compare function
 * @return int <0, 0, or >0
 */
// static
int
MetaObj::metacmp_objs(const void *c_obj1, const void *c_obj2, va_list list)
{
   // Segun la funcion de comparacion del obj en el arbol (c_obj2)
   return ((MetaObj *)c_obj2)->compare_objs((MetaObj *&)c_obj1, 0, list);
}
 
/**
 * @brief Top level compare function that will compare two MetaObjs, first by the 
 *        t2 in the time window and second by the memory addresses of the final simple MetaObjs
 * 
 * @param c_obj1 The first object 
 * @param c_obj2 The other object
 * @param list variable list of arguments to be passed o the compare function
 * @return int <0, 0, or >0
 */
int
MetaObj::compare_t(const void *c_obj1, const void *c_obj2, va_list list)
{
  int res;
  // Para conseguir una ordenacion de mayor a menor T
  if ((res = ((MetaObj *)c_obj2)->_t2 - ((MetaObj *)c_obj1)->_t2) == 0)
     res = ((MetaObj *)c_obj2)->compare((MetaObj *&)c_obj1, list);

  return res;
}

/**
 * @brief Top level compare function that will compare t2 of c_obj2 (the objects in the tree) with a 
 *      given time limit (rule time window upper limit)
 *      This is very useful in those BTrees where the objects are ordered by t2 (compare_t) when, given
 *      an object (iven) at one side of a node, we need to get those objects of the other memory (a BTree) 
 *      whose t2 will be lower of given.t1 + rule.window (t2_limit)
 * 
 * @param c_obj1 The first object NOT USED
 * @param c_obj2 The other object
 * @param list variable list of arguments to be passed o the compare function: t2_limit must be passed
 * @return int <0, 0, or >0
 */
int
MetaObj::compare_tw(const void *c_obj1, const void *c_obj2, va_list list)
{
  va_list copy;

  va_copy(copy, list);
  const long t2_limit = va_arg(copy, long);
  va_end(copy);

  // It is ordered from higher to lower T
  return ((MetaObj *)c_obj2)->_t2 - t2_limit;
}

/**
 * @brief Convert a MetaObjet in an array of final user Objects (ObjType)
 * 
 * @return ObjectType** The final array
 */
ObjectType **MetaObj::meta2array()
{
   ObjectType **array;
   int n;

   array = (ObjectType**) malloc(sizeof(ObjectType*) * n_objs());

   n=0;
   this->fill_array(n, array);
   return array;
}

/**
 * @brief Order to free an struct recursively
 * 
 * @param item The item to be freed
 */
void 
MetaObj::metadelete(void* item, va_list)
{
  ((MetaObj *)item)->delete_struct(TRUE, FALSE);
}

/**
 * @brief Prints a structs and its final objects
 * 
 * @param item The MetaObj to print
 * @param list Variable list that is expected to go with a FILE * where to print and a function able to print an ObjType object
 */
void
MetaObj::print_tree(const void *item, va_list list)
{
  typedef void (*PrintObjFunc)(FILE *, const ObjectType *);
 
  FILE *fp;
  PrintObjFunc obj_print;
  va_list copy;

  va_copy(copy, list);
  fp = va_arg(copy, FILE *);
  obj_print = va_arg(copy, PrintObjFunc);
  va_end(copy);
  
  fprintf(fp, "\t"); ((MetaObj *)item)->print(fp, obj_print); fprintf(fp, "\n");
}

/**
 * @brief Increments the number of references (#links) to a MetaObj (e.g. a reference is that it is stored in a memory)
 *      This is important due the MetaObjs behave as "smart pointers" that only know about link() and unlink()
 *      their memory doesn't need to be managed externally. The memory is freed autiomatically when in a unlink()
 *      the #links reaches 0 
 * 
 */
void
MetaObj::link()
{
  _links++;
  if (trace >= 2){
    fprintf(trace_file, "## INC LINKS of %s %lx  to %d\n",(_type == SINGLE)?"SINGLE":(_type == COMPOUND)?"COMPOUND":"SET", (unsigned long int)this,_links);
    if (_type == SINGLE)
      fprintf(trace_file, "!! %lx\n", (unsigned long int)single()->obj());
  }
}

/**
 * @brief Decrements the number of references (#links) to a MetaObj (e.g. a reference is that it is stored in a memory)
 *      This is important due the MetaObjs behave as "smart pointers" that only know about link() and unlink()
 *      their memory doesn't need to be managed externally. The memory is freed autiomatically when in a unlink()
 *      the #links reaches 0 
 * 
 */
void
MetaObj::unlink()
{
  _links--;

  if (trace >= 2){
    fprintf(trace_file, "## DEC LINKS of %s %lx  to %d\n",(_type == SINGLE)?"SINGLE":(_type == COMPOUND)?"COMPOUND":"SET", (unsigned long int)this,_links);
    if (_type == SINGLE)
      fprintf(trace_file, "!! %lx\n", (unsigned long int)single()->obj());
  }

  if (trace >= 2 && _links == 0)
  {
    if (_type == SINGLE)
      fprintf(trace_file, "UNLINK : Se borra el single 0x%lx %s\n",  (unsigned long int)this, clave(this->single()->obj()));
    else if (_type == COMPOUND)
      fprintf(trace_file, "UNLINK : Se borra un COMPOUND 0x%lx\n",  (unsigned long int)this);
    else
      fprintf(trace_file, "UNLINK : Se borra un SET 0x%lx\n",  (unsigned long int)this);
  }

  if (_links == 0)
    delete this;
}

