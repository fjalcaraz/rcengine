/**
 * @file actions.hpp
 * @author Francisco ALcaraz
 * @brief Action class header file
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "engine.h"
#include "single.hpp"
#include "nodes.hpp"

#ifndef ACTIONS_HH_INCLUDED
#define ACTIONS_HH_INCLUDED

#define CHANGE_TAG   255    // Used in MODIFY NO PROPAGATE actions

struct Action
{
    Single *_single;
    int _tag;
    int _pos;
    int _side;
    ObjectType *_old_obj;
    int _n_of_attrs;
    char _mod_attr[MAXATTRS];
    char _mod_str_attr[MAXATTRS];
    Node *_node;
    ULong *_codep;
    int _from_the_root;
    ObjectType **_context;
    int _n_objs_in_ctx;
 
    Action *_next;
 
    static Action *_action_list;
    static Action *_last;
    static Action **_action_tail;

    Action(int tag, Single *obj, MetaObj *CObj_ctx, ObjectType *old_obj, int is_external = FALSE,
                   Node *node = NULL, ULong *codep =NULL, 
                   int side = LEFT_MEM, int pos=0);
    ~Action();   
    
    void push();
    static Action *pop(Action *&mi_inic);
    void objswap();
    void free_mod_str();
    void store_mod_attr(int attr) { _mod_attr[attr] = 1; _mod_str_attr[attr]=1; };
    void fill_n_attrs();

    static Action *&main_list() { return _action_list; };
    static Action **tail() { return _action_tail; };
    static Action *last() { return _last; };
 
};
 
#endif

