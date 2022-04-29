/**
 * @file vars.cpp
 * @author Francisco Alcaraz
 * @brief Variables Management
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <string.h>
#include <stdlib.h>

#include "engine.h"

#include "codes.h"
#include "config.hpp"
#include "rules.hpp"
#include "vars.hpp"

PRIVATE Var *var[MAXVARS];
PRIVATE int top_var=0;
PRIVATE int beg_priv_pool = -1;

PRIVATE int equality_vars = -1;
PRIVATE int equality_attrs[MAXATTRS];

/**
 * @brief Construct a new Var:: Var object
 * 
 * @param name Name of the Variable
 * @param type Type of the Var
 * @param pattern associated Pattern 
 * @param attr Attribute number of the object that would match the pattern
 */
Var::Var(char *name, int type, Pattern *pattern, int attr) 
{
    _name = name;
    _type = type;
    _first_ref.setup(pattern, attr);
    _last_ref.setup(pattern, attr);
    _best_last_ref.setup(pattern, attr);
    _private = (beg_priv_pool != -1);
    var[top_var++]=this;
}
 
/**
 * @brief Destroy the Var:: Var object
 * 
 */
Var::~Var()
{
     free(_name);
}

/**
 * @brief Begin a new private pool of vars
 * 
 */
void
Var::new_priv_pool()
{
    beg_priv_pool = top_var;
    equality_vars = 0;
}

/**
 * @brief End of the private pool of vars
 * 
 */
void
Var::end_priv_pool()
{
    int n;
    int m;

    for (n=beg_priv_pool; n<top_var; n++)
    {
      if (var[n]->_type != TYPE_PATTERN)
      {
        if (var[n]->_first_ref._attr == var[n]->_last_ref._attr)
        {
           comp_war("Assuming equality on var \"%s\"\n", var[n]->_name);
           var[n]->_first_ref._patt -> assume_equality_in_set(var[n]->_first_ref._attr);
           var[n]->_private = FALSE; // If it is equal to other it can be used outside the private pool
           for (m=0; m<equality_vars; m++)
             if(equality_attrs[m] == var[n]->_first_ref._attr)
               equality_attrs[m] = -1;
        }
      }
    }

    for (m=0; m<equality_vars; m++)
      if (equality_attrs[m] >= 0)
        Pattern::curr_pattern()-> assume_equality_in_set(equality_attrs[m]);

    beg_priv_pool = -1;
    equality_vars = -1;
}

/**
 * @brief Try to enhance the pattern referenced by a variable last referenced by a given pattern
* 
 * @param patt The pattern
 */
void
Var::reassoc_vars(Pattern *patt)
{
    int n;

    for (n=0; n<top_var; n++)
    {
      if (var[n]->_last_ref._patt == patt && var[n]->_type != TYPE_PATTERN)
      {

        // According to the life state.

        // It's OK if the best reference have the same life state than this last
        if (var[n]->_last_ref._patt->life_st() == var[n]->_best_last_ref._patt->life_st())
        {
          // According to Set/Single

          //It's OK if the best reference has the same set condition than this last
          if (var[n]->_last_ref._patt->is_set() == var[n]->_best_last_ref._patt->is_set())
          {
            var[n]->_best_last_ref = var[n]->_last_ref;
          }

          // Choose as best reference that no set
          else 
          {
            if (var[n]->_last_ref._patt->is_set())
              var[n]->_last_ref = var[n]->_best_last_ref;
            else
              var[n]->_best_last_ref = var[n]->_last_ref;
          }
        }

        // Choose as best reference that affirmative pattern (NORMAL)
        else if (var[n]->_last_ref._patt->life_st() == ST_NORMAL)
        {
          var[n]->_best_last_ref = var[n]->_last_ref;
        }
      else
      {
        // The current best reference is actually taken
        var[n]->_last_ref = var[n]->_best_last_ref;
      }
    }
  }
}

/**
 * @brief 
 * 
 * @param patt 
 * @param attr 
 */
void 
Var::modify_pattern(Pattern *patt, int attr)
{ 
   _last_ref.setup(patt, attr);
}

/**
 * @brief Find a variable by name
 * 
 * @param name Name of the variable
 * @param equality If it is in a equality expression
 * @return Var* The Variable found
 */
Var *
Var::find_var(char *name, int equality)
{
    int n;
    for (n=0; n<top_var; n++)
    {
      if (strcmp(var[n]->_name, name) == 0)
      {
        // A variable may stay as private inside a Set when it is used in
        // a equality internal expression of the pattern of the objects that form the Set
        // by example { obj(att1 a, attr2 a) } means that every object in the set must have obj.attr1 = obj.attr2
        // but they vary (have different values) among the objects of the set
        
        if (beg_priv_pool>=0)
           if (equality == FALSE && Pattern::curr_pattern() != var[n]->_last_ref._patt && var[n]->_last_ref._patt->life_st() == ST_OPTIONAL)
              comp_err("Cannot access optional pattern attributes from a set\n");


        if (var[n]->_private && (beg_priv_pool<0 || n<beg_priv_pool))
        {
          if (Pattern::curr_pattern()->life_st() == ST_OPTIONAL)
            comp_err("Cannot access a non constant attribute in a set from an optional pattern\n");
          else
            comp_war("Accessing a non constant attribute in a set. first item will be used\n");
        }

        return var[n];
      }
    }
    return NULL;
}

/**
 * @brief Find a Variable that references to a Pattern and Attribute
 * 
 * @param pattern Pattern of the var
 * @param attr Attribute of the objects that match the Pattern
 * @return Var* The variable found
 */
Var *
Var::find_var(Pattern *pattern, int attr)
{
    int n;
    int selec;

    selec=-1;
    for (n=0; n<top_var; n++)
    {
      if (var[n]->_last_ref._patt == pattern && 
          var[n]->_last_ref._attr == attr && 
          var[n]->_type != TYPE_PATTERN)
      {
        // Let's select those not private if possible
        
        if (selec==-1 || var[selec]->_private)
          selec=n;
      }
    }

    if (selec != -1)
    {
        // See the comments above

        if (var[selec]->_private && (beg_priv_pool<0 || selec<beg_priv_pool))
        {
          if (Pattern::curr_pattern()->life_st() == ST_OPTIONAL)
            comp_err("Cannot access a non constant attribute in a set from an optional pattern\n");
          else
            comp_war("Accessing a non constant attribute in a set. first item will be used\n");
        }

        return var[selec];
    }
    return NULL;
}

/**
 * @brief Delete all the defined vars (it is done at the end of every rule)
 * 
 */
void
Var::delete_vars()
{
    int n;

    for (n=0; n<top_var; n++)
      delete var[n];

    top_var=0;
}

/**
 * @brief Return the number of equality vars
 * 
 * @return int 
 */
int
Var::ret_equality()
{
    return (equality_vars);
}

/**
 * @brief Add a new equality among attributes
 * 
 * @param attr The attrubute that is equal to others in the array
 */
void
Var::add_equality(int attr)
{
   equality_attrs[equality_vars++] = attr;
}

