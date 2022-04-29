/**
 * @file lex.cpp
 * @author Francisco Alcaraz
 * @brief Lexical reading of the rule packages
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "lex_p.hpp"

extern YYSTYPE eng_lval;
int eng_debug;

PRIVATE int init_lex = 0;

/**
 * @brief Get a character
 *
 * @param input buffer
 * @return char
 */
inline char GETC(char *&input)
{
  return *input++;
}

/**
 * @brief Unget a character
 *
 * @param c the character
 * @param input the input buffer
 */
inline void UNGETC(char c, char *&input)
{
  input--;
  (*input) = c;
}

/**
 * @brief Read a rule package from a string and control the enable of traces
 *
 * @param text Rule package to compile
 * @return int error
 */
PUBLIC int
read_pkg(char *text)
{
  int err;

  input = text;
  init_lex = PKG_ID;
  line_num = 1;

  err = eng_parse();

  return (err);
}

/**
 * @brief Read a Rule Set from a string and control the enable of traces
 *
 * @param text Rule Set to compile
 * @return int error
 */
PUBLIC int
read_rset(char *text)
{
  int err;

  input = text;
  init_lex = RSET_ID;
  line_num = 1;

  err = eng_parse();

  return (err);
}

/**
 * @brief Control the activation of lex debugging
 *
 * @param debug The status of debugging
 */
PUBLIC
void set_lex_debug(int debug)
{
  eng_debug = debug;
}

/**
 * @brief Print a compiler error
 *
 * @param s The error
 */
PUBLIC
void eng_error(const char *s)
{
  comp_err("%s\n", s);
}

/**
 * @brief Get the curr line where the compiler was when the error thrown
 *
 * @return int the line number
 */
PUBLIC
int get_curr_line()
{
  return line_num;
}

/**
 * @brief Lexical analyzer. Its return tokens to the syntax analyzer
 *      as Rule Package or Rule Set may be read, init_lex flag the starting point and
 *      its value is the first token that will setup the syntax analyzer
 *
 * @return token number, the token value when reading an scalar (by example a number) is stored at eng_lval
 */
PUBLIC
int eng_lex()
{
  static char c;

  // We readd the first character only at init state
  // Initially also we return a control token to select the syntax of Package of Ruleset

  if (init_lex)
  {
    int val = init_lex;
    c = GETC(input);
    init_lex = 0;
    return val;
  }

  first_printable(c);

  if (c == 0)
    return (0);

  if (isalpha(c))
    return read_ident(c);

  if (isdigit(c))
    return read_number(c);

  /*
   * Otherwise the read character is returned and read the next
   */

  if (c == '\'')
    return read_char(c);

  if (c == '"')
    return read_str(c);

  return read_special(c);
}

/**
 * @brief Move formar the input pointer until a non blank character
 *
 * @param c the first non blank found
 */
PRIVATE
void first_printable(char &c)
{

  // Identify the blank spaces while the EOF is not reached
  // Also the new line characters are being counted (line number) and
  // the commentaries are ignored

  do
  {

    // Blank chars

    while (c <= ' ' && c != 0)
    {
      if (c == '\n')
        line_num++;
      c = GETC(input);
    }

    // Commentaries

    if (c == ';')
    {
      while ((c = GETC(input)) != '\n' && c != 0)
        ;
    }

  } while (c != 0 && c <= ' ');
}

/**
 * @brief Read an identifier. It also identify the reserved words returning the specific token
 *
 * @param c first character (non blank)
 * @return int token (is a number)
 */
PRIVATE
int read_ident(char &c)
{

  TokenText_Type *tok_txt; // Here are the texts in lowercase of all reserved words
  char *pbuff;
  int len;
  static char buffer[MAXNAME + 1];

  pbuff = buffer;
  len = 0;

  do
  {
    *pbuff++ = tolower(c); // All the identifiers are in lowercase!!
    len++;
    c = GETC(input);
  } while (c != 0 && (isalnum(c) || c == '_') && len < MAXNAME);

  if (c != 0 && (isalnum(c) || c == '_') && len >= MAXNAME)
    comp_err("Identifier too long\n");

  *pbuff = '\0';

  for (tok_txt = TokenText;
       tok_txt->token != 0 && strcmp(tok_txt->txt, buffer) != 0;
       tok_txt++)
    ;

  // Also we check for null, true or false
  if (tok_txt->token != 0)
  {
    if (tok_txt->token == TRUE_VAL)
      eng_lval.num = TRUE_VALUE;
    if (tok_txt->token == FALSE_VAL)
      eng_lval.num = FALSE_VALUE;
    if (tok_txt->token == NULL_STR)
      eng_lval.num = NULL_STR_VALUE;
    return (tok_txt->token);
  }

  else
  {
    // Simple identifier, may be a pattern var if follower by ':'
    eng_lval.ident = strdup(buffer);
    first_printable(c);
    if (inside_rule() && c == ':')
    {
      c = GETC(input);
      return PATT_VAR;
    }
    else
    {
      return (IDENT); /* IDENTIFIER */
    }
  }
}

/**
 * @brief Read a number
 * 
 * @param c First non blank character
 * @return the token type
 */
PRIVATE
int read_number(char &c)
{
  // Read an integer (long) or a float

  char *pbuff;
  int len;
  static char buffer[MAXNUMBER + 1];

  pbuff = buffer;
  len = 0;

  do
  {
    *pbuff++ = c;
    len++;
    c = GETC(input);
  } while (c != 0 && (isdigit(c) || c == '.') && len < MAXNUMBER);

  if (c != 0 && isdigit(c) && len >= MAXNUMBER)
    comp_err("Number too long\n");

  if ((c == 'E' || c == 'e') && len < MAXNUMBER)
  {
    // Store E/e

    *pbuff++ = c;
    len++;
    c = GETC(input);

    // Check for the sign +/-

    if ((c == '+' || c == '-') && len < MAXNUMBER)
    {
      *pbuff++ = c;
      len++;
      c = GETC(input);
    }

    while (c != 0 && isdigit(c) && len < MAXNUMBER)
    {
      *pbuff++ = c;
      len++;
      c = GETC(input);
    }
  }

  if (c != 0 && isdigit(c) && len >= MAXNUMBER)
    comp_err("Number too long\n");

  *pbuff = '\0';

  eng_lval.num = (int)strtol(buffer, &pbuff, 10);
  if (*pbuff == '\0')
  {
    return INTEGER;
  }

  eng_lval.flo = (float)strtod(buffer, &pbuff);
  if (*pbuff == '\0')
  {
    return FLOAT;
  }
  else
  {
    return SINTAX_ERROR;
  }
}

/**
 * @brief Read the characters one by one. Also read numbers and control some escaped characters 
 * 
 * @param c First non blank character
 * @return PRIVATE 
 */
PRIVATE
int read_char(char &c)
{
  char next_c;

  c = GETC(input);
  if (c == '\\')
  {

    // Special chars

    c = GETC(input);
    switch (c)
    {
    case 'n':
      c = '\n';
      break;
    case 'r':
      c = '\r';
      break;
    case 't':
      c = '\t';
      break;
    case '0': // Octal
      c = 0;
      while (isdigit(next_c = GETC(input)))
        c = (c << 3) + (next_c - '0');
      UNGETC(next_c, input);
      break;
    default:
      if (isdigit(c)) // Decimal
      {
        c -= '0';
        while (isdigit(next_c = GETC(input)))
          c = c * 10 + (next_c - '0');
        UNGETC(next_c, input);
      }
      else
        comp_err("Unknown char\n");
      break;
    }
  }

  next_c = GETC(input);

  if (next_c == '\'')
  {
    eng_lval.num = c;
    c = GETC(input);
    return CHAR;
  }
  else
  {
    return SINTAX_ERROR;
  }
}

/**
 * @brief Read a string controlling some escaped characters
 * 
 * @param c First non blank character
 * @return PRIVATE 
 */
PRIVATE
int read_str(char &c)
{

  char *pbuff;
  int len;
  static char buffer[MAXSTR + 1];
  char next_c;

  pbuff = buffer;
  len = 0;
  while ((c = GETC(input)) != 0 && c != '"' && len < MAXSTR)
  {
    if (c == '\\')
    {
      c = GETC(input);
      switch (c)
      {
      case '"':
        c = '"';
        break;
      case 'n':
        c = '\n';
        break;
      case 'r':
        c = '\r';
        break;
      case 't':
        c = '\t';
        break;
      case '0': // Octales
        c = 0;
        while (isdigit(next_c = GETC(input)))
          c = (c << 3) + (next_c - '0');
        UNGETC(next_c, input);
        break;
      default:
        if (isdigit(c)) // Decimal
        {
          c -= '0';
          while (isdigit(next_c = GETC(input)))
            c = c * 10 + (next_c - '0');
          UNGETC(next_c, input);
        }
        else
          comp_err("Unknown char\n");
        break;
      }
    }

    *pbuff++ = c;
    len++;
  }

  *pbuff = '\0';

  // If the string has not been close at the end of line/file we return an
  //     unknown token for the syntax (SYNTAX_ERROR)
  // Else return the text found

  if (c != '"')
  {
    return (SINTAX_ERROR);
  }
  else
  {
    char *txtdir;

    c = GETC(input);

    txtdir = strdup(buffer);
    eng_lval.str = txtdir;
    return STRING;
  }
}

/**
 * @brief Deal with special symbols that has their own tokens (such as '->' = IMPL)
 * 
 * @param c First non blank character
 * @return the token found (or char found)
 */
PRIVATE
int read_special(char &c)
{
  char c_init;

  if (c == '-')
  {
    c = GETC(input);
    if (c == '>')
    {
      c = GETC(input);
      return IMPL;
    }
    else
    {
      return '-';
    }
  }

  if (c == '!')
  {
    c = GETC(input);
    if (c == '=')
    {
      c = GETC(input);
      return NEQ;
    }
    else
    {
      return '!';
    }
  }

  if (c == '.')
  {
    c = GETC(input);
    if (c == '.')
    {
      c = GETC(input);
      if (c == '.')
      {
        c = GETC(input);
        return DOTDOTDOT;
      }
      else
      {
        UNGETC(c, input);
        c = '.';
        return '.';
      }
    }
    else
      return '.';
  }

  c_init = c;
  c = GETC(input);
  return (c_init);
}


