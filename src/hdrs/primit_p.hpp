/**
 * @file primit_p.hpp
 * @author Francisco Alcaraz
 * @brief Private Functions defined at the primit module
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
 

 
#include <stdlib.h>
#include <string.h>
#include "eng.hpp"

#ifdef SOLARIS
#include <floatingpoint.h>
#endif
 

 
#include "primit.hpp"
 
#ifndef PRIMIT_P_H_INCLUDED
#define PRIMIT_P_H_INCLUDED

 
 

 
 


PRIVATE void append_call(Value *stack, int tag);
PRIVATE void head_call(Value *stack, int tag);
PRIVATE void except_call(Value *stack, int tag);
PRIVATE void tail_call(Value *stack, int tag);
PRIVATE void substr_call(Value *stack, int tag);
PRIVATE void length_call(Value *stack, int tag);
PRIVATE void strtonum_call(Value *stack, int tag);
PRIVATE void numtostr_call(Value *stack, int tag);
PRIVATE void strtofloat_call(Value *stack, int tag);
PRIVATE void floattostr_call(Value *stack, int tag);
PRIVATE void numtofloat_call(Value *stack, int tag);
PRIVATE void floattonum_call(Value *stack, int tag);
PRIVATE void destroy_set(Value *stack, int tag);
PRIVATE void printf_call(Value *stack, int tag);
PRIVATE void sprintf_call(Value *stack, int tag);
PRIVATE void fprintf_call(Value *stack, int tag);

#endif
