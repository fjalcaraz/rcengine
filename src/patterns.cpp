/**
 * @file patterns.cpp
 * @author Francisco Alcaraz
 * @brief Pattern management. A pattern is each of the clauses in the LHS of a rule
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdlib.h>
#include <string.h>

#include "codes.h"
#include "classes.hpp"
#include "rules.hpp"
#include "vars.hpp"
#include "expr.hpp"
#include "patterns.hpp"



PRIVATE Node *curr_node; 

PRIVATE Pattern*patt[256];
PRIVATE int n_patt = 0;
PRIVATE Pattern *curr_patt = NULL;
PRIVATE int check_eq_obj = TRUE;
PRIVATE int beg_set_ctx = -1;
PRIVATE ULong curr_flags = 0;

/**
 * @brief Construct a new Pattern:: Pattern object
 * 
 * @param class_p Class of the object
 * @param status Status of the object ST_NORMAL, ST_NEGATED, ST_OPTIONAL, ST_DELETED
 * @param root Root of the net
 * @param create_path 
 */
Pattern::Pattern(ObjClass *class_p, StValues status, Node *root, int create_path)
{
  _class = class_p;
  _root = root;
  _access = AS_SINGLE;
  _life_st = status;
  _last_intra_set = NULL;
  _flags = setupFlags();
  _is_set = FALSE;

  if (create_path)
    _last_intra = _root->new_path();
  else
    _last_intra = _root;

  curr_node = _last_intra;
  curr_patt = this;

  if (check_eq_obj)
  {
    int n;

    // In case an object matching this pattern can match also some other in the rule
    // lets put a Test-Not-Same-Object (TNSOBJ) code

    // TODO: it has to be simplified
    for (n=0; n<n_patt; n++)
    {
        if (patt[n]->_life_st == ST_NORMAL && patt[n]->_is_set &&
            ObjClass::could_be_equal_obj(patt[n]->_class, _class))
        {
           int pos1, mem1, pos2, mem2;
           ULong code[2];

           new_inter_assoc(curr_rule_tw());
           patt[n]->add_to_assoc(AS_SINGLE);  // Left
           this->add_to_assoc(AS_SINGLE);     // right
  
           patt[n]->get_inter_pos(&mem1, &pos1);
           this->get_inter_pos(&mem2, &pos2);

           code[0]= TNSOBJ;
           code[1]= ((mem1<<31) | (pos1<<24)| (mem2<<15) | (pos2<<8));
           
           add_code_to_curr_node(2,code);
        }
    }

    for (n=0; n<n_patt; n++)
    {
        if ((_life_st == ST_NORMAL || patt[n]->_life_st == ST_NORMAL) && !(patt[n]->_is_set) &&
            ObjClass::could_be_equal_obj(patt[n]->_class, _class))
        {
           int pos1, mem1, pos2, mem2;
           ULong code[2];
 
           new_inter_assoc(curr_rule_tw());
           patt[n]->add_to_assoc(AS_SINGLE);  // Left
           this->add_to_assoc(AS_SINGLE);     // right
 
           patt[n]->get_inter_pos(&mem1, &pos1);
           this->get_inter_pos(&mem2, &pos2);
 
           code[0]= TNSOBJ;
           code[1]= ((mem1<<31) | (pos1<<24)| (mem2<<15) | (pos2<<8));
 
           add_code_to_curr_node(2,code);
        }
    }

  }

  patt[n_patt++] = this;

}

/**
 * @brief Construct a new Pattern:: Pattern object
 *    This constructor is used for the objects in the RHS of rule (CREATE / OBJIMPL)
 * 
 * @param class_p Object Class created or implied
 */
Pattern::Pattern(ObjClass *class_p)
{
   _class = class_p;
   _root = NULL;
   _access = AS_SINGLE;
   _last_intra= NULL;
   _last_intra_set = NULL;
   _life_st = ST_NORMAL;
   _flags = 0;
   _is_set = FALSE;

    patt[n_patt++] = this;
    curr_patt = this;
   // Curr_node does not change;
}


Pattern::~Pattern()
{
}

/**
 * @brief Initialize the temporal flags with the information taken from the class
 *    This method is called form the constructor. 
 *    Before that, a static call to initFlags(flags, TRUE) must be done to take the flags form the rule
 * 
 * @return ULong The final flags
 */
ULong 
Pattern::setupFlags()
{
   
   ULong cl_flags;
 
   cl_flags = _class->temp_flags();
   initFlags(cl_flags, FALSE);
 
   return curr_flags;
}

/**
 * @brief Static method to set the timming flags even before creating the pattern
 *    This is done this way due the syntax detects the timming flags of the rule before defining the pattern indeed
 * 
 * @param value Flags values
 * @param from_rule the specific flag from rule (TRUE) or take the default from the class (FALSE)
 */
/* static */ void
Pattern::initFlags(ULong value, int from_rule)
{
   if (from_rule)
      curr_flags = value;

   else
   {
      // Mix rule current flags with the default flags taken form the class
      if ((curr_flags & STORE_MASK) == 0)
         curr_flags = ((curr_flags & TIMED_MASK) | (value & STORE_MASK));
      if ((curr_flags & TIMED_MASK) == 0)
         curr_flags = ((value & TIMED_MASK) | (curr_flags & STORE_MASK));

      // Default values
      if ((curr_flags & STORE_MASK) == 0)
         curr_flags |= IS_PERMANENT;
      if ((curr_flags & TIMED_MASK) == 0)
         curr_flags |= IS_TIMED;

      if ((curr_flags & IS_TIMED) && (curr_flags & IS_UNTIMED))
         comp_err("Timed and Untimed behaviors are incompatible\n");

      if ((((curr_flags & IS_PERMANENT) != 0) +
           ((curr_flags & IS_TEMPORAL) != 0) +
           ((curr_flags & IS_TRIGGER) != 0)) != 1)
         comp_err("Storage controls incompatible\n");
   
      if (curr_flags & IS_TRIGGER)
         set_curr_rule_exec_flags(EXEC_TRIGGER);
   }
}

/**
 * @brief Static method to get the current pattern
 * 
 * @return Pattern* 
 */
Pattern *
Pattern::curr_pattern()
{
  return curr_patt;
}

/**
 * @brief Static method to reset the access (AS_SINGLE / AS_SET) for all patterns
 * 
 */
void
Pattern::reset_access()
{
  int n;
 
  for (n=0; n<n_patt; n++)
    patt[n]->_access = NO_ACCESS;
 
  curr_patt = NULL;
}

/**
 * @brief Set the curret pattern and its access mode
 * 
 * @param patt Pattern
 * @param access access mode (AS_SINGLE / AS_SET)
 */
void
Pattern::set_curr_pattern(Pattern *patt, AccessMode access)
{
  curr_patt = patt;

  // The access AS_SET has prevalence over AS_SINGLE
 
  if (patt->_access != AS_SET)
    patt->_access = access;
}

/**
 * @brief Set the current pattern by pattern number
 * 
 * @param patt_no The pattern number
 */
void
Pattern::set_pattern(int patt_no)
{
  if (patt_no<0 || patt_no >= n_patt)
    comp_err("No such pattern number. Valid are [1-%d]\n", n_patt);
  curr_patt = patt[patt_no];
  curr_patt->_class->set_curr_class();
}

/**
 * @brief Get the pattern by pattern number
 * 
 * @param patt_no Pattern number
 * @return Pattern* The pattern selected
 */
Pattern *
Pattern::get_pattern(int patt_no)
{
  if (patt_no<0 || patt_no >= n_patt)
    comp_err("No such pattern number. Valid are [1-%d]\n", n_patt);
  return patt[patt_no];
}
  
/**
 * @brief Add a class-check node (INTRA node with TCLASS code in it) according to the class of this pattern
 *      and get the resulting last_intra node
 * 
 * @param name Name of the class
 */
void
Pattern::add_class_check(char *name)
{
   _root -> insert_class_check(&_last_intra, name, TRUE);
}

/**
 * @brief Add a new INTRA node with code to this pattern
 * 
 * @param code_len Length of code
 * @param code Array of code instructions
 */
void
Pattern::add_intra_node(int code_len, ULong *code)
{
  if (_access == AS_SINGLE || _access == NO_ACCESS)
    curr_node = _root -> insert_intra_node(&_last_intra, 
                        ObjClass::get_real_root(), code_len, code);
  else
    curr_node = _root -> insert_intra_node(&_last_intra_set, 
                        ObjClass::get_real_root(), code_len, code);
}

/**
 * @brief Static method that starts a new INTER association among patterns
 * 
 * @param time 
 */
void
Pattern::new_inter_assoc(int time)
{
   curr_node = NULL;
   Node::new_inter_assoc(time);
}

/**
 * @brief Add this pattern to the INTER association with the access set in the pattern
 * 
 */
void
Pattern::add_to_assoc()
{
   if (_is_set)
     _access = AS_SET;
   else
     _access = AS_SINGLE;
 
   switch(_access)
   {
      case AS_SINGLE:
         curr_node = _last_intra -> add_to_inter_assoc(_last_intra_set, _life_st , _flags);
         break;
      case AS_SET :
         curr_node = _last_intra_set -> add_to_inter_assoc(_last_intra_set, _life_st, _flags);
         break;
   }
}

/**
 * @brief Add this pattern to the INTER association with a custom access (AS_SINGLE/AS_SET)
 * 
* @param access custom access mode
 */
void
Pattern::add_to_assoc(AccessMode access)
{
   _access = access;

   switch(_access)
   {
      case AS_SINGLE:
         curr_node = _last_intra -> add_to_inter_assoc(_last_intra_set, _life_st , _flags);
         break;
      case AS_SET :
         curr_node = _last_intra_set -> add_to_inter_assoc(_last_intra_set, _life_st, _flags);
         break;
   }
}

/**
 * @brief Add code to current node
 *      current node is the final resulting node after all the associations made
 * 
 * @param code_len Length of code
 * @param code Array od code
 */
void
Pattern::add_code_to_curr_node(int code_len, ULong *code)
{
  curr_node->add_code(code_len, code);
}

/**
 * @brief Insert code to current node at a certail position
 *      current node is the final resulting node after all the associations made
 * 
 * @param code_len Length of code
 * @param code Array od code
 */
void
Pattern::insert_code_in_curr_node(int pos, int code_len)
{
  curr_node->insert_code(pos, code_len);
}

/**
 * @brief Set (replace) code to current node at a certail position
 *      current node is the final resulting node after all the associations made
 * 
 * @param pos position
 * @param code Value
 */
void
Pattern::set_code_in_curr_node(int pos, ULong code)
{
  curr_node->set_code(pos, code);
}

/**
 * @brief get the code of current node at a certain position
 *      current node is the final resulting node after all the associations made
 * 
 * @param pos Position
 * @return The code at that position
 */
ULong
Pattern::get_code_in_curr_node(int pos)
{
  return curr_node->get_code(pos);
}

/**
 * @brief get the length of the code of current node
 *      current node is the final resulting node after all the associations made
 * 
 * @return The length of code
 */
int
Pattern::get_len_of_curr_node()
{
  return curr_node->len();
}

/**
 * @brief Get the memory side and position of the object related to this pattern in the current node
 * 
 * @param mem Where to return the mem side
 * @param pos Where to return the position
 */
void
Pattern::get_inter_pos(int *mem, int *pos)
{
  int n,m;

  if (_last_intra != NULL)
  {
    _last_intra->set_obj_pos(0);
    curr_node->get_obj_pos(mem, pos);
  }
  else
  {
    // The objects created in the RHS of rules don't have last_intra
    // Its position is equal to its position in the patterns table
    for (n=0; n<n_patt && patt[n]->_last_intra != NULL; n++);
    for (m=0; m<n_patt && patt[n+m] != this; m++);
    *mem = RIGHT_MEM;
    *pos = m;
  }
}
  
/**
 * @brief Set a new set context
 *      A set context means an interval of patterns related to one of them that is going to be a SET
 *      This method indicates the beggining of this association, the first pattern
 * 
 */
void
Pattern::new_set_context()
{
  beg_set_ctx = n_patt;
}

/**
 * @brief this pattern is going to be a a SET, so all the related patterns are going 
 *      to be joined in an association and the created a new INTRA_SET node
 * 
 */
void
Pattern::add_set_node()
{
  int n;
  Node *common_node;
  int pos;

  new_inter_assoc(curr_rule_tw());
  for (n=beg_set_ctx; n<n_patt; n++)
  {
      patt[n]->add_to_assoc(AS_SINGLE);
  }


  // The SET nodes must be moved below OAND nodes. Last_inter_node does nor go through the OAND

  if (patt[beg_set_ctx]->_life_st == ST_NORMAL)
    common_node = patt[beg_set_ctx]->_last_intra->last_inter_node(FALSE);
  else
    common_node = patt[beg_set_ctx]->_last_intra->last_inter_node_ussing();

  patt[beg_set_ctx]->_last_intra->set_obj_pos(0);
  common_node->get_obj_pos_after(&pos);
  curr_node = Node::create_SET_node(pos, n_patt - beg_set_ctx);
  curr_node -> connect_node(common_node, LEFT_MEM);

  for (n=beg_set_ctx; n<n_patt; n++)
  {
    // Set the last_intra_set of all related patterns to this new curr_node (SET)
    patt[n]->_is_set = TRUE;
    patt[n]->_last_intra_set = curr_node;
  }

  curr_node->move_down_set_nodes();
}

/**
 * @brief All the items in the set are internally classified by the same value of a given attribute
 *    This way makes extremely fast the matching set given an attribute value 
 * 
 * @param attr Attribute number
 */
void
Pattern::assume_equality_in_set(int attr)
{

  int first_pos, pos, side, type;
  ULong code[5];

  // Calculation of first position of the set 

  patt[beg_set_ctx]->_last_intra->set_obj_pos(0);
  curr_node -> get_obj_pos(&side, &first_pos);

  // Calculation of the position of this pattern in the set
  _last_intra->set_obj_pos(0);
  curr_node -> get_obj_pos(&side, &pos);

  pos -= first_pos; // Relative position inside the SET
  type = class_of_patt()-> attr_type(attr);
  type = effective_type(type);

  code[0] = PUSHS|type;
  code[1] = ((LEFT_MEM<<15) | (pos<<8) | attr);
  code[2] = PUSHS|type;
  code[3] = ((RIGHT_MEM<<15) | (pos<<8) | attr);
  code[4] = CMP|type;

  curr_node->add_code(5, code, TRUE);
}

/**
 * @brief End of set context
 * 
 */
void
Pattern::end_set_context()
{
  beg_set_ctx = -1;
}

/**
 * @brief Add a production node
 * 
 * @return Node* The prod node created
 */
Node *
Pattern::add_prod_node()
{

  int n_affirm;
  int patt_aff, patt_set, patt_aff_window, patt_set_window;
  int n;

  // We have to assure all the pattern context are joined
  // before reaching the production node

  // Let's join all the affirmative contexts first
  
  n_affirm = 0;
  patt_aff = -1;
  patt_set = -1;
  patt_aff_window = -1;
  patt_set_window = -1;
  new_inter_assoc(curr_rule_tw());
  for (n=0; n<n_patt; n++)
  {
    if (patt[n]->_life_st == ST_NORMAL)
    {
      if (!patt[n]->_is_set)
      {
        patt_aff = n;
        if (patt[n]->_flags & IS_TIMED) patt_aff_window = n;
        n_affirm++;
      }
      else
      {
        patt_set = n;
        if (patt[n]->_flags & IS_TIMED) patt_set_window = n;
        n_affirm++;
      }
      patt[n]->add_to_assoc();
    }
  }

  // We give preference to those affirmative not SET

  if (patt_aff<0 && patt_set>=0)
    patt_aff = patt_set;

  if (patt_aff_window<0 && patt_set_window>=0)
    patt_aff_window = patt_set_window;

  if (patt_aff_window>=0)
    patt_aff = patt_aff_window;

  if (n_affirm == 0)
    comp_err("At least one pattern should be positive\n");

  if (curr_rule_tw() != NO_TIMED && patt_aff_window>=0)
  {
     for (n=0; n<n_patt; n++)
     {
        if (patt[n]->_flags & IS_TIMED)
        {
           new_inter_assoc(curr_rule_tw());
           patt[patt_aff_window]->add_to_assoc(AS_SINGLE);
           patt[n]->add_to_assoc(AS_SINGLE);
        }
     }

  }

  for (n=0; n<n_patt; n++)
  {
    if (patt[n]->_life_st != ST_NORMAL)
    {
      // Let associate now the negated or optional one by one to an affirmative pattern
      // It's better to join one by one to create simple NAND / OAND nodes because 
      // otherwise they can create AND node among them and it is not neccesary neither usefull.
      // If that patterns would be related among them in the rule these AND node will be already created
      
      // If the neg/opt pattern are already related with some affirmative pattern it is OK

      int correct_type = (patt[n]->_life_st == ST_NEGATED) ? INTER_NAND : INTER_OAND;

      if (patt[n]->_last_intra->last_inter_node_ussing()->type() != correct_type)
      {
        new_inter_assoc(curr_rule_tw());
        patt[patt_aff]->add_to_assoc();
        patt[n]->add_to_assoc();
      }
      

    }
  }

  // Lets create TIME nodes to those solitaire SET nodes
 
  if (curr_rule_tw() != NO_TIMED)
  {
     for (n=0; n<n_patt; n++)
     {
        if ((patt[n]->_flags & IS_TIMED) && (patt[n]->_flags & IS_TEMPORAL))
        {
          patt[n]->_root->create_TIMER_node(&patt[n]->_last_intra, ObjClass::get_real_root(), curr_rule_tw());
        }
     }
  }

  new_inter_assoc(curr_rule_tw());
  if (!patt[patt_aff]->_is_set)
    curr_node = patt[patt_aff]->_last_intra->last_inter_node(FALSE);
  else
    curr_node = patt[patt_aff]->_last_intra_set->last_inter_node(FALSE);

  curr_node = curr_node->insert_prod_node();

  for (n=0; n<n_patt; n++)
    if (patt[n]->_is_set)
		patt[n]->_last_intra->mark_set_positions(0, patt[n]->_last_intra_set);

  return curr_node;

}
         
/**
 * @brief Extern function to set if the checking to avoid same object matching 
 *    among multiple patterns in a rule id allowed
 * 
 */
void
allow_same_obj()		
{
  check_eq_obj = FALSE;
}

/**
 * @brief Print a Pattern
 * 
 */
void
Pattern::print()
{
   fprintf(trace_file, "PATTERN FOR OBJECT CLASS ");
   _class->print();
}

/**
 * @brief Get the pattern number (associated to a var)
 * 
 * @return int Retuns the pattern number or -i if it is the current pattern
 */
int
Pattern::pattern_no()
{
   int n;

   for (n=0; n<n_patt && patt[n]!=this; n++);

   if (n == n_patt) return -1;
   else return n;
} 

/**
 * @brief Modify last intra set node for a pattern
 * 
 * @param from The initial last intra set node used to identify the pattern 
 * @param to The new last intra set node for that pattern
 */
//Static
void
Pattern::set_last_intra_set(Node *from, Node *to)
{
   int n;

   for (n=0; n<n_patt && patt[n]->_last_intra_set != from; n++);

   if (n < n_patt)
     patt[n]->_last_intra_set = to;
}
 
/**
 * @brief Deletes all the defined patterns in the rule
 * 
 */
void
Pattern::delete_patterns()
{
  int n;

  for (n=0; n<n_patt; n++)
  {
    if (patt[n]->_root != NULL)
       patt[n]->_root->clear_rule();
    delete patt[n];
  }

  n_patt = 0;
  curr_patt = NULL;
}

/**
 * @brief Free the current rule patterns and nodes related
 * 
 */
void
Pattern::free_patterns()
{
  int n;
 
  for (n=0; n<n_patt; n++)
  {
    if (patt[n]->_root != NULL)
       patt[n]->_root->clear_rule();
    patt[n]->_last_intra->free_from_last_intra();
    delete patt[n];
  }

  n_patt = 0;
  curr_patt = NULL;
}

