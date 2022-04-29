/**
 * @file lex.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the lex module
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
 
#ifndef ANALEX__HH_INCLUDED
#define ANALEX__HH_INCLUDED
   
typedef union stacktype
{
  char   *ident;
  char   *str;
  long    num;
  float   flo;
  struct  arg_str *arg;
  struct  exp_str *exp;
} StackType;

typedef struct exp_str
{
  int op;
  int is_rel;
  union {
     struct {
       int type;
       StackType val;
     } val_exp;

     struct {
       struct exp_str *lexp, *rexp;
     } rel_exp;

     struct {
       char *name;
       struct arg_str *args;
     } fun_exp;
  } data;
} Expression;


typedef struct arg_str
{
  Expression *arg_exp;
  struct arg_str *sig;
} Argument;
  
#define YYSTYPE StackType

// MEANING AND HOW TO USE THESE STRUCTURES
//
// This structures are used by the syntactic analyzer. The structure used in
// the stack of the parser is StackType as can be seeing defining YYSTYPE as StackType.
//
// The struct Expression is used to load in memory a complex arithmetic expression to
// compile. The fields of this struct has the following dependencies:
//
//  - If op is PUSH, that means that is a simple value that will go in data.val_exp 
//    where go its type and its value
//
//  - If is_rel == TRUE, op will have the symbol of a relation of two arguments
//    the relation may be a comparison operator (<,>,<>, etc), logical operators
//    (AND/OR/NOT) as well as arithmetic operators +.-,*,/,-unary
//
//  - If is_rel == FALSE when the op is the call to a external function (FCALL)
//    with the name of teh function in data.fun_exp.name, or a primitive call
//    (with its particular op code). The arguments will go in data.fun_exp.args

PUBLIC int read_pkg(char *text);
PUBLIC int read_rset(char *text);
PUBLIC int get_curr_line(void);

int eng_lex();
void eng_error(const char *s);

#endif
