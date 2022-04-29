/**
 * @file error.cpp
 * @author Francisco Alcaraz
 * @brief Errors and Warnings management
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "error_p.hpp"

PRIVATE jmp_buf *return_buff;
PRIVATE FILE *err_file = stderr;

/**
 * @brief Set the point of execution where to jump to in case of a compiler error
 *      This return point is set by setjmp(), a function that return 1 if a longjmp() has been done (on error)
 * 
 * @param ret_buff The information required by longjump()
 */
PUBLIC void
set_return_buff(jmp_buf *ret_buff)
{
  return_buff = ret_buff;
}

/**
 * @brief Set the errors log file
 * 
 * @param file 
 */
PUBLIC void
set_errors_file(FILE *file)
{
  err_file = file;
}

/**
 * @brief Print the compilation error and makes a longjump() to finish the compilation in a safe manner
 * 
 * @param format Message format in fprintf(trace_file,  style
 * @param ... Variable list of parameters required to print the error and referenced in the format string
 */
PUBLIC void
comp_err(const char *format, ...)
{
  va_list list;
 
  fprintf(err_file, "Compiler Error, Line %d\t: ", get_curr_line());
  va_start(list, format);
  vfprintf(err_file, format, list);
  va_end(list);

  if (inside_rule())      // We end the current rule to be deleted properly
  {
    if (in_the_LHS())     // If in the LHS we delete the created patterns
      Pattern::free_patterns();
    else
      end_rule();         // else we delete the whole rule
  }

  longjmp(*return_buff, 1);
}

/**
 * @brief Generates a compilation warning message and continues
 * 
 * @param format Message format in fprintf(trace_file,  style
 * @param ... Variable list of parameters required to print the message and referenced in the format string
 */
PUBLIC void
comp_war(const char *format, ...)
{
  va_list list;
 
  if (show_warnings)
  {
    fprintf(err_file, "Compiler Warning, Line %d\t: ", get_curr_line());
    va_start(list, format);
    vfprintf(err_file, format, list);
    va_end(list);
  }
}

/**
 * @brief Set the comp warnings flag, so warnings messages may be muted
 * 
 * @param status TRUE/FALSE
 */
PUBLIC void 
set_comp_warnings(int status)
{
   show_warnings = status;
}

/**
 * @brief Print a fatal error during execution time. An exit(1) is done at the end
 * 
 * @param format Message format in fprintf(trace_file,  style
 * @param ... Variable list of parameters required to print the message and referenced in the format string
 */
PUBLIC void
engine_fatal_err(const char *format, ...)
{
  va_list list;
 
  fprintf(err_file, "Engine Fatal Error\t: ");
  va_start(list, format);
  vfprintf(err_file, format, list);
  va_end(list);
  exit(1);
}

