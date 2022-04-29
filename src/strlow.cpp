/**
 * @file strlow.cpp
 * @author Francisco Alcaraz
 * @brief Utilities to copy string to lowercase
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <ctype.h>

#include "strlow.hpp"

/**
 * @brief Copy the src string to dest changing the characters to lowercase
 * 
 * @param dest Destination string
 * @param src Source String
 * @return char* Destination string
 */
char *strlowercpy(char *dest, char *src)
{
  while (*src)
   *dest++ = tolower(*src++);

  *dest = 0;
  return dest;
}

/**
 * @brief Copy n characters of the src string to dest changing the characters to lowercase
 * 
 * @param dest Destination string
 * @param src Source String
 * @param n number of characters
 * @return char* Destination String
 */
char *strlowerncpy(char *dest, char *src, int n)
{
  while (n>0 && *src)
  {
     *dest++ = tolower(*src++);
     n--;
  }
  if (n>0) *dest = 0;
  return dest;
}
 
 

