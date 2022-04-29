/**
 * @file load.hpp
 * @author Francisco Alcaraz
 * @brief Function exported by the load module
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
#include <nodes.hpp>

#ifndef LOAD____HH_INCLUDED
#define LOAD____HH_INCLUDED


extern int trace;
extern FILE *trace_file;

// (Included in engine.h)
PUBLIC void load_code(ULong *pcode, int len_code, Node *node);
PUBLIC void reset_code(ULong *pcode, int len_code, int free_code);

#endif

