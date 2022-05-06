/**
 * @file functions.cpp
 * @author Francisco Alcaraz
 * @brief This module controls the primitives and the user defined functions and procedures
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "functions_p.hpp"

PRIVATE int loaded=FALSE;

/**
 * @brief Define the primitives
 * 
 */
PUBLIC void
def_primitives()
{

   if (loaded)
     return;

   loaded = TRUE;

   def_func(strdup("append"));
    def_arg(TYPE_STR);
    def_arg(TYPE_STR);
    def_func_type(TYPE_STR);

   def_func(strdup("head"));
    def_arg(TYPE_STR);
    def_arg(TYPE_NUM);
    def_func_type(TYPE_STR);

   def_func(strdup("tail"));
    def_arg(TYPE_STR);
    def_arg(TYPE_NUM);
    def_func_type(TYPE_STR);

   def_func(strdup("butlast"));
    def_arg(TYPE_STR);
    def_arg(TYPE_NUM);
    def_func_type(TYPE_STR);

   def_func(strdup("substr"));
    def_arg(TYPE_STR);
    def_arg(TYPE_NUM);
    def_arg(TYPE_NUM);
    def_func_type(TYPE_STR);

   def_func(strdup("length"));
    def_arg(TYPE_STR);
    def_func_type(TYPE_NUM);

   def_func(strdup("strtonum"));
    def_arg(TYPE_STR);
    def_func_type(TYPE_NUM);

   def_func(strdup("numtostr"));
    def_arg(TYPE_NUM);
    def_func_type(TYPE_STR);
    
   def_func(strdup("strtofloat"));
    def_arg(TYPE_STR);
    def_func_type(TYPE_FLO);

   def_func(strdup("floattostr"));
    def_arg(TYPE_FLO);
    def_func_type(TYPE_STR);
    
   def_func(strdup("numtofloat"));
    def_arg(TYPE_NUM);
    def_func_type(TYPE_FLO);

   def_func(strdup("floattonum"));
    def_arg(TYPE_FLO);
    def_func_type(TYPE_NUM);

   def_func(strdup("empty_set"));
    def_arg(TYPE_PATTERN);
    def_arg(TYPE_NUM);
    def_func_type(TYPE_VOID);

   def_func(strdup("sprintf"));
    def_arg(TYPE_STR);
    variable_number_of_args();
    def_func_type(TYPE_STR);

   def_func(strdup("printf"));
    def_arg(TYPE_STR);
    variable_number_of_args();
    def_func_type(TYPE_VOID);

   def_func(strdup("fprintf"));
    def_arg(TYPE_NUM);
    def_arg(TYPE_STR);
    variable_number_of_args();
    def_func_type(TYPE_VOID);
}

/**
 * @brief Define a function and set curr_func that will allow to set further configuration
 * 
 * @param name The name of the function
 */
PUBLIC void
def_func(char *name)
{
   FUNC **func_p;
   
   for (func_p=&first_func;
          (*func_p) != (FUNC*)NULL &&
                                  strncmp((*func_p)-> name, name, MAXNAME) !=0;
                  func_p=&((*func_p)->sig_func));

   if ((*func_p) == (FUNC*)NULL)
   {
     (*func_p) = new FUNC;
     curr_func = (*func_p);

     curr_func -> num_of_args = 0;
     curr_func -> var_numb_of_args = FALSE;
     strncpy(curr_func -> name, name, MAXNAME);
     curr_func -> sig_func = (FUNC*)NULL;
   }
   else
   {   
     curr_func = (*func_p);
     comp_err("Function %s already defined\n", name);
   }
   free(name);
}

/**
 * @brief Define de type of the current function (curr_func) 
 * 
 * @param type The type of the function
 */
PUBLIC void
def_func_type(int type)
{
     curr_func -> type = type;
}

/**
 * @brief Define that the current function (curr_func) has a variable number of arguments
 * 
 */
PUBLIC void
variable_number_of_args()
{
    curr_func -> var_numb_of_args = TRUE;
}

/**
 * @brief Define an argument for the current unction
 * 
 * @param type The type of the Argument
 */
PUBLIC void
def_arg(int type)
{
   if (curr_func -> num_of_args < MAXARGS)
   {
     curr_func -> arg_type[ curr_func -> num_of_args ] = type;
     curr_func -> num_of_args++;
   }
   else
   {
     comp_err("Too many arguments for function %s\n", curr_func->name);
   }
}
 
/**
 * @brief returns the type of a existing function identified by its name
 * 
 * @param name Name of the function
 * @return Function type
 */
PUBLIC int func_type(char *name)
{
   FUNC *func;

   for (func=first_func;
          func != (FUNC*)NULL && strncmp(func-> name, name, MAXNAME) !=0;
                  func=func->sig_func);
 
   if (func == (FUNC*)NULL)
   {
      comp_err("Function %s not defined\n", name);
      return(-1);
   }
   else
   {
      return func->type;
   }
}

/**
 * @brief Returns the type of the argument #num of the function identified by its name
 * 
 * @param name Name of the function
 * @param argnum Number of Argument
 * @return the type of the argument
 */
PUBLIC int arg_type(char *name, int argnum)
{
   FUNC *func;
 
   for (func=first_func;
          func != (FUNC*)NULL && strncmp(func-> name, name, MAXNAME) !=0;
                  func=func->sig_func);

   if (func == (FUNC*)NULL)
   {
      comp_err("Function %s not defined\n", name);
      return(-1);
   }

   if (func-> num_of_args <= argnum)
   {
      comp_err("Argument %d for function %s not declared\n", argnum, name);
      return(-1);
   }

   return( func->arg_type[argnum]);
}

/**
 * @brief Return the number of fixed arguments of a function identified by its name
 * 
 * @param name Name of the function
 * @return number of arguments
 */
PUBLIC int num_of_args(char *name)
{
   FUNC *func;

   for (func=first_func;
          func != (FUNC*)NULL && strncmp(func-> name, name, MAXNAME) !=0;
                  func=func->sig_func);

   if (func == (FUNC*)NULL)
   {
      comp_err("Function %s not defined\n", name);
      return(-1);
   }
   else
   {
      return(func-> num_of_args);
   }
}
 
/**
 * @brief Returns if the function has a variable number of arguments
 * 
 * @param name Name of teh function
 * @return int TRUE/FALSE
 */
PUBLIC int func_has_var_num_of_args(char *name)
{
   FUNC *func;
 
   for (func=first_func;
          func != (FUNC*)NULL && strncmp(func-> name, name, MAXNAME) !=0;
                  func=func->sig_func);
 
   if (func == (FUNC*)NULL)
   {
      comp_err("Function %s not defined\n", name);
      return(0);
   }
   else
   {
      return func->var_numb_of_args ;
   }
}

/**
 * @brief Clean up all the function definitions
 * 
 */
PUBLIC void
undef_all_func()
{
   while ( first_func != (FUNC*)NULL )
   {
     FUNC *next_func = first_func->sig_func;
     delete first_func;
     first_func = next_func;
   }

   loaded = FALSE;
}