/**
 * @file compound.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the compound module. Definition of the Compound class
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

class Compound;

//* DEFINICIONES GENERALES:

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

#include <stdio.h>

#include "engine.h"
#include "metaobj.hpp"

#ifndef COMPOUN_HH_INCLUDED
#define COMPOUN_HH_INCLUDED

class Compound : public MetaObj
{
private:
  int _n_left;
  int _n_right;
  MetaObj *_left;
  MetaObj *_right;

public:
  // Creation, composition and expand
  Compound(MetaObj *left_comp, MetaObj *right_comp, long t1 = 0L, long t2 = 0L);
  Compound(MetaObj *left_comp);
  Compound(Compound &otro);

  inline void expand(MetaObj **left, MetaObj **right);
  MetaObj *&left() { return _left; };
  void set_left(MetaObj *left_comp) { _left = left_comp; };
  void set_right(MetaObj *right_comp) { _right = right_comp; };
  MetaObj *&right() { return _right; };
  MetaObj *join_by_right_untimed(MetaObj *right_comp);

  // Compare and implementation of virtual functions
  int compare(const MetaObj *obj2, va_list list) const;
  int compare_objs(const MetaObj *obj2, int pos_offset, va_list list) const;

  static int compare_with_left(const MetaObj *obj1,
                               const MetaObj *obj2, va_list list);
  static int compare_left_with_left(const MetaObj *obj1,
                                    const MetaObj *obj2, va_list list);

  MetaObj *operator[](const int n);
  MetaObj **meta_dir(MetaObj **dir, const int n, const int n_objs);
  int n_objs() const;
  void fill_array(int &n, ObjectType *array[]);

  MetaObj *duplicate_struct(int link_singles);
  MetaObj *delete_struct(int unlink_singles, int only_minmmize_rhs);

  void set_state(ObjState ost, Status *st, int pos);
  void print(FILE *fp, void (*obj_print)(FILE *, const ObjectType *)) const;
};

inline void
Compound::expand(MetaObj **left, MetaObj **right)
{
  if (left != NULL)
    (*left) = this->_left;
  if (right != NULL)
    (*right) = this->_right;
}

#endif
