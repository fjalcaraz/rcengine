/**
 * @file utf8.cpp
 * @author Francisco Alcaraz
 * @brief Management of UTF-8 encoded strings
 * @version 0.1
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utf8str.hpp"


/**
 * @brief Get the length in UTF-8 characters of a string
 * 
 * @return int The length
 */
int
utf8str::length() {
    int bytelen = strlen(this->str);
    int len = 0;

    for (int n=0; n<bytelen; len++) {
        n += this->chrlen(this->str[n]);
    }
    return len;
}

/**
 * @brief Gets the substring from two indices
 * 
 * @param ini Index of the first character including, starting by 1
 * @param end Index of the last character including, starting by 1   substr("abcd", 3, 3) = "c" 
 * @return char* resulting string
 */
char *
utf8str::substr(int ini, int end) {
    int bytelen = strlen(this->str);
    int n, ini_n;
    int len = 0;

    if (ini > 0) ini--;
    for (n=0; n<bytelen && len < ini; len++)
        n += this->chrlen(this->str[n]);
    for (ini_n = n; n<bytelen && (end<0 || len < end); len++)
        n += this->chrlen(this->str[n]);
    
    return strndup(this->str + ini_n, n - ini_n);
}

/**
 * @brief Returns the string converted to lowercase characters 
 *      Calculate the lowercase of all the ITF-8 characters considered in LATIN-1
 * 
 * @param lower Where to leave the struing converted to lowercase
 * @param n Number of caracters. -1 = all (default value) 
 * @return char* Dynamic string with the copy of te original string converted to lowercase
 */
char *
utf8str::tolower(char *lower, int n) {

    if (!this->str) return nullptr;

    char extra = 0;
    unsigned char *p, *q;
    
    for (p = (unsigned char *)this->str, q=(unsigned char *)lower; n != 0 && *p; n--, p++, q++) {
        if (((extra && ((*p >= 0x80) && (*p <= 0xbf))) || ((!extra) && (*p >= 0x41) && (*p<= 0x5a)))
            && (((*p & 0x7f) + extra) >= 0x40)
                && (((*p & 0x7f) + extra) <= 0x5f))
            *q = *p + 0x20;
        else
            *q = *p;
        if (extra)
            extra = 0;
        else if (*p == 0xc3)
            extra = 0x40;
    }
    *q = 0;

    return lower;
}

/**
 * @brief Returns the string converted to uppercase characters 
 *      Calculate the uppercase of all the ITF-8 characters considered in LATIN-1
 * 
 * @param upper Where to leave the struing converted to uppercase
 * @param n Number of caracters. -1 = all (default value) 
 * @return char* Dynamic string with the copy of te original string converted to uppercase
 */
char *
utf8str::toupper(char *upper, int n) {
    
    if (!this->str) return nullptr;

    char extra = 0;
    unsigned char *p, *q;
    
    for (p = (unsigned char *)this->str, q=(unsigned char *)upper; n != 0 && *p; n--, p++, q++) {
        if (((extra && ((*p >= 0x80) && (*p <= 0xbf))) || ((!extra) && (*p >= 0x61) && (*p <= 0x7a)))
            && (((*p & 0x7f) + extra) >= 0x60)
                && (((*p & 0x7f) + extra) <= 0x7f))
            *q = *p - 0x20;
        else
            *q = *p;
        if (extra)
            extra = 0;
        else if (*p == 0xc3)
            extra = 0x40;
    }
    *q = 0;

    return upper;
}