/**
 * @file functions.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the functions module.
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

 
#include "engine.h"
 
 
#ifndef FUNCTIO_HH_INCLUDED
#define FUNCTIO_HH_INCLUDED

PUBLIC void def_primitives(void);
PUBLIC void def_func(char *name);
PUBLIC void def_func_type(int type);
PUBLIC void def_arg(int type);
PUBLIC int func_type(char *name);
PUBLIC int arg_type(char *name, int argnum);
PUBLIC int num_of_args(char *name);
PUBLIC void undef_all_func();
PUBLIC void variable_number_of_args();
PUBLIC int func_has_var_num_of_args(char *name);

#endif
