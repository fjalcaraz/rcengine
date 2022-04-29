/**
 * @file eng.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the eng module.
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
#include "metaobj.hpp"
#include "actions.hpp"
#include "nodes.hpp"
#include "btree.hpp"

#ifndef ENG_____HH_INCLUDED
#define ENG_____HH_INCLUDED

#define RIGHT_MEM 1
#define LEFT_MEM 0

extern PUBLIC int n_inf;              /* number of inferences made */
extern PUBLIC int trace;              /* tracing flag              */
extern PUBLIC FILE *trace_file;       /* traces file               */

PUBLIC void Do_loop(int first_loop);
PUBLIC void do_loop(Action *&list, int first_loop);

// Other auxiliary functions

PUBLIC ObjectType *get_obj_of_set(BTState *tree_state);
PUBLIC void eng_destroy_set(BTState *tree_state);
PUBLIC int n_obj_of_set(BTState *tree_state);
PUBLIC void end_obj_of_set(BTState *tree_state);

PUBLIC void objcpy(ObjectType *dest, ObjectType *ori, int n_attrs);
PUBLIC void check_trace_file_size();

extern "C"
{
    PUBLIC void print_obj(FILE *ftrace, const ObjectType *obj);
    PUBLIC void print_objkey(FILE *ftrace, const ObjectType *obj);
}

#endif
