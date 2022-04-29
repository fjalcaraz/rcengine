/**
 * @file functions_p.hpp
 * @author Francisco Alcaraz
 * @brief Private Functions defined at the functions module
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

#include <string.h>
#include <stdlib.h>

#include "codes.h"
#include "config.hpp"
#include "error.hpp"
#include "functions.hpp"


#ifndef FUNCTIONS_P_H_INCLUDED
#define FUNCTIONS_P_H_INCLUDED


typedef struct func_def_str
{
        char name[MAXNAME];
        int num_of_args;
        int type;
        int arg_type [MAXARGS];
        int var_numb_of_args;
        struct func_def_str *sig_func;
} FUNC;

PRIVATE FUNC *first_func = (FUNC*)NULL;
PRIVATE FUNC *curr_func  = (FUNC*)NULL;


#endif
