/**
 * @file load.cpp
 * @author Francisco Alcaraz
 * @brief This module contains the basic function to load and unload of packages
 *      Also transforms the nodes code to function calls to obtain a high performance
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <setjmp.h>

#include "actions.hpp"
#include "btree.hpp"
#include "lex.hpp"
#include "keys.hpp"
#include "patterns.hpp"
#include "load_p.hpp"

PRIVATE int on_ruleset;
PRIVATE int n_objs_retract;

//
// TimedFunctions. External calls to refresh the memories of the nodes and retract those object out of the window
//
class TimedFunc
{
  Node *node;
  ULong *codep;
  int (Node::*func)(ULong *, long, bool);

public:
  TimedFunc(Node *Nodo, ULong *Code, int (Node::*Func)(ULong *, long, bool))
  {
    node = Nodo;
    codep = Code;
    func = Func;
  };
  int eq(ULong *Codep) { return Codep == codep; };
  void call(long time) { (node->*func)(codep, time, true); };
  ULong *getcodep() { return codep; }
  Node *getnode() { return node; }
};

class TimedFuncList : public TimedFunc
{
  TimedFuncList *next;

public:
  TimedFuncList(Node *Nodo, ULong *Code, int (Node::*Func)(ULong *, long, bool), TimedFuncList *list)
      : TimedFunc(Nodo, Code, Func)
  {
    next = list;
  };

  void refresh(ULong time)
  {
    TimedFuncList *i;
    for (i = this; i != NULL; i = i->next)
    {
      i->call(time);
      // timer_key printf("REF node %d   code %d  key %d\n", i->getnode(), i->getcodep(), i->getkey());
    }
  };

  static void unsubscribe(ULong *Code, TimedFuncList *&list)
  {
    TimedFuncList *i, *prev;

    prev = NULL;
    for (i = list; i != NULL && !i->eq(Code); i = i->next)
      prev = i;

    if (i != NULL)
    {
      if (prev == NULL)
        list = i->next;
      else
        prev->next = i->next;
      delete i;
    }
  };
};

TimedFuncList *TimedFuncSubsList = NULL;

/**
 * @brief Top function to load a Package
 *
 * @param path File path
 * @return int TRUE if was compiled with no errores, FALSE otherwise
 */
PUBLIC
int load_pkg(char *path)
{
  int res;
  jmp_buf buff_jmp;
  int fd = -1;
  char *text = NULL;
  struct stat fst;

  if (getenv("TRACE_ENG") != NULL)
    set_trace_level(2);

  def_primitives();
  load_primitives();

  if (setjmp(buff_jmp) == 0)
  {
    set_return_buff(&buff_jmp);

    fd = open(path, O_RDONLY);

    if (fd < 0)
    {
      comp_err("open: %s\n", strerror(errno));
    }

    if (fstat(fd, &fst) < 0)
    {
      comp_err("fstat: %s\n", strerror(errno));
    }

    text = new char[fst.st_size + 1];
    (void) !read(fd, text, fst.st_size);
    text[fst.st_size] = '\0';

    read_pkg(text);
    res = 1;
  }
  else
  {
    free_pkg();
    res = 0;
  }

  if (fd >= 0)
    close(fd);

  if (text != NULL)
    delete[] text;

  return res;
}

/**
 * @brief Top level function to load a Rule Package from a string
 *
 * @param text of the rule package
 * @return int TRUE if was compiled with no errores, FALSE otherwise
 */
PUBLIC
int load_pkg_str(char *text)
{
  int res;
  jmp_buf buff_jmp;

  def_primitives();
  load_primitives();

  if (setjmp(buff_jmp) == 0)
  {
    set_return_buff(&buff_jmp);

    read_pkg(text);
    res = 1;
  }
  else
  {
    free_pkg();
    res = 0;
  }

  return res;
}

/**
 * @brief Top level function lo ad a Rule Set from a file
 *
 * @param path File name
 * @return int TRUE if was compiled with no errores, FALSE otherwise
 */
PUBLIC
int load_rset(char *path)
{
  int res;
  int fd = -1;
  char *text = NULL;
  struct stat fst;
  jmp_buf buff_jmp;

  if (setjmp(buff_jmp) == 0)
  {
    set_return_buff(&buff_jmp);

    fd = open(path, O_RDONLY);

    if (fd < 0)
    {
      comp_err("open: %s\n", strerror(errno));
    }

    if (fstat(fd, &fst) < 0)
    {
      comp_err("fstat: %s\n", strerror(errno));
    }

    text = new char[fst.st_size + 1];
    (void) !read(fd, text, fst.st_size);
    text[fst.st_size] = '\0';

    read_rset(text);
    res = 1;
  }
  else
  {
    free_curr_ruleset();
    res = 0;
  }

  if (fd >= 0)
    close(fd);

  if (text != NULL)
    delete[] text;

  return res;
}

/**
 * @brief Top level function lo load a Rule Set from a string
 *
 * @param text of the Rule Set
 * @return int TRUE if was compiled with no errores, FALSE otherwise
 */
PUBLIC
int load_rset_str(char *text)
{
  int res;
  jmp_buf buff_jmp;

  if (setjmp(buff_jmp) == 0)
  {
    set_return_buff(&buff_jmp);

    read_rset(text);
    res = 1;
  }
  else
  {
    free_curr_ruleset();
    res = 0;
  }

  return res;
}

/**
 * @brief Reset package freeing al the node memories
 *  
 */
PUBLIC
void reset_pkg()
{
  on_ruleset = FALSE;
  reset_package();
}

/**
 * @brief Reset a Rule Set freeing all the node memories of it
 * 
 * @param name Name of the rule set 
 */
PUBLIC
void reset_rset(char *name)
{
  on_ruleset = TRUE;
  n_objs_retract = 0;

  reset_ruleset(name);

  while (n_objs_retract > 0)
  {
    Do_loop(FALSE);
    n_objs_retract--;
  }
}

/**
 * @brief Release the full package, as if no package were loaded
 * 
 */
PUBLIC
void free_pkg()
{
  on_ruleset = FALSE;
  free_package();
  ObjClass::delete_all();
  // Pattern::delete_all();
  undef_all_func();
  undef_all_function();
}

/**
 * @brief Realease all the named ruleset as if it were not loaded
 * 
 * @param name Name of the rule set
 */
PUBLIC
void free_rset(char *name)
{
  on_ruleset = TRUE;
  n_objs_retract = 0;

  free_ruleset(name);
  while (n_objs_retract > 0)
  {
    Do_loop(FALSE);
    n_objs_retract--;
  }
}

/**
 * @brief Load code intrumenting it to allow it execution
 *    Mainly, the op codes are substituted by function calls
 * 
 * @param pcode Array odf code
 * @param len_code Length of it
 * @param node The node where the code is
 */
PUBLIC
void load_code(ULong *pcode, int len_code, Node *node)
{
  ULong *limit;
  ULong n;
  ULong *code_init;
  ULong func;

  limit = pcode + len_code;

  while (pcode < limit)
  {
    switch (*pcode)
    {
    case AND:
      code_init = pcode + LEN_AND_NODE + pcode[AND_NODE_NKEYS_POS];
      ((Node::IntFunction *)pcode)[0] = &Node::and_call;
      pcode = code_init;
      break;
    case NAND:
      code_init = pcode + LEN_AND_NODE + pcode[AND_NODE_NKEYS_POS];
      ((Node::IntFunction *)pcode)[0] = &Node::nand_call;
      pcode = code_init;
      break;
    case OAND:
      code_init = pcode + LEN_AND_NODE + pcode[AND_NODE_NKEYS_POS];
      ((Node::IntFunction *)pcode)[0] = &Node::oand_call;
      pcode = code_init;
      break;
    case WAND:
      code_init = pcode + LEN_WAND_NODE + pcode[AND_NODE_NKEYS_POS];
      ((Node::IntFunction *)pcode)[0] = &Node::wand_call;
      pcode = code_init;
      break;
    case NWAND:
      code_init = pcode + LEN_WAND_NODE + pcode[AND_NODE_NKEYS_POS];
      ((Node::IntFunction *)pcode)[0] = &Node::nwand_call;
      pcode = code_init;
      break;
    case OWAND:
      code_init = pcode + LEN_WAND_NODE + pcode[AND_NODE_NKEYS_POS];
      ((Node::IntFunction *)pcode)[0] = &Node::owand_call;
      pcode = code_init;
      break;
    case MAKESET:
      code_init = pcode + LEN_SET_NODE;
      ((Node::IntFunction *)pcode)[0] = &Node::set_call;
      pcode = code_init;
      break;
    case TIMER:
      code_init = pcode + LEN_TIMER_NODE;
      ((Node::IntFunction *)pcode)[0] = &Node::timer_call;
      TimedFuncSubsList = new TimedFuncList(node, pcode, &Node::timer_refresh, TimedFuncSubsList);
      pcode = code_init;
      break;
    case TOR:
      *pcode++ = (ULong)&Node::tor_call;
      n = *pcode++; // Number of jumps
      code_init = pcode + n;
      for (; n > 0; n--)
      {
        *pcode = (ULong)(code_init + *pcode); // each jump
        pcode++;
      }
      break;
    case FCALL:
      *pcode++ = (ULong)&Node::user_func_call;
      if ((void *)(func = (ULong)get_func_pointer((char *)(pcode[1]))) == NULL)
        comp_err("load: user function \"%s\" not found\n", (char *)(pcode[1]));
      *pcode = func;
      pcode += 3; // The pointer, the name and the number of arguments
      break;
    case PCALL:
      *pcode++ = (ULong)&Node::user_proc_call;
      if ((void *)(func = (ULong)get_func_pointer((char *)(pcode[1]))) == NULL)
        comp_err("load: user procedure \"%s\" not found\n", (char *)(pcode[1]));
      *pcode = func;
      pcode += 5; // The pointer, the name, the number of arguments, the tags and the length of code
      break;
    case TCLASS:
      *pcode++ = (ULong)&Node::test_class_call;
      pcode++;
      break;
    case TTRUE:
      *pcode++ = (ULong)&Node::ttrue_call;
      break;
    case TFALSE:
      *pcode++ = (ULong)&Node::tfalse_call;
      break;
    case EVAL:
      *pcode++ = (ULong)&Node::eval_call;
      pcode++; // Length of Code
      break;
    case NOT:
      *pcode++ = (ULong)&Node::not_call;
      break;
    case TNSOBJ:
      *pcode++ = (ULong)&Node::tnsobj_call;
      pcode++;
      break;
    case ADD | TYPE_NUM:
      *pcode++ = (ULong)&Node::addn_call;
      break;
    case ADD | TYPE_FLO:
      *pcode++ = (ULong)&Node::addf_call;
      break;
    case SUB | TYPE_NUM:
      *pcode++ = (ULong)&Node::subn_call;
      break;
    case SUB | TYPE_FLO:
      *pcode++ = (ULong)&Node::subf_call;
      break;
    case MUL | TYPE_NUM:
      *pcode++ = (ULong)&Node::muln_call;
      break;
    case MUL | TYPE_FLO:
      *pcode++ = (ULong)&Node::mulf_call;
      break;
    case DIV | TYPE_NUM:
      *pcode++ = (ULong)&Node::divn_call;
      break;
    case DIV | TYPE_FLO:
      *pcode++ = (ULong)&Node::divf_call;
      break;
    case MINUS | TYPE_NUM:
      *pcode++ = (ULong)&Node::minusn_call;
      break;
    case MINUS | TYPE_FLO:
      *pcode++ = (ULong)&Node::minusf_call;
      break;
    case TEQ | TYPE_STR:
      *pcode++ = (ULong)&Node::teqa_call;
      break;
    case TEQ | TYPE_NUM:
      *pcode++ = (ULong)&Node::teqn_call;
      break;
    case TEQ | TYPE_FLO:
      *pcode++ = (ULong)&Node::teqf_call;
      break;
    case TNE | TYPE_STR:
      *pcode++ = (ULong)&Node::tnea_call;
      break;
    case TNE | TYPE_NUM:
      *pcode++ = (ULong)&Node::tnen_call;
      break;
    case TNE | TYPE_FLO:
      *pcode++ = (ULong)&Node::tnef_call;
      break;
    case TLT | TYPE_STR:
      *pcode++ = (ULong)&Node::tlta_call;
      break;
    case TLT | TYPE_NUM:
      *pcode++ = (ULong)&Node::tltn_call;
      break;
    case TLT | TYPE_FLO:
      *pcode++ = (ULong)&Node::tltf_call;
      break;
    case TLE | TYPE_STR:
      *pcode++ = (ULong)&Node::tlea_call;
      break;
    case TLE | TYPE_NUM:
      *pcode++ = (ULong)&Node::tlen_call;
      break;
    case TLE | TYPE_FLO:
      *pcode++ = (ULong)&Node::tlef_call;
      break;
    case TGE | TYPE_STR:
      *pcode++ = (ULong)&Node::tgea_call;
      break;
    case TGE | TYPE_NUM:
      *pcode++ = (ULong)&Node::tgen_call;
      break;
    case TGE | TYPE_FLO:
      *pcode++ = (ULong)&Node::tgef_call;
      break;
    case TGT | TYPE_STR:
      *pcode++ = (ULong)&Node::tgta_call;
      break;
    case TGT | TYPE_NUM:
      *pcode++ = (ULong)&Node::tgtn_call;
      break;
    case TGT | TYPE_FLO:
      *pcode++ = (ULong)&Node::tgtf_call;
      break;
    case CMP | TYPE_STR:
      *pcode++ = (ULong)&Node::cmpa_call;
      break;
    case CMP | TYPE_NUM:
      *pcode++ = (ULong)&Node::cmpn_call;
      break;
    case CMP | TYPE_FLO:
      *pcode++ = (ULong)&Node::cmpf_call;
      break;
    case SUMS | TYPE_NUM:
      *pcode++ = (ULong)&Node::sumsn_call;
      pcode++; // The data
      break;
    case SUMS | TYPE_FLO:
      *pcode++ = (ULong)&Node::sumsf_call;
      pcode++; // The data
      break;
    case PRDS | TYPE_NUM:
      *pcode++ = (ULong)&Node::prdsn_call;
      pcode++; // The data
      break;
    case PRDS | TYPE_FLO:
      *pcode++ = (ULong)&Node::prdsf_call;
      pcode++; // The data
      break;
    case MINS | TYPE_NUM:
      *pcode++ = (ULong)&Node::minsn_call;
      pcode++; // The data
      break;
    case MINS | TYPE_FLO:
      *pcode++ = (ULong)&Node::minsf_call;
      pcode++; // The data
      break;
    case MINS | TYPE_STR:
      *pcode++ = (ULong)&Node::minsa_call;
      pcode++; // The data
      break;
    case MAXS | TYPE_NUM:
      *pcode++ = (ULong)&Node::maxsn_call;
      pcode++; // The data
      break;
    case MAXS | TYPE_FLO:
      *pcode++ = (ULong)&Node::maxsf_call;
      pcode++; // The data
      break;
    case MAXS | TYPE_STR:
      *pcode++ = (ULong)&Node::maxsa_call;
      pcode++; // The data
      break;
    case COUNT:
      *pcode++ = (ULong)&Node::count_call;
      pcode++; // The data
      break;
    case CONCS:
      *pcode++ = (ULong)&Node::concat_call;
      pcode++; // The data
      break;
    case POPS | TYPE_STR:
      *pcode++ = (ULong)&Node::popsa_call;
      pcode++; // The attribute
      break;
    case POPS | TYPE_NUM:
    case POPS | TYPE_FLO:
      *pcode++ = (ULong)&Node::pops_call;
      pcode++; // The attribute
      break;
    case PUSHO:
      *pcode++ = (ULong)&Node::pusho_call;
      pcode++; // The pattern that refers to
      break;
    case PUSHT:
      *pcode++ = (ULong)&Node::pusht_call;
      pcode++; // The pattern that refers to
      break;
    case PUSHS | TYPE_STR:
    case PUSHS | TYPE_NUM:
    case PUSHS | TYPE_FLO:
      *pcode++ = (ULong)&Node::pushs_call;
      pcode++; // The data
      break;
    case PUSH | TYPE_STR:
      *pcode++ = (ULong)&Node::pusha_call;
      pcode++; // The data
      break;
    case PUSH | TYPE_NUM:
    case PUSH | TYPE_FLO:
      *pcode++ = (ULong)&Node::push_call;
      pcode++; // The data
      break;
    case PROD:
      code_init = pcode + LEN_BASIC_PROD_NODE;
      ((Node::IntFunction *)pcode)[0] = &Node::prod_call;
      if (pcode[PROD_NODE_NOBJIMP_POS] != 0)
        code_init += pcode[PROD_NODE_NOBJIMP_POS]; 
      pcode = code_init;
      break;
    case OBJNEW:
      *pcode++ = (ULong)&Node::new_call;
      pcode += 4; // The object, number of attributes, flags, len_code
      break;
    case OBJMOD:
      *pcode++ = (ULong)&Node::mod_call;
      pcode += 4; // The object, number of attributes, flags, len_code
      break;
    case OBJCHG:
      *pcode++ = (ULong)&Node::mod_wp_call;
      pcode += 4; // The object, number of attributes, flags, len_code
      break;
    case OBJDEL:
      *pcode++ = (ULong)&Node::del_call;
      pcode += 2; // The object and length of code
      break;
    case OBJIMP:
      *pcode++ = (ULong)&Node::objimp_call;
      pcode += 4; // The object, number of attributes, flags (not used), len_code
      break;
    case ENDRULE:
      *pcode++ = (ULong)&Node::endrule_call;
      break;

    default:
      engine_fatal_err("load_code: Unknown code (0x%X)\n", *pcode);
    }
  }
}

/**
 * @brief Reset code. Empty the memories of the INTER nodes, Sets and Timers
 * 
 * @param code Array of code
 * @param len_code length of code
 * @param free_code if the code must be freed also
 */
PUBLIC
void reset_code(ULong *code, int len_code, int free_code)
{
  ULong *limit;
  ULong n;
  ULong *pcode, *code_init;

  pcode = code;
  limit = pcode + len_code;

  while (pcode < limit)
  {
    switch (dasm_code(*pcode))
    {
    case AND:
    {
      code_init = pcode + LEN_AND_NODE + pcode[AND_NODE_NKEYS_POS];
      BTree *t1 = (BTree *)pcode[AND_NODE_MEM_START_POS + 0];
      KeyManager keyman1(LEFT_MEM, LEFT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_AND_NODE);
      t1->setKeyManager(&keyman1);
      t1->Free(del_and_node_mem_item);
      if (free_code)
        delete t1;
      BTree *t2 = (BTree *)pcode[AND_NODE_MEM_START_POS + 1];
      KeyManager keyman2(RIGHT_MEM, RIGHT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_AND_NODE);
      t2->setKeyManager(&keyman2);
      t2->Free(del_and_node_mem_item);
      if (free_code)
        delete t2;
      pcode = code_init;
    }
    break;
    case NAND:
    {
      code_init = pcode + LEN_AND_NODE + pcode[AND_NODE_NKEYS_POS];
      BTree *t1 = (BTree *)pcode[AND_NODE_MEM_START_POS + 0];
      KeyManager keyman1(LEFT_MEM, LEFT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_AND_NODE, true);
      t1->setKeyManager(&keyman1);
      t1->Free(del_asym_node_lmem_item);
      if (free_code)
        delete t1;
      BTree *t2 = (BTree *)pcode[AND_NODE_MEM_START_POS + 1];
      KeyManager keyman2(RIGHT_MEM, RIGHT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_AND_NODE);
      t2->setKeyManager(&keyman2);
      t2->Free(del_and_node_mem_item);
      if (free_code)
        delete t2;
      pcode = code_init;
    }
    break;
    case OAND:
    {
      code_init = pcode + LEN_AND_NODE + pcode[AND_NODE_NKEYS_POS];
      BTree *t1 = (BTree *)pcode[AND_NODE_MEM_START_POS + 0];
      KeyManager keyman1(LEFT_MEM, LEFT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_AND_NODE, true);
      t1->setKeyManager(&keyman1);
      t1->Free(del_asym_node_lmem_item);
      if (free_code)
        delete t1;
      BTree *t2 = (BTree *)pcode[AND_NODE_MEM_START_POS + 1];
      KeyManager keyman2(RIGHT_MEM, RIGHT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_AND_NODE);
      t2->setKeyManager(&keyman2);
      t2->Free(del_and_node_mem_item);
      if (free_code)
        delete t2;
      pcode = code_init;
    }
    break;
    case WAND:
    {
      code_init = pcode + LEN_WAND_NODE + pcode[AND_NODE_NKEYS_POS];
      BTree *t1 = (BTree *)pcode[WAND_NODE_MEM_START_POS + 0];
      KeyManager keyman1(LEFT_MEM, LEFT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_WAND_NODE);
      t1->setKeyManager(&keyman1);
      t1->Free(del_and_node_mem_item);
      if (free_code)
        delete t1;
      BTree *t2 = (BTree *)pcode[WAND_NODE_MEM_START_POS + 1];
      KeyManager keyman2(RIGHT_MEM, RIGHT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_WAND_NODE);
      t2->setKeyManager(&keyman2);
      t2->Free(del_and_node_mem_item);
      if (free_code)
        delete t2;
      pcode = code_init;
    }
    break;
    case NWAND:
    {
      code_init = pcode + LEN_WAND_NODE + pcode[AND_NODE_NKEYS_POS];
      BTree *t1 = (BTree *)pcode[WAND_NODE_MEM_START_POS + 0];
      KeyManager keyman1(LEFT_MEM, LEFT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_WAND_NODE, true);
      t1->setKeyManager(&keyman1);
      t1->Free(del_asym_node_lmem_item);
      if (free_code)
        delete t1;
      BTree *t2 = (BTree *)pcode[WAND_NODE_MEM_START_POS + 1];
      KeyManager keyman2(RIGHT_MEM, RIGHT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_WAND_NODE);
      t2->setKeyManager(&keyman2);
      t2->Free(del_and_node_mem_item);
      if (free_code)
        delete t2;
      pcode = code_init;
    }
    break;
    case OWAND:
    {
      code_init = pcode + LEN_WAND_NODE + pcode[AND_NODE_NKEYS_POS];
      BTree *t1 = (BTree *)pcode[WAND_NODE_MEM_START_POS + 0];
      KeyManager keyman1(LEFT_MEM, LEFT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_WAND_NODE, true);
      t1->setKeyManager(&keyman1);
      t1->Free(del_asym_node_lmem_item);
      if (free_code)
        delete t1;
      BTree *t2 = (BTree *)pcode[WAND_NODE_MEM_START_POS + 1];
      KeyManager keyman2(RIGHT_MEM, RIGHT_MEM, pcode[AND_NODE_NKEYS_POS], pcode + LEN_WAND_NODE);
      t2->setKeyManager(&keyman2);
      t2->Free(del_and_node_mem_item);
      if (free_code)
        delete t2;
      pcode = code_init;
    }
    break;
    case MAKESET:
    {
      code_init = pcode + LEN_SET_NODE;
      BTree *t = (BTree *)pcode[SET_NODE_MEM_POS];
      t->Free(del_set_node_mem_item, pcode);
      if (free_code)
        delete t;
      pcode = code_init;
    }
    break;
    case TIMER:
    {
      code_init = pcode + LEN_TIMER_NODE;
      BTree *t = (BTree *)pcode[TIMER_NODE_MEM_POS];
      t->Free(del_and_node_mem_item);
      if (free_code)
        delete t;
      if (free_code)
        TimedFuncList::unsubscribe(pcode, TimedFuncSubsList);
      pcode = code_init;
    }
    break;
    case TOR:
      pcode++;
      n = *pcode++; // Number of jumps
      code_init = pcode + n;
      pcode = code_init;
      break;
    case FCALL:
      if (free_code)
        free((void *)(pcode[2])); // free of name
      pcode += 4; 
      break;
    case PCALL:
      if (free_code)
        free((void *)(pcode[2])); // free of name
      pcode += 6;      
      break;
    case TCLASS:
      pcode += 2; // The name of class points to the name buffers inside ObjClass
      break;
    case TTRUE:
    case TFALSE:
    case NOT:
      pcode++;
      break;
    case EVAL:
    case TNSOBJ:
      pcode += 2;
      break;
    case ADD | TYPE_NUM:
    case ADD | TYPE_FLO:
    case SUB | TYPE_NUM:
    case SUB | TYPE_FLO:
    case MUL | TYPE_NUM:
    case MUL | TYPE_FLO:
    case DIV | TYPE_NUM:
    case DIV | TYPE_FLO:
    case MINUS | TYPE_NUM:
    case MINUS | TYPE_FLO:
    case TEQ | TYPE_STR:
    case TEQ | TYPE_NUM:
    case TEQ | TYPE_FLO:
    case TNE | TYPE_STR:
    case TNE | TYPE_NUM:
    case TNE | TYPE_FLO:
    case TLT | TYPE_STR:
    case TLT | TYPE_NUM:
    case TLT | TYPE_FLO:
    case TLE | TYPE_STR:
    case TLE | TYPE_NUM:
    case TLE | TYPE_FLO:
    case TGE | TYPE_STR:
    case TGE | TYPE_NUM:
    case TGE | TYPE_FLO:
    case TGT | TYPE_STR:
    case TGT | TYPE_NUM:
    case TGT | TYPE_FLO:
    case CMP | TYPE_STR:
    case CMP | TYPE_NUM:
    case CMP | TYPE_FLO:
      pcode++;
      break;
    case SUMS | TYPE_NUM:
    case SUMS | TYPE_FLO:
    case PRDS | TYPE_NUM:
    case PRDS | TYPE_FLO:
    case MINS | TYPE_NUM:
    case MINS | TYPE_FLO:
    case MINS | TYPE_STR:
    case MAXS | TYPE_NUM:
    case MAXS | TYPE_FLO:
    case MAXS | TYPE_STR:
    case COUNT:
    case CONCS:
      pcode += 2;
      break;
    case PUSHO:
      pcode += 2;
      break;
    case PUSHT:
      pcode += 2;
      break;
    case POPS | TYPE_STR:
    case POPS | TYPE_NUM:
    case POPS | TYPE_FLO:
    case PUSHS | TYPE_STR:
    case PUSHS | TYPE_NUM:
    case PUSHS | TYPE_FLO:
      pcode += 2;
      break;
    case PUSH | TYPE_STR:
      pcode++;
      if (free_code)
        free((void *)(*pcode)); // The attribute
      pcode++;
      break;
    case PUSH | TYPE_NUM:
    case PUSH | TYPE_FLO:
      pcode += 2;
      break;
    case PROD:
      code_init = pcode + LEN_BASIC_PROD_NODE;
      if (free_code)
        free((void *)(pcode[PROD_NODE_RULENAME_POS]));
      if (free_code)
        free((void *)(pcode[PROD_NODE_RULESETNM_POS]));
      if (pcode[PROD_NODE_NOBJIMP_POS] != 0)
        code_init += pcode[PROD_NODE_NOBJIMP_POS]; 

      // To simplify the PROD nodes always has a position for memory even when it is only needed
      // when there are implied objects
      
      {
        BTree *t = (BTree *)pcode[PROD_NODE_MEM_POS];
        t->Free(del_prod_node_mem_item, pcode + PROD_START_OBJ_POS, pcode[PROD_NODE_NOBJIMP_POS]);
        if (free_code)
          delete t;
      }
      pcode = code_init;
      break;
    case OBJCHG:
    case OBJMOD:
    case OBJNEW:
    case OBJIMP:
      pcode += 5;
      break;
    case OBJDEL:
      pcode += 3;
      break;
    case ENDRULE:
      pcode++;
      break;

    default:
      engine_fatal_err("free_code: Unknown code (0x%X)\n", *pcode);
    }
  }

  if (free_code)
    free(code);
}

/**
 * @brief Refresh engine calling to the timed functions with a real timestamp in order to free 
 *      those items that are timed or are in some memory at a Time Windowed Rule memory
 * 
 * @param real_time Time stamp
 */
PUBLIC
void engine_refresh(long real_time)
{
  TimedFuncSubsList->refresh(real_time);
}

/**
 * @brief Remove an item in a memory of an AND node. Simply unlink the MetaObj
 * 
 * @param item 
 * @return PRIVATE 
 */
PRIVATE
void del_and_node_mem_item(void *item, va_list)
{
  ((MetaObj *)item)->unlink();
}

/**
 * @brief Remove an set in a memory of an AND node. The set is identified and called flush_set() method
 * 
 * @param item MetaObj 
 */
PRIVATE
void del_set_node_mem_item(void *item, va_list list)
{

  va_list copy;
  va_copy(copy, list);
  ULong *base_dir = va_arg(copy, ULong *);
  va_end(copy);

  Set *set = (*(((MetaObj *)item)->meta_dir((MetaObj **)(&item), (int)(base_dir[SET_NODE_FIRST_ITEM_POS] >> 8), (int)(base_dir[SET_NODE_N_ITEMS_POS]))))->set();

  // We empty SET
  set->flush_set();

  // Delete the duplicated struct
  ((MetaObj *)item)->delete_struct(FALSE, FALSE);
}

/**
 * @brief Remove an item in the left memory of an asymetric inter node. In this nodes insted a MetaObj they have a MatchCount
 * 
 * @param count MatchCount object
 */
void del_asym_node_lmem_item(void *count, va_list)
{
  ((MatchCount *)count)->item->unlink();
  delete ((MatchCount *)count);
}

/**
 * @brief Remove an item in a production node memory
 * 
 * @param item of the memory
 * @param list: additional data needed to retract implied objects
 */
PRIVATE
void del_prod_node_mem_item(void *item, va_list list)
{
  int *ind_obj;
  ULong n_objs_impl;
  MetaObj *prod_item = (MetaObj *)item;
  MetaObj *Left, *Right;
  MetaObj *item_impl;
  Action *act;
  int n;
  va_list copy;

  va_copy(copy, list);
  ind_obj = va_arg(copy, int *);
  n_objs_impl = va_arg(copy, ULong);
  va_end (copy);

  prod_item->compound()->expand(&Left, &Right);

  for (n = 0; n < n_objs_impl; n++)
  {
    item_impl = (*Right)[ind_obj[n]];
    if (on_ruleset)
    {
      item_impl->link(); // It is for the inference, to be done
      act = new Action(RETRACT_TAG, item_impl->single(), NULL, item_impl->single()->obj());
      act->push();
      n_objs_retract++;
    }
  }

  if (Right != NULL)
    Right->delete_struct(TRUE, FALSE);
  Left->unlink();

  prod_item->unlink();
}
