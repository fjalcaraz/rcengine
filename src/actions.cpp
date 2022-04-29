/**
 * @file actions.cpp
 * @author Francisco Alcaraz
 * @brief This file manages the Actions that are propagated through the nodes network
 *       An Action is a pair of an object (Single) with an basic action over it (Creation, Deletion or Modification)
 *       Due to an action may produce other actions (over other objects) as the effect of the action propagation, 
 *       a list of subsequent actions is maintained due all these acction must be propagated until no one will be pending
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "engine.h"
#include "single.hpp"
#include "classes.hpp"
#include "actions.hpp"

Action *Action::_action_list = NULL;
Action *Action::_last = NULL;
Action **Action::_action_tail= &_action_list;

/**
 * @brief Action creation
 * 
 * @param tag: The accion type or event to be applied to the object
 * @param single: The Single that carry out the object the action is applied to
 * @param CObj_ctx: the metaobject that is all the objects that triggered the rule that has this action in her right side, it is the execution context
 * @param old_obj: Old version of the object in case of modification
 * @param is_external: If the modification is external to the rule engine
 * @param node: Node from where the Action is propagated. Null means the root of the net
 * @param codep: The code that change the object in case of modification or creation
 * @param side: the side left/right of the object (in case of modification or deletion) in the context
 * @param pos: the position into the side of the object (in case of modification or deletion) in the context
 * 
 * @return an Action
 */
Action::Action(int tag, Single *single, MetaObj *CObj_ctx, ObjectType *old_obj, 
               int is_external /*=FALSE*/,
               Node *node /*=NULL*/, ULong *codep /*=NULL*/, 
               int side /*=LEFT_MEM*/, int pos /*=0*/)
{

   _tag 		= tag;
   _single 	= single;
   _old_obj = old_obj;
   _side 	= side;
   _pos     = pos;
   _codep 	= codep;

   _node = ((node == NULL)? ObjClass::get_real_root(): node);
   _from_the_root = (_node == ObjClass::get_real_root() && codep == NULL);

   if (CObj_ctx != NULL)
   {
      _context 		= CObj_ctx->meta2array();
      _n_objs_in_ctx = CObj_ctx->n_objs();
   }
   else
   {
      _context 		= NULL;
      _n_objs_in_ctx = 0;
   }

   _n_of_attrs = -1;
   memset(_mod_attr, is_external, MAXATTRS);
   memset(_mod_str_attr, 0, MAXATTRS);

   _next = NULL;
}

/**
 * @brief Destroy the Action:: Action object
 * 
 */
Action::~Action()
{
   if (_tag == MODIFY_TAG)
       free_mod_str();

   if (_old_obj != NULL && _old_obj != _single->obj())
        free( _old_obj );

   _single->unlink();

}

/**
 * @brief Fills the number of attributes deppending of the class of teh object 
 * that is stored as attribute 0
 * 
 */
void
Action::fill_n_attrs()
{
   ObjClass *the_class;

   the_class = *ObjClass::get_class(_single->obj()->attr[0].str.str_p);

   if (the_class != NULL)
     _n_of_attrs = the_class->n_attrs();
}

/**
 * @brief pop an action form the action list after its execution
 * 
 * @param initial curret executed action
 * @return Action* 
 */
Action *Action::pop(Action *&initial)
{
   Action *act = initial;
 
   while (act != NULL && act->_tag == CHANGE_TAG)
   {
      initial = initial->_next;
      delete act;
      act = initial;
   }
  
   if (act == NULL){
     _action_tail = &initial;
     return NULL;
   }
 
   initial = initial->_next;
 
   if (initial == NULL)
     _action_tail = &initial;

   return act;
}

/**
 * @brief push the action at the end of the action queue. The action is appended to the end of the tail (last in, last executed) 
 * 
 */
void Action::push()
{
   *_action_tail = this;
   _action_tail = &(_next);
   _last = this;
}

/**
 * @brief free the old value of a modified string (as value of attributes) 
 * once the action has been propagated and the old value is no longer needed 
 * 
 */
void Action::free_mod_str()
{
    int i;
 
    if (_n_of_attrs == -1 ) fill_n_attrs();

    for (i=0; i < _n_of_attrs; i++)
    {
        if (_mod_str_attr[i])
        {
            if (_old_obj->attr[i].str.dynamic_flags == DYNAMIC)
                free(_old_obj->attr[i].str.str_p);
        }
    }
 
}

/**
 * @brief convert a modified object in its previous version or in the new one
 * 
 */
void Action::objswap()
{
   int n;
   Value tmp_attr;
   void *tmp_user_data;
   ObjectType *obj = _single->obj();
 
   if (_n_of_attrs == -1 ) fill_n_attrs();

   for (n = 0; n < _n_of_attrs; n++)    // Here n_attrs includes attr[0]
   {          
       if (_mod_attr[n])
       {
         tmp_attr = _old_obj->attr[n];
         _old_obj->attr[n] = obj->attr[n];
         obj->attr[n] = tmp_attr;
       }
   }
 
   tmp_user_data = _old_obj->user_data;
   _old_obj->user_data = obj->user_data;
   obj->user_data = tmp_user_data;
}


