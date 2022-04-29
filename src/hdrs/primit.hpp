/**
 * @file primit.hpp
 * @author Francisco Alcaraz
 * @brief Function exported by the primit module
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

#ifndef PRIMIT__HH_INCLUDED
#define PRIMIT__HH_INCLUDED


PUBLIC ExternFunction get_func_pointer(char *name);
PUBLIC void load_primitives();
PUBLIC void undef_all_function();

#endif
