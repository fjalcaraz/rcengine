/**
 * @file metaobj.hpp
 * @author Francisco ALcaraz
 * @brief Class MetaObj definition
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */


class MetaObj;

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
#include <stdlib.h>
#include <stdarg.h>

#include "engine.h"
 
#ifndef METAOBJ_HH_INCLUDED
#define METAOBJ_HH_INCLUDED


class Set;
class Single;
class Compound;
struct Status;

typedef enum { SINGLE, COMPOUND, SET } ClassType;
typedef enum { OLD_ST, NEW_ST } ObjState;

class MetaObj // Abstract class
{
  protected:
    ClassType _type;
    long _t1;
    long _t2;
    int  _links;

  public:
    // Derived class casting
    ClassType class_type() const	{ return _type; };
    Set *set() const			{ return (Set *)this; };
    Compound *compound() const		{ return (Compound *)this; };
    Single *single() const		{ return (Single *)this; };

    // Time Window Management
    long t1()				{ return _t1; };
    long t2()				{ return _t2; };
    void comp_window  (MetaObj *other, long &t1, long &t2, int w_this, int w_other);

    // Management of the MetaObj as an Array
    virtual MetaObj *operator[](const int n) = 0;
    virtual MetaObj **meta_dir(MetaObj **dir, const int n, const int n_objs) = 0;

    // Conversion to array
    ObjectType ** meta2array();
    virtual void fill_array(int &n, ObjectType **array) = 0;
    virtual int n_objs() const = 0;

    // Comparison
    virtual int compare(const MetaObj *obj2, va_list list) const = 0;
    virtual int compare_objs(const MetaObj *obj2, int pos_offset, va_list list) const = 0;

    // Object Life control
    void link();
    void unlink();
    int links()				{ return _links; };

    // Struct Management
    virtual MetaObj * duplicate_struct(int link_singles) = 0;
    virtual MetaObj * delete_struct(int unlink_singles, int only_minmmize_rhs) = 0;


    // Interface to trees
    static int metacmp(const void *c_obj1, const void *c_obj2, va_list list);
    static int metacmp_objs(const void *c_obj1, const void *c_obj2, va_list list);
    static void metadelete(void* item, va_list list);
    static int compare_t(const void *c_obj1, const void *c_obj2, va_list list); // Compare w/ time 
    static int compare_tw(const void *c_obj1, const void *c_obj2, va_list list); // Compare w/ time window

    // Control of the objects in modification
    virtual void set_state(ObjState ost, Status *st, int pos) = 0;

    // Printing / Debugging
    virtual void print(FILE *fp, void (*obj_print)(FILE *, const ObjectType *)) const = 0;
    static void print_tree(const void *item, va_list list);

};


#endif
