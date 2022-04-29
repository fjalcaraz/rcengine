/**
 * @file callbacks.cpp
 * @author Francisco Alcaraz
 * @brief this file defines the functions that the external code must call to define callback 
 *       functions that will be called with every object event 
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "engine.h"
#include "callbacks.hpp"
// List of callbacks
struct CallBackList
{
   CallBackFunc f;
   int when_flags;
   struct CallBackList *next;
};
   
PRIVATE CallBackList *list = NULL;

/**
 * @brief Define a callback function
 * 
 * @param when_flags: the flags when the callback will be called: WHEN_INSERTED, WHEN_MODIFIED, WHEN_RETRACTED, WHEN NOT USED
 * @param f the callback function 
 */
PUBLIC 
void add_callback_func(int when_flags, CallBackFunc f)
{
   CallBackList **p;

   for (p=&list ; (*p)!= NULL && (*p)->f != f; p = &((*p)->next));

   if ((*p) == NULL)
   {
      (*p) = new CallBackList;
      (*p)->f = f;
      (*p)->when_flags = when_flags;
      (*p)->next = NULL;
   }
   else (*p)->when_flags |= when_flags;
}

/**
 * @brief Delete a callback function to be called on certain event types. The callback function is removed only if 
 *     there is no more events to attend (when_flags == 0)
 * 
 * @param when_flags: the flags removed from calling: WHEN_INSERTED, WHEN_MODIFIED, WHEN_RETRACTED, WHEN NOT USED
 * @param f the callback function 
 */
PUBLIC 
void del_callback_func(int when_flags, CallBackFunc f)
{
   CallBackList **p;
 
   for (p=&list ; (*p)!= NULL && (*p)->f != f; p = &((*p)->next));
 
   if ((*p) != NULL)
   {
     (*p)->when_flags &= ~when_flags;
     if ((*p)->when_flags == 0)
     {
        CallBackList *the_callback = (*p);
        (*p) = (*p)->next;
        delete the_callback;
     }
   }
}

/**
 * @brief The function called internally when an object is created
 * 
 * @param obj Object
 * @param ctx Context (Left Side of Rule matching)
 * @param n_objs Number of objects in the context
 */
PUBLIC
void object_created(ObjectType *obj, ObjectType **ctx, int n_objs)
{
   CallBackList *p;

   for (p=list ; p!= NULL; p = p->next)
   {
     if ((p->when_flags & WHEN_INSERTED) != 0)
       (* p->f)(WHEN_INSERTED, obj, ctx, n_objs);
   }
}

/**
 * @brief The function called internally when an object is modified
 * 
 * @param obj Object
 * @param ctx Context (Left Side of Rule matching)
 * @param n_objs Number of objects in the context
 */
PUBLIC
void object_modified(ObjectType *obj, ObjectType **ctx, int n_objs)
{
   CallBackList *p;

   for (p=list ; p!= NULL; p = p->next)
   {
     if ((p->when_flags & WHEN_MODIFIED) != 0)
       (* p->f)(WHEN_MODIFIED, obj, ctx, n_objs);
   }
}

/**
 * @brief The function called internally when an object is deleted
 * 
 * @param obj Object
 */
PUBLIC
void object_deleted(ObjectType *obj)
{
   CallBackList *p;
 
   for (p=list ; p!= NULL; p = p->next)
   {
     if ((p->when_flags & WHEN_RETRACTED) != 0)
       (* p->f)(WHEN_RETRACTED, obj, NULL, 0);
   }
}

/**
 * @brief The function called internally when an object is no more used
 * 
 * @param obj Object
 */
PUBLIC
void object_no_more_used(ObjectType *obj)
{
   CallBackList *p;
 
   for (p=list ; p!= NULL; p = p->next)
   {
     if ((p->when_flags & WHEN_NOT_USED) != 0)
       (* p->f)(WHEN_NOT_USED, obj, NULL, 0);
   }
}
 
