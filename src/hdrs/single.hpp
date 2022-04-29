/**
 * @file single.hpp
 * @author Francisco Alcaraz
 * @brief Class Single definition
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef NULL
#define NULL    0
#endif

#ifndef ERROR
#define ERROR   -1
#endif

#ifndef PUBLIC
#define PUBLIC
#define PRIVATE static
#endif


#include <stdio.h>
#include <limits.h>

#include "engine.h"
#include "error.hpp"
#include "metaobj.hpp"
#include "load.hpp"

#ifndef SINGLE__HH_INCLUDED
#define SINGLE__HH_INCLUDED


class Single: public MetaObj
{
  private:
    static Single _null_single;
    long int _key;
    ObjectType *_obj;
  public:
    inline Single(ObjectType *object);
    inline Single();
    ObjectType *obj() 			{ return _obj; };
    static Single *null_single()	{ return &_null_single; };
    void set_deleted()                  { _key = 0; };
    int has_been_deleted()              { return (_key == 0); };

    // Virtual functions inherited
    int compare(const MetaObj *obj2, va_list list) const;
    int compare_objs(const MetaObj *obj2, int pos_offset, va_list list) const;
    MetaObj * operator[](const int n);
    MetaObj **meta_dir(MetaObj **dir, const int n, const int n_objs);
    int n_objs() const;
    void fill_array(int &n, ObjectType *array[]);
    MetaObj * duplicate_struct(int link_singles);
    MetaObj * delete_struct(int unlink_singles, int only_minmmize_rhs);
    void set_state(ObjState ost, Status *st, int pos);

    void print(FILE *fp, void (*obj_print)(FILE *, const ObjectType *)) const;
};
      
// inline  methods

inline
Single::Single(ObjectType *object) 
{ 
  _type = SINGLE; 
  _obj = object; 
  _t1 = _t2 = _obj->time;
  _key = (long int)object;
  _links = 1;
  if (trace >= 2)
     fprintf(trace_file, "### new Sg %lx obj %lx\n", (long unsigned int)this, (long unsigned int)_obj);
}

inline
Single::Single()
{
  _type = SINGLE;
  _t1 = LONG_MAX;
  _t2 = 0;
  _obj = NULL;
  _key = 0;
  _links = 1;
  if (trace >= 2)
     fprintf(trace_file, "### new Sg %lx obj %lx\n", (long unsigned int)this, (long unsigned int)_obj);
}

#endif
