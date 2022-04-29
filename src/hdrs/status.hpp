/**
 * @file status.hpp
 * @author Francisco Alcaraz
 * @brief Definitions of Status struct
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */


struct Status;

#ifndef STATUS__HH_INCLUDED
#define STATUS__HH_INCLUDED

#include "engine.h"
#include "single.hpp"
#include "actions.hpp"

struct Status
{
   Single *_single;
   Single *_old_single;
   ObjectType *_obj;
   ObjectType *_old_obj;

   Status(Action *act)
   { 
       _single=act->_single;
       _old_single = act->_single;
       _obj=act->_single->obj();
       _old_obj=act->_old_obj;
   };

   Status()
   {
       _single = NULL;
       _old_single = NULL;
       _obj = NULL;
       _old_obj = NULL;
   };
};

#endif
