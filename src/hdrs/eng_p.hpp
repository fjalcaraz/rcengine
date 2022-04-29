/**
 * @file eng_p.hpp
 * @author Francisco Alcaraz
 * @brief Private Functions defined at the eng module
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
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
 
#include "engine.h"

#include "codes.h"
#include "load.hpp" 
#include "metaobj.hpp"
#include "compound.hpp"
#include "set.hpp"
#include "single.hpp"
#include "confset.hpp"
#include "classes.hpp"
#include "nodes.hpp"
#include "status.hpp"
#include "eng.hpp" 
#include "btree.hpp" 
#include "error.hpp" 
#include "callbacks.hpp" 

#define MAX_NVECT 15000   // Maximo numero de objetos en trazas

#define TRACE_FILE_SIZE 5000000

struct vector_key{
  const void *_vector;
  char _key[4];
} ;


typedef void (*RemoveFunc)(void *&);


PRIVATE ObjectType *old_obj_modify  = NULL;  /* Para guardar el obj ante mod externa */

PRIVATE char trace_file_name[256]="stdout";  /* Salida de trazas nombre fichero */
PRIVATE unsigned long trace_file_size = TRACE_FILE_SIZE;  /* trazas tamagno maximo */
PRIVATE int trace_file_size_control = FALSE; /* Control del tamanio del f trazas*/


#define STR(a) ((a==NULL)?"(null)" : a)

int vectors_compare(const void *vec1, const void *vec2, va_list lista);
 
char *get_key_of_nvector(int nvector);

