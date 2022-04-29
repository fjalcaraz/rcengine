/**
 * @file error_p.hpp
 * @author Francisco Alcaraz
 * @brief Private Functions defined at the error module
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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "lex.hpp"
#include "rules.hpp"
#include "patterns.hpp"
#include "error.hpp"

PRIVATE int show_warnings = 0;
