/**
 * @file dasm_rete.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the dasm_rete module.
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef ERROR
#define ERROR -1
#endif

#ifndef PUBLIC
#define PUBLIC
#define PRIVATE static
#endif

#include "engine.h"

#ifndef DASM_RE_HH_INCLUDED
#define DASM_RE_HH_INCLUDED

PUBLIC ULong dasm_code(ULong code);
PUBLIC void dasm_rete(const char *prefix, ULong *code_array, int codelen);

#endif
