/**
 * @file load_p.hpp
 * @author Francisco Alcaraz
 * @brief Private Functions defined at the load module
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
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
 
#include "engine.h"

#include <btree.hpp>
#include "codes.h"
#include "load.hpp"
#include "primit.hpp"
#include "eng.hpp"
#include "metaobj.hpp"
#include "compound.hpp"
#include "single.hpp"
#include "set.hpp"
#include "dasm_rete.hpp"
#include "rules.hpp"
#include "classes.hpp"
#include "lex.hpp"
#include "error.hpp"
#include "functions.hpp"










PRIVATE void del_and_node_mem_item(void* item, va_list);
PRIVATE void del_asym_node_lmem_item(void* count, va_list);
PRIVATE void del_set_node_mem_item(void* item, va_list list);
PRIVATE void del_prod_node_mem_item(void* item, va_list list);

