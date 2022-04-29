/**
 * @file expr.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the expr module.
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

#ifndef NO_CAST_TYPE
#define NO_CAST_TYPE -1
#endif

#ifndef PUBLIC
#define PUBLIC
#define PRIVATE static
#endif

#include "lex.hpp"

#ifndef EXPR____HH_INCLUDED
#define EXPR____HH_INCLUDED

typedef enum
{
    ST_NORMAL,
    ST_NEGATED,
    ST_OPTIONAL,
    ST_DELETED
} StValues;

#define effective_type(type) ((type == TYPE_BOOL || type == TYPE_CHAR) ? TYPE_NUM : type)

PUBLIC void match_val(int type, ...);
PUBLIC void match_ref(long ref);
PUBLIC void match_type_ref(int type, long ref);
PUBLIC void equality_ref();
PUBLIC void set_status(StValues status);
PUBLIC void match_class(char *classname);
PUBLIC void match_attr(char *name);
PUBLIC void def_patt_var(char *varname);
PUBLIC int ref_of_var(char *varname, int create_on_missing, int equality = 0);
PUBLIC int ref_of_pvar(char *pvarname, char *attr_name, int equality = 0);
PUBLIC int ref_of_pnum(int pnum, char *attr_name);
PUBLIC int patt_num_of_var(char *varname);
PUBLIC void begin_set();
PUBLIC void end_set();
PUBLIC void end_of_pattern();
PUBLIC Expression *create_val(int type, StackType value);
PUBLIC Expression *create_rel(int op, Expression *lexp, Expression *rexp);
PUBLIC Expression *create_fun(char *name, Argument *arg);
PUBLIC Argument *create_arg(Expression *exp);
PUBLIC Argument *concat_arg(Argument *arg1, Argument *arg2);
PUBLIC Expression *create_primitive(int op, int n_args, ...);
PUBLIC void match_exp(Expression *exp);
PUBLIC void def_set();
PUBLIC void store_exp(Expression *exp);
PUBLIC void init_proc(char *name, ULong tags);
PUBLIC void store_proc(char *name, Argument *args);
PUBLIC void notify_for_create(char *class_name, ULong flags);
PUBLIC void notify_for_modify(int modify_type, int pattern_num, ULong flags);
PUBLIC void notify_for_delete(int pattern_num, ULong flags);
PUBLIC void notify_for_obj_imply(char *class_name);
PUBLIC void end_of_RH_order();

#endif
