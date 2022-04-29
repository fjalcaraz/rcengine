/**
 * @file error.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the error module.
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

#include <stdio.h>
#include <stdio.h>
#include <setjmp.h>

#ifndef ERROR___HH_INCLUDED
#define ERROR___HH_INCLUDED

PUBLIC void set_return_buff(jmp_buf *ret_buff);
PUBLIC void comp_err(const char *info, ...);
PUBLIC void comp_war(const char *info, ...);
PUBLIC void engine_fatal_err(const char *format, ...);

#endif
