/**
 * @file callbacks.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the callbacks module
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef CALLBACKS_HH_INCLUDED
#define CALLBACKS_HH_INCLUDED

#include "engine.h"

PUBLIC void object_created(ObjectType *obj, ObjectType **ctx, int n_objs);
PUBLIC void object_modified(ObjectType *obj, ObjectType **ctx, int n_objs);
PUBLIC void object_deleted(ObjectType *obj);
PUBLIC void object_no_more_used(ObjectType *obj);

#endif
