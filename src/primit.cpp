/**
 * @file primit.cpp
 * @author Francisco Alcaraz
 * @brief Implementation of the primitives of the language
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <unistd.h>
#include "config.hpp"
#include "primit_p.hpp"

#define ZEROPAD 1  /* pad with zero */
#define SIGN 2     /* unsigned/signed long */
#define PLUS 4     /* show plus */
#define SPACE 8    /* space if plus */
#define LEFT 16    /* left justified */
#define SPECIAL 32 /* 0x */
#define SMALL 64   /* use 'abcdef' instead of 'ABCDEF' */

/* we use this so that we can do without the ctype library */
#define is_digit(c) ((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s);
int do_div(int *num, int base);
static char *format_number(char *str, int num, int base, int size, int precision, int type);
static char *format_float(char *str, double num, int size, int precision, int float_class, int type);

// printf types
#define PRINTF 0
#define SPRINTF 1
#define FPRINTF 2
static void gprintf_call(Value *stack, int prtype);

// List of defined functions
struct FuncList
{
  char name[MAXNAME];
  ExternFunction f;
  struct FuncList *next;
};

PRIVATE FuncList *list = NULL;
PRIVATE int primit_loaded = FALSE;

/**
 * @brief Define a new Function
 * 
 * @param name Name of the function
 * @param f Function implementation
 */
PUBLIC
void def_function(const char *name, ExternFunction f)
{
  FuncList *p;

  p = new FuncList;
  strncpy(p->name, name, MAXNAME);
  p->f = f;
  p->next = list;
  list = p;
}

/**
 * @brief Undefine all the functions
 * 
 */
PUBLIC
void undef_all_function()
{
  FuncList *p;

  while (list != NULL)
  {
    p = list->next;
    delete list;
    list = p;
  }
  primit_loaded = FALSE;
}

/**
 * @brief Get an function by name
 * 
 * @param name 
 * @return The function implementation
 */
PUBLIC
ExternFunction get_func_pointer(char *name)
{
  FuncList *p;

  for (p = list; p != NULL && strncmp(p->name, name, MAXNAME) != 0; p = p->next)
    ;

  if (p == NULL)
    return NULL;
  else
    return p->f;
}

/**
 * @brief Define all the primitives as functions
 * 
 * @return PUBLIC 
 */
PUBLIC
void load_primitives()
{
  if (primit_loaded)
    return;

  def_function("append", append_call);
  def_function("head", head_call);
  def_function("except", except_call);
  def_function("tail", tail_call);
  def_function("substr", substr_call);
  def_function("length", length_call);
  def_function("numtostr", numtostr_call);
  def_function("strtonum", strtonum_call);
  def_function("floattostr", floattostr_call);
  def_function("strtofloat", strtofloat_call);
  def_function("floattonum", floattonum_call);
  def_function("numtofloat", numtofloat_call);
  def_function("sprintf", sprintf_call);
  def_function("printf", printf_call);
  def_function("fprintf", fprintf_call);
  def_function("empty_set", empty_set);

  primit_loaded = TRUE;
}

/**
 * @brief Do the concatenation of two strings in the stack al leave the result in it 
 * 
 * @param stack Data stack
 */
PRIVATE
void append_call(Value *stack, int /*tag*/)
{
  char *str;
  int len;

  if (stack[0].str.str_p == NULL)
  {
    stack[0].str.str_p = stack[1].str.str_p;
    stack[0].str.dynamic_flags = stack[1].str.dynamic_flags;
  }
  else if (stack[1].str.str_p != NULL)
  {

    len = strlen(stack[0].str.str_p) + strlen(stack[1].str.str_p);

    str = (char *)malloc(len + 1);

    sprintf(str, "%s%s", stack[0].str.str_p, stack[1].str.str_p);

    if (stack[0].str.dynamic_flags == DYNAMIC)
      free(stack[0].str.str_p);
    if (stack[1].str.dynamic_flags == DYNAMIC)
      free(stack[1].str.str_p);

    stack[0].str.dynamic_flags = DYNAMIC;
    stack[0].str.str_p = str;
  }
}

/**
 * @brief Get the first characters of a string and leave the result there
 * 
 * @param stack Data stack with the string at [0] and the length at [1]
 */
PRIVATE
void head_call(Value *stack, int /*tag*/)
{
  char *str;
  int len;

  if (stack[0].str.str_p == NULL)
    return;

  len = strlen(stack[0].str.str_p);

  if (len < stack[1].num)
  {
    stack[1].num = len;
  }

  str = (char *)malloc((unsigned int)(stack[1].num) + 1);
  strncpy(str, stack[0].str.str_p, stack[1].num);
  str[stack[1].num] = '\0';

  if (stack[0].str.dynamic_flags == DYNAMIC)
    free(stack[0].str.str_p);

  stack[0].str.dynamic_flags = DYNAMIC;
  stack[0].str.str_p = str;
}

/**
 * @brief Returns a string after removing the last n characters of it
 * 
 * @param stack Data stack with the string at [0] and the length at [1]
 */
PRIVATE
void except_call(Value *stack, int /*tag*/)
{
  char *str;
  unsigned int len;

  if (stack[0].str.str_p == NULL)
    return;

  len = strlen(stack[0].str.str_p);

  if (len < stack[1].num)
  {
    stack[1].num = len;
  }

  str = (char *)malloc(len - (unsigned int)(stack[1].num) + 1);
  strcpy(str, stack[0].str.str_p + stack[1].num);

  if (stack[0].str.dynamic_flags == DYNAMIC)
    free(stack[0].str.str_p);

  stack[0].str.dynamic_flags = DYNAMIC;
  stack[0].str.str_p = str;
}

/**
 * @brief Returns the last n characters of a string
 * 
 * @param stack Data stack with the string at [0] and the length at [1]
 */
PRIVATE
void tail_call(Value *stack, int /*tag*/)
{
  char *str;
  unsigned int len;

  if (stack[0].str.str_p == NULL)
    return;

  len = strlen(stack[0].str.str_p);

  if (len < stack[1].num)
  {
    stack[1].num = len;
  }

  str = (char *)malloc((unsigned int)(stack[1].num) + 1);
  strcpy(str, stack[0].str.str_p + len - stack[1].num);

  if (stack[0].str.dynamic_flags == DYNAMIC)
    free(stack[0].str.str_p);

  stack[0].str.dynamic_flags = DYNAMIC;
  stack[0].str.str_p = str;
}

/**
 * @brief 
 * 
 * @param stack Data stack with the string at [0] and the starting offset at [1] and ending offset at [2]
 *      The indexes are based 1 (number of character). substr("abcd", 3, 3) => "c"
 */
PRIVATE
void substr_call(Value *stack, int /*tag*/)
{
  char *str;
  int len;

  if (stack[0].str.str_p == NULL)
    return;

  len = strlen(stack[0].str.str_p);

  stack[1].num--; // To be the index

  if (len < stack[1].num)
    stack[1].num = len;

  if (len < stack[2].num)
    stack[2].num = len;

  if (stack[1].num < 0)
    stack[1].num = 0;

  if (stack[2].num < 0)
    stack[1].num = 0;

  if (stack[1].num > stack[2].num)
    stack[1].num = stack[2].num;

  str = (char *)malloc((unsigned int)(stack[2].num) - (unsigned int)(stack[1].num) + 1);
  sprintf(str, "%.*s",
          (int)(stack[2].num - stack[1].num),
          stack[0].str.str_p + stack[1].num);

  if (stack[0].str.dynamic_flags == DYNAMIC)
    free(stack[0].str.str_p);

  stack[0].str.dynamic_flags = DYNAMIC;
  stack[0].str.str_p = str;
}

/**
 * @brief Return the length of the string and leave it in the stack
 * 
 * @param stack Data stack with the string at [0]
 */
PRIVATE
void length_call(Value *stack, int /*tag*/)
{
  int len;

  if (stack[0].str.str_p == NULL)
  {
    stack[0].num = 0;
    return;
  }

  len = strlen(stack[0].str.str_p);

  if (stack[0].str.dynamic_flags == DYNAMIC)
    free(stack[0].str.str_p);

  stack[0].num = len;
}

/**
 * @brief Converts a string to a number and leave it in the stack
 * 
 * @param stack Data stack with the string at [0]
 */
PRIVATE
void strtonum_call(Value *stack, int /*tag*/)
{
  int num;

  if (stack[0].str.str_p == NULL)
  {
    stack[0].num = 0;
    return;
  }

  num = atoi(stack[0].str.str_p);

  if (stack[0].str.dynamic_flags == DYNAMIC)
    free(stack[0].str.str_p);

  stack[0].num = num;
}

/**
 * @brief Converts a number to a string and leave it in the stack
 * 
 * @param stack Data stack with the number at [0]
 */
PRIVATE
void numtostr_call(Value *stack, int /*tag*/)
{
  char *str;

  str = (char *)malloc(21);

  (void) !gcvt((double)stack[0].num, 5, str);

  stack[0].str.str_p = str;
  stack[0].str.dynamic_flags = DYNAMIC;
}

/**
 * @brief Converts a string to a float and leave it in the stack
 * 
 * @param stack Data stack with the string at [0]
 */
PRIVATE
void strtofloat_call(Value *stack, int /*tag*/)
{
  float num;

  if (stack[0].str.str_p == NULL)
  {
    stack[0].flo = 0.0;
    return;
  }

  num = (float)atof(stack[0].str.str_p);

  if (stack[0].str.dynamic_flags == DYNAMIC)
    free(stack[0].str.str_p);

  stack[0].flo = num;
}

/**
 * @brief Converts a float to a string and leave it in the stack
 * 
 * @param stack Data stack with the float at [0]
 */
PRIVATE
void floattostr_call(Value *stack, int /*tag*/)
{
  char *str;

  str = (char *)malloc(21);

  (void) !gcvt((double)stack[0].flo, 5, str);

  stack[0].str.str_p = str;
  stack[0].str.dynamic_flags = DYNAMIC;
}

/**
 * @brief Converts a number to a float and leave it in the stack
 * 
 * @param stack Data stack with the float at [0]
 */
PRIVATE
void numtofloat_call(Value *stack, int /*tag*/)
{

  stack[0].flo = (float)(stack[0].num);
}

/**
 * @brief Converts a float to a number and leave it in the stack
 * 
 * @param stack Data stack with the float at [0]
 */
PRIVATE
void floattonum_call(Value *stack, int /*tag*/)
{

  stack[0].num = (long)(stack[0].flo);
}

/**
 * @brief Destroy all the items in a set a and leave it empty
 * 
 * @param stack Data stack with the BTState of the set at [0]
 */
PRIVATE
void empty_set(Value *stack, int /*tag*/)
{
  eng_destroy_set((BTState *)stack[0].num);
}

/**
 * @brief Generates a string from a format string (similar to those used in sprintf) and 
 *      a set of parameters referenced in the format string
 * 
 * @param stack Data stack with the format string at [0] and followed by the rest of parameters
 */
PRIVATE
void sprintf_call(Value *stack, int /*tag*/)
{
  gprintf_call(stack, SPRINTF);
}

/**
 * @brief print to stdout a string formed from a format string (similar to those used in sprintf) and 
 *      a set of parameters referenced in the format string
 * 
 * @param stack Data stack with the format string at [0] and followed by the rest of parameters
 */
PRIVATE
void printf_call(Value *stack, int /*tag*/)
{
  gprintf_call(stack, PRINTF);
}

/**
 * @brief print to a file identified by a number a string formed from a format string (similar 
 *      to those used in sprintf) and a set of parameters referenced in the format string
 *      The filename will be /tmp/eng_<pid>.<number>  where pis is the pid of the process and
 *      the number is the first parameter passed to fprintf.
 *      fprintf(<number>, <format>, <param>, <param>, ...)
 * 
 * @param stack Data stack with the format string at [0] and followed by the rest of parameters
 */
PRIVATE
void fprintf_call(Value *stack, int /*tag*/)
{
  gprintf_call(stack, FPRINTF);
}

//
// Static functions (gprintf and related)
//

/**
 * @brief Gets a number in the string and advance the pointer in it
 * 
 * @param s Pointer in the string
 * @return int Number found
 */
static int skip_atoi(const char **s)
{
  int i = 0;

  while (is_digit(**s))
    i = i * 10 + *((*s)++) - '0';
  return i;
}

/**
 * @brief Gets a number in base 10 of a number in other base
 * 
 * @param num Original number
 * @param base Base of the number
 * @return int Final base 10 value
 */
int do_div(int *num, int base)
{
  int r;
  r = *num % base;
  *num = *num / base;
  return (r);
}

/**
 * @brief Format a number
 * 
 * @param str Where to store the formated number
 * @param num Number to format
 * @param base Base of the number to write
 * @param size Number os characters padded by spaces
 * @param precision Number of digits 0 padded by left
 * @param type Format options
 * @return char* returns the end of str that is not \0 closed
 */
static char *format_number(char *str, int num, int base, int size, int precision, int type)
{
  char c, sign, tmp[36];
  const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i;

  if (type & SMALL)
    digits = "0123456789abcdefghijklmnopqrstuvwxyz";
  if (type & LEFT)
    type &= ~ZEROPAD;
  if (base < 2 || base > 36)
    return 0;
  c = (type & ZEROPAD) ? '0' : ' ';
  if (type & SIGN && num < 0)
  {
    sign = '-';
    num = -num;
  }
  else
    sign = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
  if (sign)
    size--;
  if (type & SPECIAL)
    if (base == 16)
      size -= 2;
    else if (base == 8)
      size--;
  i = 0;
  if (num == 0)
    tmp[i++] = '0';
  else
    while (num != 0)
      tmp[i++] = digits[do_div(&num, base)];
  if (i > precision)
    precision = i;
  size -= precision;
  if (!(type & (ZEROPAD + LEFT)))
    while (size-- > 0)
      *str++ = ' ';
  if (sign)
    *str++ = sign;
  if (type & SPECIAL)
    if (base == 8)
      *str++ = '0';
    else if (base == 16)
    {
      *str++ = '0';
      *str++ = digits[33];
    }
  if (!(type & LEFT))
    while (size-- > 0)
      *str++ = c;
  while (i < precision--)
    *str++ = '0';
  while (i-- > 0)
    *str++ = tmp[i];
  while (size-- > 0)
    *str++ = ' ';
  return str;
}

/**
 * @brief Format a float
 * 
 * @param str Where to store the formated float
 * @param num Float to format
 * @param size Number os characters padded by spaces
 * @param precision Number of digits 0 including decimals
 * @param float_class class of float (g, e, f)
 * @param type Format options
 * @return char* returns the end of str that is not \0 closed
 */
static char *format_float(char *str, double num, int size, int precision, int float_class, int type)
{
  int len, decpt, sign;
  char buff[2000];
  char *tmp;
  char *buf = str;
  char sign_ch;
  int n;

  if (precision < 0)
    precision = 6;

  if (num < 0)
  {
    sign_ch = '-';
    num = -num;
  }
  else
    sign_ch = (type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);

  if (sign_ch)
    size--;

  switch (float_class)
  {
    case 'e':
    case 'E':
      tmp = ecvt(num, precision + 1, &decpt, &sign);
      break;
    case 'f':
      tmp = fcvt(num, precision, &decpt, &sign);
      break;
    case 'g':
    case 'G':
      tmp = gcvt(num, precision, buff); // , (type & SPECIAL), &buff);
      break;
  }

  switch (float_class)
  {
    case 'e':
    case 'E':
      if (decpt - 1 >= -99 && decpt - 1 <= 99)
        size -= 4;
      else
        size -= 5;
      if (tmp[1] || (type & SPECIAL))
        size--; // El '.'
      break;
    case 'f':
      if (decpt == 0)
        size--; // '0' initial
      if (tmp[decpt] || (type & SPECIAL))
        size--; // El '.'
      break;
  }

  len = strlen(tmp);
  size -= len;

  if (!(type & LEFT))
    while (size-- > 0)
      *str++ = ' ';

  if (sign_ch)
    *str++ = sign_ch;

  switch (float_class)
  {
    case 'e':
    case 'E':
      sprintf(str, "%.1s%s%s%c%+.2d", tmp,
              ((tmp[1] || (type & SPECIAL)) ? "." : ""),
              tmp + 1,
              float_class, decpt - 1);
      break;
    case 'f':
      if (decpt == 0)
        *str++ = '0';

      sprintf(str, "%.*s%s%s", decpt, tmp,
              ((tmp[decpt] || (type & SPECIAL)) ? "." : ""),
              tmp + decpt);
      break;
    case 'g':
      strcpy(str, tmp);
      break;
  }

  str += strlen(str);
  while (size-- > 0)
    *str++ = ' ';
  return str;
}

/**
 * @brief Print all the variants of printf taken the parameters from the stack
 * 
 * @param stack Data stack with the parameters
 * @param prtype Printf type (SPRINT, PRINTF, FPRINTF)
 */
static void gprintf_call(Value *stack, int prtype)
{
  char *fmt;
  int len;
  int i;
  int of;
  char *str;
  char *s;
  int *ip;
  int sign, decpt;
  static int outfile_flag[5] = {1, 1, 1, 1, 1};
  static FILE *outfile[5];
  char outfile_name[128];

  int flags; /* flags to format_xxxx() */

  int field_width; /* width of output field */
  int precision;   /* min. # of digits for integers; max
          number of chars for from string */
  int qualifier;   /* 'h', 'l', or 'L' for integer fields */
  char buf[1024];

  int nargs = 0;

  if (prtype == FPRINTF)
  {
    of = stack[nargs++].num;
    if (of < 0)
      of = 0;
    if (of > 4)
      of = 4;
    if (outfile_flag[of] == 1)
    {
      outfile_flag[of] = 0;
      sprintf(outfile_name, "/tmp/eng_%d.%d", getpid(), of);
      outfile[of] = fopen(outfile_name, "a");
    }
  }

  fmt = (char *)stack[nargs++].str.str_p;

  for (str = buf; *fmt; ++fmt)
  {
    if (*fmt != '%')
    {
      *str++ = *fmt;
      continue;
    }

    /* process flags */
    flags = 0;
  repeat:
    ++fmt; /* this also skips first '%' */
    switch (*fmt)
    {
      case '-':
        flags |= LEFT;
        goto repeat;
      case '+':
        flags |= PLUS;
        goto repeat;
      case ' ':
        flags |= SPACE;
        goto repeat;
      case '#':
        flags |= SPECIAL;
        goto repeat;
      case '0':
        flags |= ZEROPAD;
        goto repeat;
    }

    /* get field width */
    field_width = -1;
    if (is_digit(*fmt))
      field_width = skip_atoi((const char **)&fmt);
    else if (*fmt == '*')
    {
      /* it's the next argument */
      fmt++;
      field_width = stack[nargs++].num;
      if (field_width < 0)
      {
        field_width = -field_width;
        flags |= LEFT;
      }
    }

    /* get the precision */
    precision = -1;
    if (*fmt == '.')
    {
      fmt++;
      if (*fmt == '-')
        fmt++;
      if (is_digit(*fmt))
        precision = skip_atoi((const char **)&fmt);
      else if (*fmt == '*')
      {
        /* it's the next argument */
        fmt++;
        precision = stack[nargs++].num;
        if (precision < 0)
          precision = 0;
      }
    }

    /* It is not used
// get the conversion qualifier
qualifier = -1;
if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
qualifier = *fmt;
++fmt;
}
    */

    switch (*fmt)
    {
    case 'c':
      if (!(flags & LEFT))
        while (--field_width > 0)
          *str++ = ' ';
      *str++ = (unsigned char)stack[nargs++].num;
      while (--field_width > 0)
        *str++ = ' ';
      break;

    case 's':
      s = stack[nargs].str.str_p;
      len = strlen(s);
      if (precision < 0)
        precision = len;
      else if (len > precision)
        len = precision;

      if (!(flags & LEFT))
        while (len < field_width--)
          *str++ = ' ';
      for (i = 0; i < len; ++i)
        *str++ = *s++;
      while (len < field_width--)
        *str++ = ' ';

      if (stack[nargs].str.dynamic_flags == DYNAMIC)
        free(stack[nargs].str.str_p);
      nargs++;

      break;

    case 'o':
      str = format_number(str, stack[nargs++].num, 8,
                          field_width, precision, flags);
      break;

    case 'p':
      if (field_width == -1)
      {
        field_width = 8;
        flags |= ZEROPAD;
      }
      str = format_number(str,
                          (unsigned long)stack[nargs++].num, 16,
                          field_width, precision, flags);
      break;

    case 'x':
      flags |= SMALL;
    case 'X':
      str = format_number(str, stack[nargs++].num, 16,
                          field_width, precision, flags);
      break;

    case 'd':
    case 'i':
      flags |= SIGN;
    case 'u':
      str = format_number(str, stack[nargs++].num, 10,
                          field_width, precision, flags);
      break;
    /* It is not useful
case 'n':
ip = (int *)stack[nargs++].num;
*ip = (str - buf);
break;
    */
    case 'f':
    case 'g':
    case 'G':
    case 'e':
    case 'E':
      str = format_float(str, stack[nargs++].flo,
                         field_width, precision, *fmt, flags);
      break;
    default:
      if (*fmt != '%')
        *str++ = '%';
      if (*fmt)
        *str++ = *fmt;
      else
        --fmt;
      break;
    }
  }
  *str = '\0';

  if (prtype == FPRINTF)
    if (stack[1].str.dynamic_flags == DYNAMIC)
      free(stack[1].str.str_p);
    else if (stack[0].str.dynamic_flags == DYNAMIC)
      free(stack[0].str.str_p);

  if (prtype == SPRINTF)
  {
    stack[0].str.str_p = strdup(buf);
    stack[0].str.dynamic_flags = DYNAMIC;
  }
  else if (prtype == PRINTF)
  {
    printf("%s", buf);
    fflush(stdout); // This print always flushes stdout
  }
  else if (prtype == FPRINTF)
  {
    fprintf(outfile[of], "%s", buf);
    fflush(outfile[of]); // This print always flushes stdout
  }
}
