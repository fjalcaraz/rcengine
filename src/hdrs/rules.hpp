/**
 * @file rules.hpp
 * @author Francisco Alcaraz
 * @brief Function exported by the rules module
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

#include <stdio.h>

#include "engine.h"


#ifndef RULES___HH_INCLUDED
#define RULES___HH_INCLUDED

#define NO_TIMED  -2
#define DEF_TIME  -1

 
PUBLIC void def_package(char *name);
PUBLIC void def_ruleset(char *ruleset_name);
PUBLIC void end_ruleset();

PUBLIC void default_time_window(int time);
PUBLIC void def_rule(char *name, int cat, int tm);
PUBLIC void def_production();
PUBLIC void obj_imply_at(int pos);
PUBLIC void end_rule();
PUBLIC int prod_has_memory(ULong *code);

PUBLIC int curr_rule_tw(); 
PUBLIC int in_the_LHS();
PUBLIC int in_the_RHS();
PUBLIC int inside_rule();

PUBLIC void free_package();
PUBLIC void free_ruleset(char *name);
PUBLIC void free_curr_ruleset();

PUBLIC void reset_package();
PUBLIC void reset_ruleset(char *name);

PUBLIC void set_curr_rule_exec_flags(int flags);
PUBLIC int curr_rule_exec_flags();


#endif
