/**
 * @file rules.cpp
 * @author Francisco Alcaraz
 * @brief Management of Rules, Rulesets and Packages
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <btree.hpp>

#include "codes.h"
#include "config.hpp"
#include "vars.hpp"
#include "error.hpp"
#include "patterns.hpp"
#include "rules.hpp"
#include "strlow.hpp"

struct PackageInfo
{
  BTree rulesets;
};
 
struct RulesetInfo
{
  char name[MAXNAME];
  BTree rules;
 
  RulesetInfo(char *nm) { strlowerncpy(name, nm, MAXNAME);};
};


PRIVATE char package_name[MAXNAME];
PRIVATE char curr_ruleset[MAXNAME];
PRIVATE char *curr_rulename;
PRIVATE ULong curr_rulecat;
PRIVATE int default_tw = NO_TIMED;
PRIVATE int curr_tw = NO_TIMED;
PRIVATE ULong prod_code[LEN_BASIC_PROD_NODE + 256 + 1];
PRIVATE int n_obj_impl;
PRIVATE int in_LHS=FALSE;
PRIVATE int in_rule=FALSE;
PRIVATE int rule_flags=0;
PRIVATE PackageInfo pkg;
PRIVATE RulesetInfo *curr_ruleset_info;
PRIVATE int compare_rulesets(const void * rset1, const void *rset2, va_list);
PRIVATE void free_rulesets(void *rset, va_list);
PRIVATE void reset_rulesets(const void *rset, va_list);
PRIVATE Node *curr_prod_node;

/**
 * @brief Define the name of the Rule Package
 * 
 * @param name Name of the Package
 */
PUBLIC void
def_package(char *name)
{
  strcpy(package_name, name);
  free(name);
}

/**
 * @brief Define a new RuleSet 
 * 
 * @param ruleset_name The RuleSet name
 */
PUBLIC void
def_ruleset(char *ruleset_name)
{
  RulesetInfo sample(ruleset_name);
  RulesetInfo **rset;

  rset = (RulesetInfo **)pkg.rulesets.Insert(&sample, compare_rulesets);

  if (*rset == &sample)
    *rset = new RulesetInfo(sample);

  curr_ruleset_info = *rset;

  strcpy(curr_ruleset, ruleset_name);
  free(ruleset_name);
}

/**
 * @brief End of current RuleSet
 * 
 */
PUBLIC void
end_ruleset()
{
  curr_ruleset_info = NULL;
}

/**
 * @brief Define default time window
 * 
 * @param sec Seconds of time window 
 * @return PUBLIC 
 */
PUBLIC void
default_time_window(int sec)
{
  default_tw = sec;
}

/**
 * @brief Define a new Rule in the current RuleSet
 * 
 * @param name Name of the Rule
 * @param cat Priority category
 * @param tw Rule time window
 */
PUBLIC void
def_rule(char *name, int cat, int tw)
{
  if (tw == 0)
    comp_err("Rule time window cannot be 0\n");

  curr_rulename  = name;
  curr_rulecat   = cat;

  if (tw == DEF_TIME)
    curr_tw      = default_tw;
  else
    curr_tw      = tw;

  in_LHS = TRUE;
  in_rule = TRUE;
  rule_flags = 0;
}

/**
 * @brief Set the current rule execution flags, that is a tag mask that will control when the
 *        rule must execute the RHS part (some action must be executed under the current tag)
 * 
 * @param flags EXEC_INSERT, EXEC_MODIFY. EXEC_DELETE, EXEC_ALL
 */
PUBLIC void
set_curr_rule_exec_flags(int flags)
{
  rule_flags |= flags;

  if (in_rule && !in_LHS)
     curr_prod_node->set_code(PROD_NODE_FLAGS_POS, rule_flags);
}

/**
 * @brief Returns the current rule execution flags, that is a tag mask that will control when the
 *        rule must execute the RHS part (some action must be executed under the current tag)
 * 
 */
PUBLIC int
curr_rule_exec_flags()
{
  return rule_flags;
}

/**
 * @brief Returns the current Rule Time Window
 * 
 */
PUBLIC int
curr_rule_tw()
{
  return curr_tw;
}

/**
 * @brief Add a Production Node and register it in the rule set
 *      The structure of a PROD node is as follows
 *        [0] : PROD
 *        [1] : rulename
 *        [2] : Ruleset
 *        [3] : Rulecat
 *        [4] : Number of implied Objects in the RHS (default 0)
 *        [5] : Memory of the Prod Node (Only in case of Implied Objects: Compound that store 
 *                                       the context at left and the implied objects at the right)
 *        [6] : Flags of Execution the Prod Node
 *        in case of implied objects
 *        [7] : Position of the first implied object
 *        [8] : Position of second impled object
 *        ....
 */
PUBLIC
void def_production()
{
  curr_prod_node = Pattern::add_prod_node();

  prod_code[0] = PROD;
  prod_code[PROD_NODE_RULENAME_POS]  = (ULong)curr_rulename;
  prod_code[PROD_NODE_RULESETNM_POS] = (ULong)strdup(curr_ruleset);
  prod_code[PROD_NODE_RULECAT_POS]   = curr_rulecat;
  prod_code[PROD_NODE_NOBJIMP_POS]   = 0L;
  prod_code[PROD_NODE_MEM_POS]     = (ULong)new BTree();
  prod_code[PROD_NODE_FLAGS_POS]     = curr_rule_exec_flags();
  n_obj_impl = 0;
  curr_rulename = NULL;

  curr_prod_node->add_code(LEN_BASIC_PROD_NODE, prod_code);

  curr_ruleset_info->rules.Insert(curr_prod_node, BTree::compareEq);

  in_LHS = FALSE;
}

/**
 * @brief Declare in the Production node that a new Implied Object is in the RHS of rule.
 *      It inserts this information in the current production node
 * 
 * @param pos Position of the object
 */
PUBLIC
void obj_imply_at(int pos)
{
  curr_prod_node->insert_code(PROD_START_OBJ_POS + n_obj_impl, 1);
  curr_prod_node->set_code(PROD_START_OBJ_POS + n_obj_impl, pos);

  curr_prod_node->set_code(PROD_NODE_NOBJIMP_POS, ++n_obj_impl);
}

/**
 * @brief Notify the end of the rule
 *      Insert this code at the end of production node
 *      Release all the patterns and vars
 * 
 */
PUBLIC void
end_rule()
{
  ULong code[1];

  code[0] = ENDRULE;
  Pattern::add_code_to_curr_node(1, code);

  Pattern::delete_patterns();
  Var::delete_vars();
  in_rule = FALSE;
}

/**
 * @brief Returns if the Prod Node has memory 
 *      Prod nodes only have memory in case of implied object to maintain the required 
 *      relation between context (LHS) and the implied (RHS)
 * 
 * @param code 
 */
PUBLIC int 
prod_has_memory(ULong *code)
{
  return (code[PROD_NODE_NOBJIMP_POS]!=0L);
}

/**
 * @brief Returns if the compilation is in the LHS of the rule
 * 
 *  @return bool
 */
PUBLIC int 
in_the_LHS()
{
  return in_rule && in_LHS;
}

/**
 * @brief Returns if the compilation is in the RHS of the rule
 * 
 *  @return bool
 */
PUBLIC int 
in_the_RHS()
{
  return in_rule&& !in_LHS;
}

/**
 * @brief Returns if the compilation is in a rule
 * 
 *  @return
 */
PUBLIC int
inside_rule()
{
  return in_rule;
}
 
/**
 * @brief Free all the Package (Freeing its RuleSets)
 * 
 */
PUBLIC void
free_package()
{
   pkg.rulesets.Free(free_rulesets);
   if (curr_rulename != NULL)
     free(curr_rulename);
   curr_rulename = NULL;
}

/**
 * @brief BTree walking function to release a RuleSet. It frees all the rules
 * 
 * @param rset RulSet to free
 */
PRIVATE void
free_rulesets(void *rset, va_list)
{
   ((RulesetInfo *)rset)->rules.Free(Node::free_rule);
   delete (RulesetInfo *)rset;
}

/**
 * @brief Free (remove) a RuleSet by Name
 * 
 * @param name Name of the RuleSet to free
 */
PUBLIC void
free_ruleset(char *name)
{
   RulesetInfo sample(name);
   RulesetInfo *rset;

   rset = (RulesetInfo *)pkg.rulesets.Delete(&sample, compare_rulesets);

   if (rset != NULL)
   {
     rset->rules.Free(Node::free_rule);
     delete rset;
   }
}

/**
 * @brief Free the current RuleSet
 * 
 */
PUBLIC void
free_curr_ruleset()
{
   if (curr_ruleset_info != NULL)
   {
     curr_ruleset_info->rules.Free(Node::free_rule);
     pkg.rulesets.Delete(curr_ruleset_info, compare_rulesets);
     delete curr_ruleset_info;
     curr_ruleset_info = NULL;
   }
}

/**
 * @brief Reset the Package (cleaning all the internal memories)
 * 
 * @return PUBLIC 
 */
PUBLIC void
reset_package()
{
   pkg.rulesets.WalkBy(reset_rulesets);
}
 
/**
 * @brief BTree walk function to reset all the RuleSets os a Package
 * 
 * @param rset RuleSet to reset
 */
PRIVATE void
reset_rulesets(const void *rset, va_list)
{
   ((RulesetInfo *)rset)->rules.WalkBy(Node::reset_rule);
}
 
/**
 * @brief Reset a RuleSet by name (cleaning all the internal memories)
 * 
 * @param name Name of the RuleSet
 */
PUBLIC void
reset_ruleset(char *name)
{
   RulesetInfo sample(name);
   RulesetInfo *rset;
 
   rset = (RulesetInfo *)pkg.rulesets.Find(&sample, compare_rulesets);
 
   if (rset != NULL)
     rset->rules.WalkBy(Node::reset_rule);
}

/**
 * @brief Compare function between RuleSets used for the BTree of the Package
 * 
 * @param rset1 RuleSet 1
 * @param rset2 RuleSet2
 * @return <0, 0, >0 The comparison is made by name
 */
PRIVATE
int compare_rulesets(const void * rset1, const void *rset2, va_list)
{
   return strncmp(((RulesetInfo *)rset1)->name,
                  ((RulesetInfo *)rset2)->name,
                  MAXNAME);
}



