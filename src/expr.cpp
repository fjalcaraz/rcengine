/**
 * @file expr.cpp
 * @author Francisco Alcaraz
 * @brief Expressions compiler.
 *        The expressions are compiled generating a tree of operands data and variable that is a mirror of
 *        what the syntax is analysing, taking into consideration the precedence of the operators
 *        This Expression is finnally flattened generating the final RETE code
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdlib.h>

#include "engine.h"

#include "codes.h"
#include "lex.hpp"
#include "config.hpp"
#include "functions.hpp"
#include "nodes.hpp"
#include "patterns.hpp"
#include "classes.hpp"
#include "vars.hpp"
#include "rules.hpp"

#include "expr.hpp"

PRIVATE Pattern *patt_of_ref(long ref);
PRIVATE int attr_of_ref(long ref);
PRIVATE int type_of_ref(long ref);
PRIVATE int check_type(Expression **exp_p, int bool_in_stack);
PRIVATE int optimize_negations(Expression **exp_p, int in_log_not);
PRIVATE int pre_evaluate_exp(Expression **exp_p);
PRIVATE void pre_eval_arith(Expression *exp);
PRIVATE void pre_eval_minus(Expression *exp);
PRIVATE void classify_expr(Expression *exp,
                           Expression **intra_exp_p, Expression **inter_exp_p);
PRIVATE int find_node_type(Expression *exp);
PRIVATE void join_required_objects(Expression *exp, int expr_type);
PRIVATE int compile_expresion(Expression *exp, int node_type);
PRIVATE void add_code(int lcode, ...);
PRIVATE void free_code();

PRIVATE ULong *curr_code = NULL;
PRIVATE int curr_lcode = 0;
PRIVATE StValues curr_status;

#define hasOneArg(op) (op == MINUS || op == TTRUE || op == TFALSE || op == EVAL || op == NOT || op == CAST_INT || op == CAST_OBJ || op == CAST_BOO || op == CAST_CHA)

/**
 * @brief Insert a node with the code to match the value of an attribute 
 *        of current object with a simple value
 * 
 * @param type Type of the attribute
 * @param ... simple value (will depend on the type)
 */
PUBLIC
void match_val(int type, ...)
{
  ULong data;
  float float_num;

  va_list list;
  int curr_attr, attr_type;

  va_start(list, type);

  ObjClass::curr_attr_type(&curr_attr, &attr_type);

  if (type != attr_type)
    comp_err("Incorrect type in matching\n");

  if (type == TYPE_BOOL || type == TYPE_CHAR || type == TYPE_PATTERN)
    type = TYPE_NUM; // When compiled, the types are simplified to NUM, STR or FLOAT

  add_code(2, PUSHS | type, curr_attr);

  if (type == TYPE_BOOL)
  {
    data = va_arg(list, int);
    if (data == FALSE_VALUE)
      add_code(1, TFALSE);
    else
      add_code(1, TTRUE);
  }
  else
  {
    switch (type)
    {
    case TYPE_STR:
      data = (ULong)va_arg(list, char *);
      break;
    case TYPE_CHAR:
      type = TYPE_NUM; /* continues ...*/
    case TYPE_NUM:
      data = (ULong)va_arg(list, int);
      break;
    case TYPE_FLO:
      float_num = (float)va_arg(list, double);
      data = *(ULong *)(&float_num);
      break;
    }

    add_code(3, PUSH | type, data,
             TEQ | type);
  }

  Pattern::curr_pattern()->add_intra_node(curr_lcode, curr_code);
  free_code();

  va_end(list);
}

/**
 * @brief Insert a node with the code to match the value of an attribute 
 *        of current object with the attribute value of another object in the rule identified by the reference
 *        References are encoded: MPPP PPPP AAAA AAAA (M: Memory side, P: Position, A: Attribute number)
 *        When a reference is used to bind a variable to an object attribute, 
 *        is set the 16th bit to denote a PATTERN_TYPE var (var bound to an object)
 *        The node created may be INTRA (restriction applied to the same object) or INTER (affecting to
 *        other object that has to be joined to current in an AND Node)
 * 
 * @param type Final type if casting done
 * @param ref Reference to locate the other object and its attribute
 */
PUBLIC
void match_type_ref(int type, long ref)
{
  int typeofref;
  int curr_attr, curr_type, true_curr_type;
  Pattern *curr_pattern, *ref_pattern;
  int curr_mem, curr_pos;
  int ref_mem, ref_pos, ref_attr;
  ULong data1, data2;
  Var *var;

  ObjClass::curr_attr_type(&curr_attr, &curr_type);
  curr_pattern = Pattern::curr_pattern();

  // The type is checked and if it is new

  typeofref = type_of_ref(ref);

  if (type == NO_CAST_TYPE)
  {
    if (typeofref != curr_type)
      comp_err("Incorrect type in matching\n");
  }
  else
  {
    if (type != curr_type)
      comp_err("Incorrect type in matching\n");
    if (typeofref == TYPE_STR || typeofref == TYPE_FLO)
      comp_err("Incorrect argument type in casting\n");
    curr_type = typeofref;
  }

  ref_pattern = patt_of_ref(ref);
  ref_attr = attr_of_ref(ref);
  true_curr_type = curr_type;

  if (curr_type == TYPE_BOOL || curr_type == TYPE_CHAR || curr_type == TYPE_PATTERN)
    curr_type = TYPE_NUM;

  if (ref_pattern != curr_pattern)
  {
    // AND Node

    Pattern::new_inter_assoc(curr_rule_tw());
    ref_pattern->add_to_assoc(AS_SINGLE);
    curr_pattern->add_to_assoc(AS_SINGLE);

    curr_pattern->get_inter_pos(&curr_mem, &curr_pos);
    ref_pattern->get_inter_pos(&ref_mem, &ref_pos);

    data1 = ((ref_mem << 15) | (ref_pos << 8) | ref_attr);
    data2 = ((curr_mem << 15) | (curr_pos << 8) | curr_attr);

    if (true_curr_type == TYPE_PATTERN && ref_attr == 0)
      add_code(5, PUSHO, data1,
               PUSHS | curr_type, data2,
               TEQ | curr_type);
    else
      add_code(5, PUSHS | curr_type, data1,
               PUSHS | curr_type, data2,
               TEQ | curr_type);

    Pattern::add_code_to_curr_node(curr_lcode, curr_code);
    free_code();

    if ((var = Var::find_var(ref_pattern, ref_attr)) != NULL)
      var->modify_pattern(curr_pattern, curr_attr);
  }

  else
  {
    // Let's verify if it a definition or a reuse in other attribute (equality)

    if (curr_attr != ref_attr)
    {
      // It will be a INTRA node with mem = 0, pos = 0

      add_code(5, PUSHS | curr_type, ref_attr,
               PUSHS | curr_type, curr_attr,
               TEQ | curr_type);
      Pattern::curr_pattern()->add_intra_node(curr_lcode, curr_code);
      free_code();
      if ((var = Var::find_var(ref_pattern, ref_attr)) != NULL)
        var->modify_pattern(curr_pattern, curr_attr);
    }
  }
}

/**
 * @brief Register last reference variable to be equal to current attribute
 * 
 */
PUBLIC
void equality_ref()
{
  int curr_attr, curr_type;

  if (Var::ret_equality() == -1)
    comp_err("= (equality) only valid on sets\n");

  ObjClass::curr_attr_type(&curr_attr, &curr_type);
  Var::add_equality(curr_attr);
}

/**
 * @brief Set the status of the current object
 *        The status can be ST_NORMAL, ST_NEGATED, ST_OPTIONAL, ST_DELETED
 * 
 * @param status The status
 */
PUBLIC
void set_status(StValues status)
{
  curr_status = status;
}

/**
 * @brief Create a INTRA Node that check if the classname of the object it equal to the given
 * 
 * @param classname The given classname
 */
PUBLIC
void match_class(char *classname)
{
  ObjClass::set_curr_class_LHR(classname, curr_status);
  free(classname);
}

/**
 * @brief Set current Attribute
 * 
 * @param name Name of the attribute
 */
PUBLIC
void match_attr(char *name)
{
  ObjClass::set_curr_attr(name);
  free(name);
}

/**
 * @brief Define a Variable assigned to an object in the LHS of Rule, what is called a Pattern
 *    The variable will be a reference to that object
 * 
 
 * 
 * @param varname varname associated to the object pattern
 */
PUBLIC
void def_patt_var(char *varname)
{

  Pattern *curr_pattern;
  Var *var;

  // Check if it is new

  var = Var::find_var(varname);

  if (var != NULL)
  {
    comp_err("Pattern variable %s already used\n", varname);
    free(varname);
  }
  else
  {
    // When a variable is bound to an object (PATTERN_TYPE) 
    // is set the 16th bit (or passed 0x10000 as attribute)
    curr_pattern = Pattern::curr_pattern();
    new Var(varname, TYPE_PATTERN, curr_pattern, 0x10000);
  }
}

/**
 * @brief Returns the number of the pattern associated to a var. The pattern number is the order 
 *        of the object in the LHS of the rule, its position.
 * 
 * @param varname Name of the var
 */
PUBLIC
int patt_num_of_var(char *varname)
{
  int patt_no;
  Var *var;

  /* Check if it is new */

  var = Var::find_var(varname);

  if (var == NULL)
    comp_err("Variable %s not previously bound\n", varname);

  if (var->type() != TYPE_PATTERN)
    comp_err("Incorrect variable type\n");

  patt_no = var->pattern()->pattern_no();

  free(varname);
  return patt_no;
}

/**
 * @brief Find a var and return its reference
 *        References are encoded: MPPP PPPP AAAA AAAA (M: Memory side, P: Position, A: Attribute number)
 * 
 * @param varname Variable name found
 * @param create_on_missing: If the variable should be created in case of not found
 * @param equality Check if true (in find var) if the var is nor pointing to some OPTIONAL object that may nor be present
 *    Optional patterns may have expression that match other objects with them but it is unidirectional, cannot be set equalities 
 *    from the optional to other object not optional
 */
PUBLIC
int ref_of_var(char *varname, int create_on_missing, int equality)
{
  Var *var;
  long ref;
  int curr_attr, curr_type;
  Pattern *curr_pattern;

  /* Check if it is new */

  var = Var::find_var(varname, equality);

  if (var != NULL)
  {
    ref = (((var->pattern()->pattern_no()) << 8) | var->attr());
    free(varname);
  }
  else
  {
    // No previous
    if (!create_on_missing)
      comp_err("Variable %s not previously bound\n", varname);

    ObjClass::curr_attr_type(&curr_attr, &curr_type);
    curr_pattern = Pattern::curr_pattern();
    new Var(varname, curr_type, curr_pattern, curr_attr);
    ref = (((curr_pattern->pattern_no()) << 8) | curr_attr);
  }
  return ref;
}

/**
 * @brief Gets the reference from a pattern var and an attribute name of the class of the object pointed by the var
 *        References are encoded: MPPP PPPP AAAA AAAA (M: Memory side, P: Position, A: Attribute number)
 * 
 * @param pvarname Varname
 * @param attr_name Attribute name
 * @param equality: Check (in find_var), if can be used in a equality expresssion
 */
PUBLIC
int ref_of_pvar(char *pvarname, char *attr_name, int equality)
{
  Var *pvar;
  int n_patt, n_attr;

  pvar = Var::find_var(pvarname, equality);

  if (pvar == NULL)
    comp_err("Variable %s not previously bound\n", pvarname);

  if (pvar->type() != TYPE_PATTERN)
    comp_err("Incorrect variable type\n");

  n_patt = pvar->pattern()->pattern_no();
  n_attr = pvar->pattern()->class_of_patt()->attr_index(attr_name);
  if (n_attr == -1)
    comp_err("Attribute %s not defined\n", attr_name);

  free(pvarname);
  free(attr_name);
  return ((n_patt << 8) | n_attr);
}

/**
 * @brief Gets a reference from a pattern number (object at that position in the LHS), and an attribute name of that object
 *        References are encoded: MPPP PPPP AAAA AAAA (M: Memory side, P: Position, A: Attribute number)
 * 
 * @param pnum Pattern Number
 * @param attr_name Attribute name
 */
PUBLIC
int ref_of_pnum(int pnum, char *attr_name)
{
  Pattern *patt;
  int n_attr;

  patt = Pattern::get_pattern(pnum);
  n_attr = patt->class_of_patt()->attr_index(attr_name);
  if (n_attr == -1)
    comp_err("Attribute %s not defined\n", attr_name);

  free(attr_name);
  return ((pnum << 8) | n_attr);
}

/**
 * @brief Get a Pattern from a reference
 *        References are encoded: MPPP PPPP AAAA AAAA (M: Memory side, P: Position, A: Attribute number)
 * 
 * @param ref The reference
 */
PRIVATE
Pattern *patt_of_ref(long ref)
{
  return Pattern::get_pattern((int)((ref >> 8) & 0x07F));
}

/**
 * @brief Get the attribute number from a reference
 *        References are encoded: MPPP PPPP AAAA AAAA (M: Memory side, P: Position, A: Attribute number)
 * 
 * @param ref The reference
 */
PRIVATE
int attr_of_ref(long ref)
{
  return ref & 0xFF;
}

/**
 * @brief Get the type from a reference
 *        References are encoded: MPPP PPPP AAAA AAAA (M: Memory side, P: Position, A: Attribute number)
 *        When a variables is bound to a pattern it has as attribute 0x10000 (16th bit set)
 * 
 * @param ref The reference
 */
PRIVATE
int type_of_ref(long ref)
{
  // see def_patt_var
  if (ref & 0x10000)
    return TYPE_PATTERN;
  else
    return Pattern::get_pattern((int)(ref >> 8))->class_of_patt()->attr_type((int)(ref & 0xFF));
}

/**
 * @brief Begin a set pattern definition
 * 
 */
PUBLIC
void begin_set()
{
  Var::new_priv_pool();
  Pattern::new_set_context();
}

/**
 * @brief End of a set pattern definition
 * 
 * @return PUBLIC 
 */
PUBLIC
void end_set()
{
  Pattern::add_set_node();
  Var::end_priv_pool();
  Pattern::end_set_context();
}

/**
 * @brief End of a pattern definition
 * 
 * @return PUBLIC 
 */
PUBLIC
void end_of_pattern()
{
  Var::reassoc_vars(Pattern::curr_pattern());
}

// Expressions

/**
 * @brief Create an expresion node with a single value
 * 
 * @param type Type of the value
 * @param value Value
 * @return the resulting Expression
 */
PUBLIC
Expression *create_val(int type, StackType value)
{
  Expression *val;

  val = new Expression;
  val->op = PUSH;
  val->is_rel = FALSE;

  val->data.val_exp.type = type;
  val->data.val_exp.val = value;
  return (val);
}

/**
 * @brief Create an Expression node with relation (some opperand with expressions at left and at right sides)
 * 
 * @param op Opperand
 * @param lexp Left side Expression
 * @param rexp Right side Expression
 * @return The top node Expression
 */
PUBLIC
Expression *create_rel(int op, Expression *lexp, Expression *rexp)
{
  Expression *rel;

  rel = new Expression;

  rel->op = op;
  rel->is_rel = TRUE;
  rel->data.rel_exp.lexp = lexp;
  rel->data.rel_exp.rexp = rexp;
  return (rel);
}

/**
 * @brief Create an Expression node with a function call 
 * 
 * @param name Name of function
 * @param arg Argument values list
 * @return The top node Expression
 */
PUBLIC
Expression *create_fun(char *name, Argument *arg)
{
  Expression *val;

  val = new Expression;

  val->op = FCALL;
  val->is_rel = FALSE;
  val->data.fun_exp.name = name;
  val->data.fun_exp.args = arg;
  return (val);
}

/**
 * @brief Create an Argument (for a function call) from a Expression
 * 
 * @param exp Expression node of the argument
 */
PUBLIC
Argument *create_arg(Expression *exp)
{
  Argument *arg;

  arg = new Argument;

  arg->arg_exp = exp;
  arg->sig = (Argument *)NULL;
  return (arg);
}

/**
 * @brief add to an argument the next argument in function call order
 * 
 * @param arg1 Previous Argument
 * @param arg2 Next Argument
 * @return Argument * arg1 that is the head of the tail
 */
PUBLIC
Argument *concat_arg(Argument *arg1, Argument *arg2)
{
  arg1->sig = arg2;
  return arg1;
}

/**
 * @brief Create a primitive call
 * 
 * @param op Opperand = Primitive Code
 * @param n_args Number of Arguments 
 * @param ... The arguments will came as a variable number of Expressions
 * @return The final Expresion (top node)
 */
PUBLIC
Expression *create_primitive(int op, int n_args, ...)
{
  Argument *curr_arg, *new_arg;
  Expression *val;
  va_list list;

  val = new Expression;
  val->op = op;
  val->is_rel = FALSE;
  val->data.fun_exp.name = NULL;
  val->data.fun_exp.args = curr_arg = NULL;

  va_start(list, n_args);

  while (n_args > 0)
  {
    new_arg = new Argument;
    new_arg->arg_exp = va_arg(list, Expression *);
    new_arg->sig = NULL;
    if (curr_arg == NULL)
      val->data.fun_exp.args = new_arg;
    else
      concat_arg(curr_arg, new_arg);
    curr_arg = new_arg;

    n_args--;
  }
  va_end(list);
  return val;
}

/**
 * @brief Compile an Expresion
 *    First the logical negations are expanded using Morgan
 *    Then the expressions are classified according it affects to one node or to several.
 *    This division with generate INTRA nodes or INTER nodes
 *    The INTRA expressions will generate simple INTRA nodes hunging from the last intra node of the referenced object
 *    The INTER expressions will generate INTER nodes to join all the affected objects and then will go to the last INTER node created
 * 
 * @param exp The Expression
 */
PUBLIC
void match_exp(Expression *exp)
{

  Expression *inter_exp;
  Expression *intra_exp;
  Pattern *curr;
  int type;

  type = check_type(&exp, FALSE);
  if (type != TYPE_BOOL)
    comp_err("Incorrect expression type\n");

  (void)optimize_negations(&exp, FALSE);
  pre_evaluate_exp(&exp);

  inter_exp = intra_exp = (Expression *)NULL;

  // Keep the current pattern before classify the expression
  curr = Pattern::curr_pattern();
  classify_expr(exp, &intra_exp, &inter_exp);

  if (intra_exp != (Expression *)NULL)
  {
    while (intra_exp->op == TAND)
    {
      Expression *tand_expr;

      Pattern::reset_access();
      join_required_objects(intra_exp->data.rel_exp.lexp, INTRA);
      (void)compile_expresion(intra_exp->data.rel_exp.lexp, INTRA);
      if (Pattern::curr_pattern() == NULL)
        curr->add_intra_node(curr_lcode, curr_code);
      else
        Pattern::curr_pattern()->add_intra_node(curr_lcode, curr_code);
      free_code();
      tand_expr = intra_exp;
      intra_exp = intra_exp->data.rel_exp.rexp;
      delete tand_expr;
    }
    Pattern::reset_access();
    join_required_objects(intra_exp, INTRA);
    (void)compile_expresion(intra_exp, INTRA);
    if (Pattern::curr_pattern() == NULL)
      curr->add_intra_node(curr_lcode, curr_code);
    else
      Pattern::curr_pattern()->add_intra_node(curr_lcode, curr_code);
    free_code();
  }

  if (inter_exp != (Expression *)NULL)
  {
    while (inter_exp->op == TAND)
    {
      Expression *tand_expr;

      Pattern::reset_access();
      Pattern::new_inter_assoc(curr_rule_tw());
      join_required_objects(inter_exp->data.rel_exp.lexp, INTER_AND);
      (void)compile_expresion(inter_exp->data.rel_exp.lexp, INTER_AND);
      Pattern::add_code_to_curr_node(curr_lcode, curr_code);
      free_code();
      tand_expr = inter_exp;
      inter_exp = inter_exp->data.rel_exp.rexp;
      delete tand_expr;
    }
    Pattern::reset_access();
    Pattern::new_inter_assoc(curr_rule_tw());
    join_required_objects(inter_exp, INTER_AND);
    (void)compile_expresion(inter_exp, INTER_AND);
    Pattern::add_code_to_curr_node(curr_lcode, curr_code);
    free_code();
  }

  // Restore the current pattern
  Pattern::set_curr_pattern(curr, AS_SINGLE);
}

/**
 * @brief Check the type in all the Expression is coherent
 * 
 * @param exp where the pointer to te Expresion is stored
 * @param bool_in_stack In case of boolean expresions if the result must reside in the stack or as the final result
 * @return the Type of the expression
 */
PRIVATE
int check_type(Expression **exp, int bool_in_stack)
{
  int type_l, type_r, type;
  int data_in_stack;
  Argument *arg;
  int n;
  int attr_ref;
  long ref;
  Pattern *patt_ref;

  switch ((*exp)->op)
  {
  case CAST_BOO:
  case CAST_CHA:
  case CAST_OBJ:
  case CAST_INT:
    arg = (*exp)->data.fun_exp.args;
    type_r = check_type(&arg->arg_exp, TRUE);
    if (type_r != TYPE_NUM && type_r != TYPE_CHAR && type_r != TYPE_BOOL &&
        type_r != TYPE_PATTERN)
      comp_err("Mismatch type in expression\n");
    else
      switch ((*exp)->op)
      {
      case CAST_INT:
        type = TYPE_NUM;
        break;
      case CAST_CHA:
        type = TYPE_CHAR;
        break;
      case CAST_OBJ:
        type = TYPE_PATTERN;
        break;
      case CAST_BOO:
        type = TYPE_BOOL;
        break;
      }

    data_in_stack = TRUE;
    break;
  case TGT:
  case TGE:
  case TEQ:
  case TNE:
  case TLE:
  case TLT:
    type_l = check_type(&((*exp)->data.rel_exp.lexp), TRUE);
    type_r = check_type(&((*exp)->data.rel_exp.rexp), TRUE);
    if (type_l != type_r)
      comp_err("Mismatch type in expression\n");
    else
      type = TYPE_BOOL;
    data_in_stack = FALSE;
    break;
  case TAND:
  case TOR:
    type_l = check_type(&((*exp)->data.rel_exp.lexp), FALSE);
    type_r = check_type(&((*exp)->data.rel_exp.rexp), FALSE);
    if (type_l != TYPE_BOOL || type_r != TYPE_BOOL)
      comp_err("Mismatch type in expression\n");
    else
      type = TYPE_BOOL;
    data_in_stack = FALSE;
    break;
  case TNOT:
    if (bool_in_stack)
    {
      (*exp)->op = NOT;
      type = check_type(&((*exp)->data.rel_exp.rexp), TRUE);
      data_in_stack = TRUE;
    }
    else
    {
      type = check_type(&((*exp)->data.rel_exp.rexp), FALSE);
      data_in_stack = FALSE;
    }
    if (type != TYPE_BOOL)
      comp_err("Mismatch type in expression\n");
    break;
  case ADD:
  case SUB:
  case MUL:
  case DIV:
    type_l = check_type(&((*exp)->data.rel_exp.lexp), TRUE);
    type_r = check_type(&((*exp)->data.rel_exp.rexp), TRUE);
    if (type_l != type_r || (type_l != TYPE_NUM && type_l != TYPE_FLO))
      comp_err("Mismatch type in expression\n");
    else
      type = type_l;
    data_in_stack = TRUE;
    break;
  case MINUS:
    type = check_type(&((*exp)->data.rel_exp.rexp), TRUE);
    if (type != TYPE_NUM && type != TYPE_FLO)
      comp_err("Mismatch type in expression\n");
    data_in_stack = TRUE;
    break;
  case PUSH:
    type = (*exp)->data.val_exp.type;

    if (type == TYPE_REF)
    {
      ref = (*exp)->data.val_exp.val.num;
      attr_ref = attr_of_ref(ref);
      patt_ref = patt_of_ref(ref);

      if ((patt_ref->life_st() == ST_NEGATED || patt_ref->life_st() == ST_OPTIONAL) && in_the_RHS())
      {
        comp_err("Cannot reference negated/optional patterns in the RHS\n");
      }
      type = type_of_ref(ref);
    }
    data_in_stack = TRUE;
    break;
  case PUSHT:
    ref = (*exp)->data.fun_exp.args->arg_exp->data.val_exp.val.num;
    patt_ref = Pattern::get_pattern((int)ref);
    if (patt_ref->is_set())
    {
      comp_err("Cannot obtain a single time from a set\n");
    }
    if ((patt_ref->life_st() == ST_NEGATED || patt_ref->life_st() == ST_OPTIONAL) && in_the_RHS())
    {
      comp_err("Cannot reference negated/optional patterns in the RHS\n");
    }
    type = TYPE_NUM;
    data_in_stack = TRUE;
    break;
  case FCALL:
  {
    int nargsf = num_of_args((*exp)->data.fun_exp.name);
    int fhvar_num_of_args = func_has_var_num_of_args((*exp)->data.fun_exp.name);
    n = 0;
    for (arg = (*exp)->data.fun_exp.args; arg != NULL; arg = arg->sig)
    {
      if (n < nargsf)
      {
        if (check_type(&arg->arg_exp, TRUE) != arg_type((*exp)->data.fun_exp.name, n))
          comp_err("Mismatch argument type\n");
      }
      else
        check_type(&arg->arg_exp, TRUE); // check_type must be executed
                                         // on all arguments !!
      n++;
    }

    if (n < nargsf || (n > nargsf && !fhvar_num_of_args))
      comp_err("Incorrect number of arguments passed to function %s\n",
               (*exp)->data.fun_exp.name);

    type = func_type((*exp)->data.fun_exp.name);
    data_in_stack = TRUE;
  }
  break;
  case COUNT:
    ref = (*exp)->data.fun_exp.args->arg_exp->data.val_exp.val.num;
    patt_ref = Pattern::get_pattern((int)ref);
    if (!patt_ref->is_set())
    {
      comp_err("Reference variable not of a SET pattern\n");
    }
    type = TYPE_NUM;
    data_in_stack = TRUE;
    break;
  case SUMS:
  case PRDS:
  case MINS:
  case MAXS:
    ref = (*exp)->data.fun_exp.args->arg_exp->data.val_exp.val.num;
    patt_ref = patt_of_ref(ref);
    if (!patt_ref->is_set())
    {
      comp_err("Reference variable not of a SET pattern\n");
    }
    if ((*exp)->op != MINS && (*exp)->op != MAXS && 
        type_of_ref(ref) != TYPE_NUM && type_of_ref(ref) != TYPE_FLO)
    {
      comp_err("Reference to a non numeric attribute\n");
    }
    type = type_of_ref(ref);
    data_in_stack = TRUE;
    break;
  case CONCS:
    ref = (*exp)->data.fun_exp.args->arg_exp->data.val_exp.val.num;
    patt_ref = patt_of_ref(ref);
    if (!patt_ref->is_set())
    {
      comp_err("Reference variable not of a SET pattern\n");
    }
    if (type_of_ref(ref) != TYPE_STR)
    {
      comp_err("Reference to a non string attribute\n");
    }
    if (check_type(&((*exp)->data.fun_exp.args->sig->arg_exp), FALSE) != TYPE_STR)
    {
      comp_err("Mismatch argument type\n");
    }
    type = TYPE_STR;
    data_in_stack = TRUE;
    break;
  default:
    comp_err("INTERNAL ERROR : Unknown node operator in expresion\n");
  }

  if (type == TYPE_BOOL)
  {
    if (bool_in_stack && !data_in_stack)
      (*exp) = create_rel(EVAL, NULL, (*exp));
    if (!bool_in_stack && data_in_stack)
      (*exp) = create_rel(TTRUE, NULL, (*exp));
  }
  return type;
}

/**
 * @brief Applies Morgan to the not expressions. It is specially required in INTRA node expressions
 * 
 * @param exp_p Where the analyzed Expression pointer is stored
 * @param do_compl If is being evaluated the complementary of the expression (negation)
 * @return in the complementary was done 
 */
PRIVATE
int optimize_negations(Expression **exp_p, int do_compl)
{
  Expression *exp = (*exp_p);
  Argument *arg;
  int res;
  int did_comp = do_compl;

  switch (exp->op)
  {
  case NOT:
    res = optimize_negations(&(exp->data.rel_exp.rexp), !do_compl);
    // If the complementary could be done delete the NOT expression
    if (!do_compl == res)
    {
      *(exp_p) = exp->data.rel_exp.rexp;
      delete exp;
    }
    break;
  case TNOT:
    (void)optimize_negations(&(exp->data.rel_exp.rexp), !do_compl);
    *(exp_p) = exp->data.rel_exp.rexp;
    delete exp;
    break;
  case FCALL:
    for (arg = exp->data.fun_exp.args; arg != (Argument *)NULL; arg = arg->sig)
      (void)optimize_negations(&(arg->arg_exp), FALSE);
    did_comp = FALSE;
    break;
  case PUSH:
    if (do_compl && exp->data.val_exp.type == TYPE_BOOL)
    {
      if (exp->data.val_exp.val.num == TRUE_VALUE)
        exp->data.val_exp.val.num = FALSE_VALUE;
      else
        exp->data.val_exp.val.num = TRUE_VALUE;
    }
    did_comp = FALSE;
    break;

  default:
    if (exp->is_rel)
    {
      if (do_compl)
      {
        switch (exp->op)
        {
        case TGT:
          exp->op = TLE;
          do_compl = FALSE;
          break;
        case TGE:
          exp->op = TLT;
          do_compl = FALSE;
          break;
        case TEQ:
          exp->op = TNE;
          do_compl = FALSE;
          break;
        case TNE:
          exp->op = TEQ;
          do_compl = FALSE;
          break;
        case TLE:
          exp->op = TGT;
          do_compl = FALSE;
          break;
        case TLT:
          exp->op = TGE;
          do_compl = FALSE;
          break;
        case TAND:
          exp->op = TOR;
          break;
        case TOR:
          exp->op = TAND;
          break;
        case TTRUE:
          exp->op = TFALSE;
          do_compl = FALSE;
          break;
        case TFALSE:
          exp->op = TTRUE;
          do_compl = FALSE;
          break;
        }
      }

      // Operands with only one operator (rexpr)
      if (!hasOneArg(exp->op))
        (void)optimize_negations(&(exp->data.rel_exp.lexp), do_compl);
      (void)optimize_negations(&(exp->data.rel_exp.rexp), do_compl);
    }
    break;
  }
  return did_comp;
}

/**
 * @brief Try to preevaluate the value of an expression if all are scalars
 * 
 * @param exp_p Where the pointer to the expresions is stored
 * @return if it was changed
 */
PRIVATE
int pre_evaluate_exp(Expression **exp_p)
{

  Argument *arg;
  Expression *exp = (*exp_p);
  int test_eval_l, test_eval_r; // To know if they are arithmetically evaluable

  switch (exp->op)
  {
  case ADD:
  case SUB:
  case MUL:
  case DIV:
    test_eval_l = pre_evaluate_exp(&(exp->data.rel_exp.lexp));
    test_eval_r = pre_evaluate_exp(&(exp->data.rel_exp.rexp));
    if (test_eval_l && test_eval_r)
    {
      pre_eval_arith(exp);
      return (TRUE);
    }
    break;
  case MINUS:
    test_eval_r = pre_evaluate_exp(&(exp->data.rel_exp.rexp));
    if (test_eval_r)
    {
      pre_eval_minus(exp);
      return (TRUE);
    }
    break;
  case TGT:
  case TGE:
  case TEQ:
  case TNE:
  case TLE:
  case TLT:
  case TAND:
  case TOR:
    (void)pre_evaluate_exp(&(exp->data.rel_exp.lexp));
    (void)pre_evaluate_exp(&(exp->data.rel_exp.rexp));
    break;
  case FCALL:
    for (arg = exp->data.fun_exp.args; arg != (Argument *)NULL; arg = arg->sig)
      (void)pre_evaluate_exp(&(arg->arg_exp));
    break;
  case PUSH:
    if (exp->data.val_exp.type == TYPE_NUM ||
        exp->data.val_exp.type == TYPE_FLO ||
        exp->data.val_exp.type == TYPE_CHAR ||
        exp->data.val_exp.type == TYPE_BOOL)
      return (TRUE);
    break;
  case TTRUE:
  case TFALSE:
    (void)pre_evaluate_exp(&(exp->data.rel_exp.rexp));
    break;
  }
  return (FALSE);
}

/**
 * @brief Preevaluate arithmetical expressions if all the data are scalars
 * 
 * @param exp The expression to be evaluated. May be changed by its final value
 */
PRIVATE void
pre_eval_arith(Expression *exp)
{

  Expression *exp_l, *exp_r;
  int etype, type;

  exp_l = exp->data.rel_exp.lexp;
  exp_r = exp->data.rel_exp.rexp;

  type = exp_l->data.val_exp.type;
  etype = effective_type(type);

  exp->data.val_exp.type = type;

  switch (exp->op)
  {
  case ADD:
    if (etype == TYPE_NUM)
      exp->data.val_exp.val.num =
          exp_l->data.val_exp.val.num + exp_r->data.val_exp.val.num;
    else
      exp->data.val_exp.val.flo =
          exp_l->data.val_exp.val.flo + exp_r->data.val_exp.val.flo;
    break;
  case SUB:
    if (etype == TYPE_NUM)
      exp->data.val_exp.val.num =
          exp_l->data.val_exp.val.num - exp_r->data.val_exp.val.num;
    else
      exp->data.val_exp.val.flo =
          exp_l->data.val_exp.val.flo - exp_r->data.val_exp.val.flo;
    break;
  case MUL:
    if (etype == TYPE_NUM)
      exp->data.val_exp.val.num =
          exp_l->data.val_exp.val.num * exp_r->data.val_exp.val.num;
    else
      exp->data.val_exp.val.flo =
          exp_l->data.val_exp.val.flo * exp_r->data.val_exp.val.flo;
    break;
  case DIV:
    if (etype == TYPE_NUM)
      exp->data.val_exp.val.num =
          exp_l->data.val_exp.val.num / exp_r->data.val_exp.val.num;
    else
      exp->data.val_exp.val.flo =
          exp_l->data.val_exp.val.flo / exp_r->data.val_exp.val.flo;
    break;
  }
  exp->op = PUSH;
  exp->is_rel = FALSE;
  delete exp_l;
  delete exp_r;
}

/**
 * @brief Try to preevaluate the unary minus in a expression, if all the subexpression is scalar 
 * 
 * @param exp Expression that is evaluated. May be simplified by a single value
 */
PRIVATE void
pre_eval_minus(Expression *exp)
{

  Expression *exp_r;

  exp_r = exp->data.rel_exp.rexp;

  exp->data.val_exp.type = exp_r->data.val_exp.type;

  if (exp->data.val_exp.type == TYPE_NUM)
    exp->data.val_exp.val.num = -exp_r->data.val_exp.val.num;
  else
    exp->data.val_exp.val.flo = -exp_r->data.val_exp.val.flo;

  exp->op = PUSH;
  exp->is_rel = FALSE;
  delete exp_r;
}

/**
 * @brief Divide the expressions depending if the affects to only one object (intra) or several (inter)
 *    The intra conditions will be converted to INTRA nodes of each object and the inter will generate an INTRA node 
 *    TAND operator allow to divide the expression due it may be considered as a chain of filters to be applied in order (AND off all)
 * 
 * @param exp The expresion
 * @param intra_exp_p The intra part or th expression
 * @param inter_exp_p The inter part of the expression
 */
PRIVATE
void classify_expr(Expression *exp, Expression **intra_exp_p, Expression **inter_exp_p)
{

  int node_type;
  Expression **last_intra, **last_inter;
  Expression *aux;

  while (exp->op == TAND)
  {
    // Reset the patterns access (no current pattern) so first parrent referenced will be
    // taken as current, and if there is any other, the relation will be considered INTER

    Pattern::reset_access();
    node_type = find_node_type(exp->data.rel_exp.lexp);
    if (node_type == INTRA)
    {
      (*intra_exp_p) = exp;
      last_intra = intra_exp_p;
      intra_exp_p = &(exp->data.rel_exp.rexp);
    }
    else
    {
      (*inter_exp_p) = exp;
      last_inter = inter_exp_p;
      inter_exp_p = &(exp->data.rel_exp.rexp);
    }
    exp = exp->data.rel_exp.rexp;
  }

  Pattern::reset_access();
  node_type = find_node_type(exp);

  if (node_type == INTRA)
  {
    (*intra_exp_p) = exp;
    if ((*inter_exp_p) != NULL)
    {
      aux = (*last_inter);
      (*last_inter) = (*last_inter)->data.rel_exp.lexp;
      delete aux;
    }
  }
  else
  {
    (*inter_exp_p) = exp;
    if ((*intra_exp_p) != NULL)
    {
      aux = (*last_intra);
      (*last_intra) = (*last_intra)->data.rel_exp.lexp;
      delete aux;
    }
  }
}

/**
 * @brief Find the type of node need for an Expression, INTRA if only deals with a single pattern, INTER otherwise
 * 
 * @param exp The Expression to evaluate
 * @return Type of NODE: INTRA or INTER_AND
 */
PRIVATE
int find_node_type(Expression *exp)
{
  Argument *arg;
  int node_type;
  Pattern *ref_patt;
  Pattern *curr = Pattern::curr_pattern();

  if (exp->is_rel)
  {
    if (!hasOneArg(exp->op) && find_node_type(exp->data.rel_exp.lexp) == INTER_AND)
      return (INTER_AND);
    else
      return (find_node_type(exp->data.rel_exp.rexp));
  }
  else
  {
    if (exp->op == FCALL)
    {
      arg = exp->data.fun_exp.args;
      node_type = INTRA;
      while (arg != (Argument *)NULL && node_type != INTER_AND)
      {
        node_type = find_node_type(arg->arg_exp);
        arg = arg->sig;
      }
      return (node_type);
    }

    else if (exp->op == PUSH)
    {

      if (exp->data.val_exp.type != TYPE_REF)
        return (INTRA);

      ref_patt = patt_of_ref(exp->data.val_exp.val.num);

      // If there is not a reference pattern let's take the current pattern as the current
      if (curr == NULL || ref_patt == curr)
      {
        Pattern::set_curr_pattern(ref_patt, AS_SINGLE);
        return (INTRA);
      }
      else
        return (INTER_AND);
    }
    else if (exp->op == CAST_INT || exp->op == CAST_CHA || exp->op == CAST_OBJ || exp->op == CAST_BOO)
    {
      node_type = find_node_type(exp->data.fun_exp.args->arg_exp);
      return (node_type);
    }
    else if (exp->op == SUMS || exp->op == PRDS ||
             exp->op == MINS || exp->op == MAXS ||
             exp->op == COUNT || exp->op == CONCS ||
             exp->op == PUSHT)
    {
      // The reference goes in the first argument

      int ref = (int)exp->data.fun_exp.args->arg_exp->data.val_exp.val.num;

      // Second argument for CONC (the separator)
      if (exp->op == CONCS && find_node_type(exp->data.fun_exp.args->sig->arg_exp) == INTER_AND)
        return INTER_AND;

      if (exp->op == COUNT || exp->op == PUSHT)
        ref_patt = Pattern::get_pattern(ref);
      else
        ref_patt = patt_of_ref(ref);

      // If there is not a reference pattern let's take the current pattern as the current
      if (curr == NULL || ref_patt == curr)
      {
        Pattern::set_curr_pattern(ref_patt, AS_SINGLE);
        return (INTRA);
      }
      else
        return (INTER_AND);
    }
    else
    {
      comp_err("Internal Error : Unknown expr operator %d in find_node_type\n", exp->op);
      return 0;
    }
  }
}

/**
 * @brief Deppending whether the Expression is INTRA or INTER, the unique pattern referenced is set or
 *        a pattern association is created to join all of them
 * 
 * @param exp 
 * @param expr_type The type INTRA or INTER
 * @return PRIVATE 
 */
PRIVATE
void join_required_objects(Expression *exp, int expr_type)
{
  Argument *arg;
  Pattern *patt;
  int ref;

  switch (exp->op)
  {

  case PUSH:
    if (exp->data.val_exp.type == TYPE_REF) // The value is a reference
    {
      patt = patt_of_ref(exp->data.val_exp.val.num);

      if (expr_type == INTRA)
        Pattern::set_curr_pattern(patt, AS_SINGLE);
      else
        patt->add_to_assoc(AS_SINGLE);
    }
    break;

  case PUSHT:
    ref = (int)exp->data.fun_exp.args->arg_exp->data.val_exp.val.num;

    patt = Pattern::get_pattern(ref);

    if (expr_type == INTRA)
      Pattern::set_curr_pattern(patt, AS_SINGLE);
    else
      patt->add_to_assoc(AS_SINGLE);
    break;

  case CAST_BOO:
  case CAST_CHA:
  case CAST_OBJ:
  case CAST_INT:
    join_required_objects(exp->data.fun_exp.args->arg_exp, expr_type);
    break;

  case SUMS:
  case PRDS:
  case MINS:
  case MAXS:
  case CONCS:
    ref = (int)exp->data.fun_exp.args->arg_exp->data.val_exp.val.num;

    if (exp->op == CONCS)
      join_required_objects(exp->data.fun_exp.args->sig->arg_exp, expr_type);

    patt = patt_of_ref(ref);

    if (expr_type == INTRA)
      Pattern::set_curr_pattern(patt, AS_SET);
    else
      patt->add_to_assoc(AS_SET);
    break;

  case COUNT:
    ref = (int)exp->data.fun_exp.args->arg_exp->data.val_exp.val.num;

    patt = Pattern::get_pattern(ref);

    if (expr_type == INTRA)
      Pattern::set_curr_pattern(patt, AS_SET);
    else
      patt->add_to_assoc(AS_SET);
    break;

  case FCALL:
    arg = exp->data.fun_exp.args;
    while (arg != (Argument *)NULL)
    {
      join_required_objects(arg->arg_exp, expr_type);
      arg = arg->sig;
    }
    break;

  case MINUS:
  case TTRUE:
  case TFALSE:
  case EVAL:
  case NOT:
    join_required_objects(exp->data.rel_exp.rexp, expr_type);
    break;

  default: /* Binary relations */
    join_required_objects(exp->data.rel_exp.lexp, expr_type);
    join_required_objects(exp->data.rel_exp.rexp, expr_type);
    break;
  }
}

/**
 * @brief Compile an expression generating the equivalent code
 * 
 * @param exp The expression to compile
 * @param node_type INTRA or INTER_AND
 * @return the type of the expression 
 */
PRIVATE
int compile_expresion(Expression *exp, int node_type)
{
  int prg_ini, prg_data;
  Expression *exp_or;
  Argument *arg, *next_arg;
  int n, type;
  ULong data, op;
  int mem_ref, attr_ref, pos_ref;
  long ref;
  Pattern *patt_ref;

  switch (exp->op)
  {
  case TAND:
    (void)compile_expresion(exp->data.rel_exp.lexp, node_type);
    (void)compile_expresion(exp->data.rel_exp.rexp, node_type);
    delete exp;
    type = TYPE_BOOL;
    break;

  case TOR:
    /* Space for the TOR and the jumps */
    add_code(2, TOR, 0L);
    prg_data = curr_lcode - 1;
    for (n = 1, exp_or = exp;
         exp_or->op == TOR;
         exp_or = exp_or->data.rel_exp.rexp, n++)
    {
      // Space for the jumps
      add_code(1, 0);
    }

    // Space for the final jump
    add_code(1, 0);
    prg_ini = curr_lcode;

    curr_code[prg_data++] = n;
    do
    {
      exp_or = exp;
      (void)compile_expresion(exp->data.rel_exp.lexp,
                              node_type);
      exp = exp->data.rel_exp.rexp;
      delete exp_or;

      curr_code[prg_data++] = curr_lcode - prg_ini;
    } while (exp->op == TOR);

    (void)compile_expresion(exp, node_type);
    curr_code[prg_data++] = curr_lcode - prg_ini;
    type = TYPE_BOOL;
    break;

  case TEQ:
  case TNE:
  case TLT:
  case TGT:
  case TLE:
  case TGE:
    type = compile_expresion(exp->data.rel_exp.lexp, node_type);
    (void)compile_expresion(exp->data.rel_exp.rexp, node_type);
    if (type == TYPE_CHAR || type == TYPE_BOOL || type == TYPE_PATTERN)
      type = TYPE_NUM;
    add_code(1, exp->op | type);
    delete exp;
    type = TYPE_BOOL;
    break;

  case TTRUE:
  case TFALSE:
  case NOT:
    (void)compile_expresion(exp->data.rel_exp.rexp, node_type);
    add_code(1, exp->op);
    type = TYPE_BOOL;
    delete exp;
    break;
  case EVAL:
    add_code(2, exp->op, 0L);
    prg_ini = curr_lcode;
    (void)compile_expresion(exp->data.rel_exp.rexp, node_type);
    curr_code[prg_ini - 1] = curr_lcode - prg_ini;
    type = TYPE_BOOL;
    delete exp;
    break;

  case ADD:
  case SUB:
  case MUL:
  case DIV:
    type = compile_expresion(exp->data.rel_exp.lexp, node_type);
    (void)compile_expresion(exp->data.rel_exp.rexp, node_type);
    add_code(1, exp->op | type);
    delete exp;
    break;

  case MINUS:
    type = compile_expresion(exp->data.rel_exp.rexp, node_type);
    add_code(1, exp->op | type);
    delete exp;
    break;

  case FCALL:
    n = 0;
    for (arg = exp->data.fun_exp.args; arg != (Argument *)NULL; arg = next_arg)
    {
      next_arg = arg->sig;
      (void)compile_expresion(arg->arg_exp, node_type);
      delete arg;
      n++;
    }

    type = func_type(exp->data.fun_exp.name);
    data = (ULong)exp->data.fun_exp.name;
    add_code(4, FCALL, 0L, data, (ULong)n);
    delete exp;
    break;

  case COUNT:
    ref = exp->data.fun_exp.args->arg_exp->data.val_exp.val.num;
    patt_ref = Pattern::get_pattern((int)ref);
    patt_ref->get_inter_pos(&mem_ref, &pos_ref);
    add_code(2, COUNT, (mem_ref << 15) | (pos_ref << 8));
    delete exp->data.fun_exp.args->arg_exp;
    delete exp->data.fun_exp.args;
    delete exp;
    type = TYPE_NUM;
    break;

  case SUMS:
  case PRDS:
  case MINS:
  case MAXS:
    ref = exp->data.fun_exp.args->arg_exp->data.val_exp.val.num;
    patt_ref = patt_of_ref(ref);
    type = type_of_ref(ref);
    patt_ref->get_inter_pos(&mem_ref, &pos_ref);
    op = exp->op | type;
    attr_ref = attr_of_ref(ref);
    add_code(2, op, (mem_ref << 15) | (pos_ref << 8) | attr_ref);
    delete exp->data.fun_exp.args->arg_exp;
    delete exp->data.fun_exp.args;
    delete exp;
    break;

  case CONCS:
    ref = exp->data.fun_exp.args->arg_exp->data.val_exp.val.num;
    patt_ref = patt_of_ref(ref);

    // Compile of the second argument that mus be of TYPE_STR type
    (void)compile_expresion(exp->data.fun_exp.args->sig->arg_exp, node_type);

    patt_ref->get_inter_pos(&mem_ref, &pos_ref);
    attr_ref = attr_of_ref(ref);
    add_code(2, CONCS, (mem_ref << 15) | (pos_ref << 8) | attr_ref);

    delete exp->data.fun_exp.args->sig;
    delete exp->data.fun_exp.args->arg_exp;
    delete exp->data.fun_exp.args;
    delete exp;
    type = TYPE_STR;
    break;

  case CAST_BOO:
  case CAST_CHA:
  case CAST_OBJ:
  case CAST_INT:
    (void)compile_expresion(exp->data.fun_exp.args->arg_exp, node_type);
    switch (exp->op)
    {
    case CAST_INT:
      type = TYPE_NUM;
      break;
    case CAST_CHA:
      type = TYPE_CHAR;
      break;
    case CAST_OBJ:
      type = TYPE_PATTERN;
      break;
    case CAST_BOO:
      type = TYPE_BOOL;
      break;
    }
    delete exp->data.fun_exp.args;
    delete exp;
    break;

  case PUSHT:
    ref = exp->data.fun_exp.args->arg_exp->data.val_exp.val.num;
    patt_ref = Pattern::get_pattern((int)ref);
    patt_ref->get_inter_pos(&mem_ref, &pos_ref);
    add_code(2, PUSHT, (mem_ref << 15) | (pos_ref << 8));
    delete exp->data.fun_exp.args->arg_exp;
    delete exp->data.fun_exp.args;
    delete exp;
    type = TYPE_NUM;
    break;

  case PUSH:
    type = exp->data.val_exp.type;
    switch (type)
    {

    case TYPE_CHAR:
    case TYPE_BOOL:
    case TYPE_NUM:
      op = (exp->op | TYPE_NUM);
      data = (ULong)exp->data.val_exp.val.num;
      add_code(2, op, data);
      break;
    case TYPE_STR:
      op = (exp->op | type);
      data = (ULong)exp->data.val_exp.val.str;
      add_code(2, op, data);
      break;
    case TYPE_FLO:
      op = (exp->op | type);
      data = *(ULong *)(&(exp->data.val_exp.val.flo));
      add_code(2, op, data);
      break;
    case TYPE_REF:
      ref = exp->data.val_exp.val.num;
      attr_ref = attr_of_ref(ref);
      patt_ref = patt_of_ref(ref);

      if (node_type == INTRA)
      {
        mem_ref = LEFT_MEM;
        pos_ref = 0;
      }
      else
      {
        patt_ref->get_inter_pos(&mem_ref, &pos_ref);
      }

      type = type_of_ref(ref);
      if (type == TYPE_PATTERN)
      {
        if (patt_ref->is_negated())
          comp_err("negated pattern not valid here\n");
        if (attr_ref == 0)
          op = PUSHO;
        else
          op = (PUSHS | TYPE_NUM);
      }
      else if (type == TYPE_CHAR || type == TYPE_BOOL)
        op = (PUSHS | TYPE_NUM);
      else
        op = (PUSHS | type);

      add_code(2, op, (mem_ref << 15) | (pos_ref << 8) | (attr_ref));
      break;
    default:
      fprintf(trace_file, "Unknown node type in expresion\n");
    }

    delete exp;
    break;

  default:
    fprintf(trace_file, "Unknown node type in expresion\n");
  }
  return (type);
}

//
// RIGHT HAND SIDE OF THE RULE
//

/**
 * @brief Store an Expression in the current pattern, current attribute
 * 
 * @param exp the Expression
 */
PUBLIC
void store_exp(Expression *exp)
{
  int curr_attr, curr_type, mem, pos;

  Pattern::curr_pattern()->get_inter_pos(&mem, &pos);
  ObjClass::curr_attr_type(&curr_attr, &curr_type);

  // No new associations are made. Current is maintained and all the code goes to the same node
  
  if (check_type(&exp, TRUE) != curr_type)
    comp_err("Incorrect expression type\n");

  (void)pre_evaluate_exp(&exp);

  (void)compile_expresion(exp, INTER_AND);

  if (curr_type == TYPE_CHAR || curr_type == TYPE_BOOL || curr_type == TYPE_PATTERN)
    curr_type = TYPE_NUM;

  add_code(2, POPS | curr_type, (mem << 15) | (pos << 8) | curr_attr);
}

/**
 * @brief Call a procedure depending on the TAG propagated (ON INSERT, ON MODIFY, ON DELETE)
 * 
 * @param name Name of the procedure
 * @param flags: propagated tags that allow execution
 */
PUBLIC
void init_proc(char *name, ULong flags)
{

  /* We don't know the code length nor the number of arguments yet ! */

  add_code(6, PCALL, 0L, name, (ULong)0, flags, (ULong)0);
  set_curr_rule_exec_flags(flags);
}

/**
 * @brief Store all the parameters that the procedure call needs
 * 
 * @param name The name of the procedure
 * @param args The Expressions that are the parameters of the procedure 
 */
PUBLIC
void store_proc(char *name, Argument *args)
{

  Argument *arg_i, *next_arg_i;
  int n;
  int nargsf = num_of_args(name);
  int fhvar_num_of_args = func_has_var_num_of_args(name);
  n = 0;
  for (arg_i = args; arg_i != (Argument *)NULL; arg_i = next_arg_i)
  {
    if (n < nargsf)
    {
      if (check_type(&(arg_i->arg_exp), TRUE) != arg_type(name, n))
        comp_err("Mismatch argument type\n");
    }
    else
      check_type(&(arg_i->arg_exp), TRUE); // check_type must be executed
                                           // on all arguments !!
    n++;
    (void)pre_evaluate_exp(&(arg_i->arg_exp));
    compile_expresion(arg_i->arg_exp, INTER_AND);
    next_arg_i = arg_i->sig;
    delete arg_i;
  }

  if (func_type(name) != TYPE_VOID)
    comp_err("Function %s is not a Procedure\n", name);

  if (n < nargsf || (n > nargsf && !fhvar_num_of_args))
    comp_err("Incorrect number of arguments passed to procedure %s\n", name);

  /* set code length and number of arguments */
  // Num argumentos
  curr_code[3] = n;
  // Longitud del codigo
  curr_code[5] = curr_lcode - 6;

  Pattern::add_code_to_curr_node(curr_lcode, curr_code);
  free_code();
}

/**
 * @brief The rule imply the creation of a new object, depending of the tag propagated
 *      This function only fill the created object with its classname. 
 *      Further attributes fulfilling will be done in end_of_RH_order()
 * 
 * @param class_name Class name of the objet to create
 * @param flags Tags propagated allowed to trigger this action
 */
PUBLIC
void notify_for_create(char *class_name, ULong flags)
{

  Pattern *curr_patt;
  int mem, pos;

  ObjClass::set_curr_class_RHR(class_name);
  curr_patt = Pattern::curr_pattern();

  curr_patt->get_inter_pos(&mem, &pos);
  add_code(9,
           OBJNEW,
           (mem << 15) | (pos << 8),
           ObjClass::n_attrs_of_curr() - 1,
           flags,
           0L,
           PUSH | TYPE_STR,
           class_name,
           POPS | TYPE_STR,
           (mem << 15) | (pos << 8));

  set_curr_rule_exec_flags(flags);
}

/**
 * @brief The rule imply the modification/change of an object, depending of the tag propagated
 *      This function only initialice the action with the pattern number 
 *      Further attributes fulfilling will be done in end_of_RH_order()
 * 
 * @param class_name Class name of the objet to be changed or modified
 * @param flags Tags propagated allowed to trigger this action
 */
PUBLIC
void notify_for_modify(int modify_type, int pattern_num, ULong flags)
{
  int mem, pos;
  StValues life_st;
  Pattern *curr_patt;

  Pattern::set_pattern(pattern_num);
  curr_patt = Pattern::curr_pattern();

  life_st = curr_patt->life_st();

  // The MODIFY event only is trigger if there is nor a previous CREATION for the 
  // same object pendding, due the CREATION notification will comunicate the final state
  // of the object including the modification
  // If there would be a previous DELETE of the object the modifications is aborted due 
  // the object will be already deleted when communicating the Modification 
  
  switch (life_st)
  {
  case ST_NORMAL:
    if (curr_patt->is_set())
      comp_err("Cannot be modified a set\n");

    curr_patt->get_inter_pos(&mem, &pos);
    if (modify_type == 0)
      add_code(5, OBJMOD, (mem << 15) | (pos << 8), ObjClass::n_attrs_of_curr() - 1, flags, 0L);
    else
      add_code(5, OBJCHG, (mem << 15) | (pos << 8), ObjClass::n_attrs_of_curr() - 1, flags, 0L);
    set_curr_rule_exec_flags(flags);
    break;
  case ST_DELETED:
    comp_err("Cannot be modified a deleted object\n");
    break;
  case ST_NEGATED:
  case ST_OPTIONAL:
    comp_err("Cannot be modified a negated/optional object\n");
    break;
  }
}

/**
 * @brief The rule imply the delete of an object, depending of the tag propagated
 * 
 * @param class_name Class name of the objet to be changed or modified
 * @param flags Tags propagated allowed to trigger this action
 */
PUBLIC
void notify_for_delete(int pattern_num, ULong flags)
{
  int mem, pos;
  StValues life_st;
  Pattern *curr_patt;

  Pattern::set_pattern(pattern_num);
  curr_patt = Pattern::curr_pattern();

  life_st = curr_patt->life_st();

  switch (life_st)
  {
  case ST_NORMAL:
    if (curr_patt->is_set())
      comp_err("Cannot be deleted a set\n");

    curr_patt->get_inter_pos(&mem, &pos);
    add_code(3, OBJDEL, (mem << 15) | (pos << 8), flags);

    // Delete does not call to end_of_RH_order(), let's free the code here
    Pattern::add_code_to_curr_node(curr_lcode, curr_code);
    free_code();

    curr_patt->set_life_st(ST_DELETED);
    set_curr_rule_exec_flags(flags);
    break;
  case ST_DELETED:
    comp_err("Cannot be deleted an already deleted object\n");
    break;
  case ST_NEGATED:
  case ST_OPTIONAL:
    comp_err("Cannot be deleted a negated/optional object\n");
    break;
  }
}

/**
 * @brief The rule imply an object
 *      This means that the tag is maintained in the object implied (e.g. TAG INSERT -> The Object is INSERTED(created))
 *      This function only initialice the object with the class name
 *      Further attributes fulfilling will be done in end_of_RH_order()
 * 
 * @param class_name Class name of the objet to be changed or modified
 */
PUBLIC
void notify_for_obj_imply(char *class_name)
{

  Pattern *curr_patt;
  int mem, pos;

  ObjClass::set_curr_class_RHR(class_name);
  curr_patt = Pattern::curr_pattern();

  curr_patt->get_inter_pos(&mem, &pos);
  add_code(9,
           OBJIMP,
           (mem << 15) | (pos << 8),
           ObjClass::n_attrs_of_curr() - 1,
           0L, // To unify lengths with con NEW/MODIFY (flags)
           0L,
           PUSH | TYPE_STR,
           class_name,
           POPS | TYPE_STR,
           (mem << 15) | (pos << 8));
  
  if (curr_rule_exec_flags() & EXEC_TRIGGER)
    comp_err("Rule can't accept implied objects due to use of triggers\n");
  obj_imply_at(pos);

  set_curr_rule_exec_flags(EXEC_ALL);
}

/**
 * @brief This function fix the length of the additional code assocciated to creation, modification or obj implication
 *        on the RHS of the rule. Deletion does not call to this function
 *        Fix the length, add the code to production node and free it
 * 
 */
PUBLIC
void end_of_RH_order()
{
  // Fix of code length
  curr_code[4] = curr_lcode - 5;
  Pattern::add_code_to_curr_node(curr_lcode, curr_code);
  free_code();
}

/**
 * @brief This function free the temporal code repository while it is being generated
 * 
 */
PRIVATE void
free_code()
{
  if (curr_code != NULL)
    free(curr_code);

  curr_code = NULL;
  curr_lcode = 0;
}

/**
 * @brief This function add code to the temporal code repository while it is being generated 
 * 
 * @param code_len Len of code
 * @param ... Code words (as many as code_len)
 */
PRIVATE void
add_code(int code_len, ...)
{
  va_list list;
  int n;

  va_start(list, code_len);

  if (code_len > 0)
  {
    curr_code = (ULong *)realloc((void *)(curr_code),
                                 (curr_lcode + code_len) * sizeof(ULong));

    for (n = 0; n < code_len; n++)
      curr_code[curr_lcode++] = va_arg(list, ULong);
  }

  va_end(list);
}
