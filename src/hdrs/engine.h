/**
 * @file engine.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the engine library
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef _ENGINE_H
#define _ENGINE_H

#ifndef PUBLIC
#define PUBLIC
#endif

#ifndef PRIVATE
#define PRIVATE static
#endif

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

#include <stdio.h>
#include <stdarg.h>

/* Constants to control the protection over string values */
#define PROTECTED 2
#define DYNAMIC 1

/* Boolean contants */

#define TRUE_VALUE 1
#define FALSE_VALUE 0

#define NULL_STR_VALUE 0

/* Constants for engine_loop */

#define INSERT_TAG 0x1
#define RETRACT_TAG 0x2
#define MODIFY_TAG 0x3

#define TYPE_STR 0
#define TYPE_NUM 1
#define TYPE_FLO 2
#define TYPE_SAME 3

/* Special types used during compilation */

#define TYPE_CHAR 4
#define TYPE_BOOL 5
#define TYPE_REF 6
#define TYPE_PATTERN 7
#define ANY_TYPE 255
#define TYPE_VOID -1

#define MAXATTRS 256

/* EXECUTION TAGS */
#define EXEC_INSERT 0x1
#define EXEC_MODIFY 0x2
#define EXEC_RETRACT 0x4
#define EXEC_ALL (EXEC_INSERT | EXEC_MODIFY | EXEC_RETRACT)
#define EXEC_TRIGGER 0x8

/* TAGS FOR THE CALLBACKS */
#define WHEN_INSERTED 0x1
#define WHEN_MODIFIED 0x2
#define WHEN_RETRACTED 0x4
#define WHEN_NOT_USED 0x8

#define WHEN_EVENTS (WHEN_INSERTED | WHEN_MODIFIED | WHEN_RETRACTED)
#define WHEN_ALL (WHEN_INSERTED | WHEN_MODIFIED | WHEN_RETRACTED | WHEN_NOT_USED)

typedef unsigned long ULong;
typedef unsigned int UInt;
typedef unsigned char UChar;

typedef enum
{
        AND_NODE_MEM,
        ASYM_NODE_LMEM,
        SET_NODE_MEM,
        PROD_NODE_MEM
} NodeMemType;

typedef union
{
        struct
        {
                char *str_p;
                int dynamic_flags;
        } str;
        long num;
        float flo;
} Value;

typedef struct obj_data
{
        long time;
        void *user_data;
        Value attr[1];
} ObjectType;

typedef void (*ExternFunction)(Value *, int /*tag*/);

/* Comunication to the user of changing in objects. The contexts can be free whenever */
typedef void (*CallBackFunc)(int when_flag, ObjectType *obj, ObjectType **ctx, int n_objs);

#ifdef __cplusplus
extern "C"
{
#endif

        /* To control the number of inferences */
        PUBLIC void reset_inf_cnt();
        PUBLIC int get_inf_cnt();

         /* To control the traces of the system */
        PUBLIC void set_trace_level(int level);
        PUBLIC void set_trace_file(FILE *file);              /* by default stdout */
        PUBLIC void set_trace_file_name(const char *name); 
        PUBLIC void set_trace_file_size(unsigned long size); /* by default 5MB    */

        PUBLIC void set_lex_debug(int debug);

        /* To manage sets in the user functions */
        PUBLIC void end_obj_of_set(void *tree_state);
        PUBLIC ObjectType *get_obj_of_set(void *tree_state);
        PUBLIC int n_obj_of_set(void *tree_state);

        /* Do the temporal refresh of the engine */
        PUBLIC void engine_refresh(long real_time);

        /* To define callback functions to communicate events (event listeners) */
        /* The context arrays may be free whenever */
        PUBLIC void add_callback_func(int when_flags, CallBackFunc f);
        PUBLIC void del_callback_func(int when_flags, CallBackFunc f);

        /* To define external functions */
        PUBLIC void def_function(const char *name, ExternFunction f);

        /* To control the output of warnings */
        PUBLIC void set_comp_warnings(int status);

        /* To defien a FILE where to send the compilation errors (by default stderr) */
        PUBLIC void set_errors_file(FILE *file);

        /* To the load and unload of rule packages */
        PUBLIC int load_pkg(char *path);
        PUBLIC int load_pkg_str(char *text);
        PUBLIC void free_pkg();
        PUBLIC void reset_pkg();
        PUBLIC int load_rset(char *path);
        PUBLIC int load_rset_str(char *text);
        PUBLIC void free_rset(char *name);
        PUBLIC void reset_rset(char *name);

        /* Management of objects (creation and propagation) */
        PUBLIC ObjectType *new_object(int n_attrs, long time);
        PUBLIC void engine_modify(ObjectType *obj);
        PUBLIC void engine_loop(int tag, ObjectType *obj);

        /* Management of object classes, inheritance and attributes */
        PUBLIC void *get_class(char *name, int *n_attr);
        PUBLIC int class_is_subclass_of(char *name1, char *name2);
        PUBLIC char *attr_name(void *objclass, int n_attr);
        PUBLIC int attr_type(void *objclass, int n_attr);
        PUBLIC int attr_index(void *objclass, char *name);

        /* To allow the matching in different patterns of the same object. (by default is false) */
        PUBLIC void allow_same_obj();

        /* Debug utility to generate a small different identifier for every different value passed as parameter */ 
        PUBLIC const char *clave(const void *vector);

        /* Debug utility to print Objects */
        PUBLIC void print_obj(FILE *file, const ObjectType *obj);

#ifdef __cplusplus
}
#endif

#endif
