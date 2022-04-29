/**
 * @file set.hpp
 * @author Francisco Alcaraz
 * @brief Set class definition
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

class Set;


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
 
#include "engine.h"
#include "error.hpp"

#include "metaobj.hpp"
#include "single.hpp"
#include "btree.hpp"

 
#ifndef SET_____HH_INCLUDED
#define SET_____HH_INCLUDED


class Set : public MetaObj
{
  private :
    BTree _mem;
    int _last_op;
    int _state;
    MetaObj *_last_obj;

  public :
    inline Set() : _mem() {
        _type = SET;
        _t1 = _t2 = 0;
        _links = 1;
        _last_op = 0;
        _state = 0;
        _last_obj = NULL;
    };

    // Methods for the insertion and removing of items in the set

    inline void flush_set() { _mem.Free( MetaObj::metadelete); };
    int is_null() 				{ return _mem.numItems() == 0; };
    int will_be_null() 			{ return _mem.numItems() <= 1; };
    MetaObj * add_elem(MetaObj *elem);
    MetaObj * delete_elem(MetaObj *elem);
    MetaObj * find_elem(MetaObj *elem);

    inline void setOp(int tag, MetaObj *obj)
    {
        _last_obj = obj;
        _last_op  = tag;

        // initially the operation is not done and due to this the _state
        // is the opposite to the tag of teh operation, the desired final _state
        _state = _last_op ^ 0x3; // Opposite op = Inversing the bits (MODIFY will set to 0, nothing is done
    };

    // Operations over the elements
    long sum_set_n(int n_attr);
    float sum_set_f(int n_attr);
    long prod_set_n(int n_attr);
    float prod_set_f(int n_attr);
    long min_set_n(int n_attr);
    float min_set_f(int n_attr);
    char *min_set_a(int n_attr);
    long max_set_n(int n_attr);
    float max_set_f(int n_attr);
    char *max_set_a(int n_attr);
    char *concat(int n_attr, char *sep);
    MetaObj *first_item_of_set() const;

    // Implemantation of inherited methods

    int compare(const MetaObj *obj2, va_list list) const;
    int compare_objs(const MetaObj *obj2, int pos_offset, va_list list) const;
    MetaObj * operator[](const int n);
    MetaObj **meta_dir(MetaObj **dir, const int n, const int n_objs);
    void fill_array(int &n, ObjectType *array[]);
    int n_objs() const;
    MetaObj * duplicate_struct(int link_singles);
    MetaObj * delete_struct(int unlink_singles, int only_minmmize_rhs);
    void set_state(ObjState ost, Status *st, int pos);

    void print(FILE *fp, void (*obj_print)(FILE *, const ObjectType *)) const;
    BTree *get_tree();
};
    

#endif
