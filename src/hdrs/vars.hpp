/**
 * @file vars.hpp
 * @author Francisco Alcaraz
 * @brief Class Var and references
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


#include "patterns.hpp"

#ifndef VARS____HH_INCLUDED
#define VARS____HH_INCLUDED

struct Reference
{
    Pattern *_patt;
    int      _attr;
 
    void setup(Pattern *pattern, int attr)
    {
       _patt = pattern;
       _attr = attr;
    };
};

class Var
{
   private:
     char *_name;
     int _type;
     int _private;
     Reference _first_ref;
     Reference _last_ref;
     Reference _best_last_ref;
   public:
     Var(char *name, int type, Pattern *pattern, int attr);
     ~Var();
     static Var *find_var(char *name, int equality=0);
     static Var *find_var(Pattern *pattern, int attr);
     char *name() {return _name;    };
     int type()   { return _type;   };
     int attr()   { return _last_ref._attr;   };
     Pattern *pattern()   { return _last_ref._patt;};
     void modify_pattern(Pattern *patt, int attr);
     static void delete_vars();
     static void new_priv_pool();
     static void end_priv_pool();
     static void reassoc_vars(Pattern *patt);
     static int ret_equality();
     static void add_equality(int attr);
};

#endif
