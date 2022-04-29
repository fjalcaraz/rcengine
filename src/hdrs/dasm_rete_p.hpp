/**
 * @file dasm_rete_p.hpp
 * @author Francisco Alcaraz
 * @brief Private Functions defined at the dasm_rete module
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

#include <stdio.h>
#include <string.h>

#include "codes.h"
#include "eng.hpp"
#include "dasm_rete.hpp"


#ifndef DASM_RETE_P_H_INCLUDED
#define DASM_RETE_P_H_INCLUDED


PRIVATE char *objstr(unsigned long data);
PRIVATE void print_type(ULong type);
PRIVATE void print_flags(ULong double_flags);

#endif
