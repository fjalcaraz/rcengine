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

#ifndef UTF8___HH_INCLUDED
#define UTF8___HH_INCLUDED

class utf8str {

    private:
        char *str;

    public:
        inline utf8str(char *str) {
            this->str = str;
        };
        inline int chrlen(unsigned char c) {
            int len;
            
            if ((c & 0x80) == 0)
                len = 1;
            else if ((c & 0xE0) == 0xC0)
                len = 2;
            else if ((c & 0xF0) == 0xE0)
                len = 3;
            else if ((c & 0xF8) == 0xF0)
                len = 4;
            else { 
                fprintf(stderr, "Wrong character 0x%x\n", c);
                return 0;
            }
            return len;
        }
        int length();
        char * substr(int ini, int end = -1);
        char * tolower(char *lower, int n = -1);
        char * toupper(char *lower, int n = -1);
        
};

#endif