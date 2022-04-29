/**
 * @file dasm_rete.cpp
 * @author Francisco Alcaraz
 * @brief Disassemble of the code stored in a node of the net
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "nodes.hpp"
#include "classes.hpp"
#include "dasm_rete_p.hpp"

struct assoc
{
  Node::IntFunction func;
  ULong code;
};

PRIVATE
assoc assoc_matrix[] = {
    &Node::user_func_call, FCALL,
    &Node::user_proc_call, PCALL,
    &Node::test_class_call, TCLASS,
    &Node::tnsobj_call, TNSOBJ,

    &Node::ttrue_call, TTRUE,
    &Node::tfalse_call, TFALSE,
    &Node::eval_call, EVAL,
    &Node::not_call, NOT,

    &Node::addn_call, (ADD | TYPE_NUM),
    &Node::addf_call, (ADD | TYPE_FLO),
    &Node::subn_call, (SUB | TYPE_NUM),
    &Node::subf_call, (SUB | TYPE_FLO),
    &Node::muln_call, (MUL | TYPE_NUM),
    &Node::mulf_call, (MUL | TYPE_FLO),
    &Node::divn_call, (DIV | TYPE_NUM),
    &Node::divf_call, (DIV | TYPE_FLO),
    &Node::minusn_call, (MINUS | TYPE_NUM),
    &Node::minusf_call, (MINUS | TYPE_FLO),

    &Node::teqa_call, (TEQ | TYPE_STR),
    &Node::tnea_call, (TNE | TYPE_STR),
    &Node::tlta_call, (TLT | TYPE_STR),
    &Node::tlea_call, (TLE | TYPE_STR),
    &Node::tgea_call, (TGE | TYPE_STR),
    &Node::tgta_call, (TGT | TYPE_STR),
    &Node::cmpa_call, (CMP | TYPE_STR),

    &Node::teqn_call, (TEQ | TYPE_NUM),
    &Node::tnen_call, (TNE | TYPE_NUM),
    &Node::tltn_call, (TLT | TYPE_NUM),
    &Node::tlen_call, (TLE | TYPE_NUM),
    &Node::tgen_call, (TGE | TYPE_NUM),
    &Node::tgtn_call, (TGT | TYPE_NUM),
    &Node::cmpn_call, (CMP | TYPE_NUM),

    &Node::teqf_call, (TEQ | TYPE_FLO),
    &Node::tnef_call, (TNE | TYPE_FLO),
    &Node::tltf_call, (TLT | TYPE_FLO),
    &Node::tlef_call, (TLE | TYPE_FLO),
    &Node::tgef_call, (TGE | TYPE_FLO),
    &Node::tgtf_call, (TGT | TYPE_FLO),
    &Node::cmpf_call, (CMP | TYPE_FLO),

    &Node::sumsn_call, (SUMS | TYPE_NUM),
    &Node::sumsf_call, (SUMS | TYPE_FLO),
    &Node::prdsn_call, (PRDS | TYPE_NUM),
    &Node::prdsf_call, (PRDS | TYPE_FLO),
    &Node::minsn_call, (MINS | TYPE_NUM),
    &Node::minsf_call, (MINS | TYPE_FLO),
    &Node::minsa_call, (MINS | TYPE_STR),
    &Node::maxsn_call, (MAXS | TYPE_NUM),
    &Node::maxsf_call, (MAXS | TYPE_FLO),
    &Node::maxsa_call, (MAXS | TYPE_STR),
    &Node::count_call, COUNT,
    &Node::concat_call, CONCS,
    &Node::push_call, (PUSH | TYPE_NUM),
    &Node::pusha_call, (PUSH | TYPE_STR),
    &Node::pushs_call, (PUSHS | TYPE_NUM),
    &Node::pusho_call, PUSHO,
    &Node::pusht_call, PUSHT,

    &Node::tor_call, TOR,
    &Node::and_call, AND,
    &Node::nand_call, NAND,
    &Node::oand_call, OAND,
    &Node::wand_call, WAND,
    &Node::nwand_call, NWAND,
    &Node::owand_call, OWAND,
    &Node::set_call, MAKESET,
    &Node::timer_call, TIMER,
    &Node::prod_call, PROD,

    &Node::mod_call, OBJMOD,
    &Node::del_call, OBJDEL,
    &Node::new_call, OBJNEW,
    &Node::objimp_call, OBJIMP,
    &Node::mod_wp_call, OBJCHG,

    &Node::popsa_call, (POPS | TYPE_STR),
    &Node::pops_call, (POPS | TYPE_NUM),

    &Node::endrule_call, ENDRULE,
    NULL, 0};

/**
 * @brief Convert the stored function pointer to its code
 * 
 * @param code Function Pointer
 * @return ULong code equivalent 
 */
PUBLIC
ULong dasm_code(ULong code)
{
  int n;
  Node::IntFunction func = (Node::IntFunction)code;
  static ULong last = 0;

  for (n = 0; assoc_matrix[n].func != (Node::IntFunction)NULL && assoc_matrix[n].func != func; n++)
    ;

  if (assoc_matrix[n].func == (Node::IntFunction)NULL)
    fprintf(stderr, "Unknown code in dasm_code after processing %-.3lX\n", last);

  last = assoc_matrix[n].code;

  return assoc_matrix[n].code;
}

/**
 * @brief print a complete disassembled RETE code of a node, formed by an op code and its parameters
 * 
 * @param prefix Used for padding the printing
 * @param code_array to be disassembled
 * @param codelen Length of the code
 */
PUBLIC
void dasm_rete(const char *prefix, ULong *code_array, int codelen)
{

  ULong code, type;
  ULong m;
  ULong pp = 0;
  ULong pp_inic;
  float float_num;

  for (pp = 0; pp < codelen; pp++)
  {
    fprintf(trace_file, "%s", prefix);
    code = dasm_code(code_array[pp]);
    switch (code)
    {
      case PROD:
        fprintf(trace_file, "%lu\tPROD\t%s/%s (cat=%lu, #implobj=%lu", pp,
              (char *)code_array[pp + PROD_NODE_RULENAME_POS],
              (char *)code_array[pp + PROD_NODE_RULESETNM_POS],
              code_array[pp + PROD_NODE_RULECAT_POS],
              code_array[pp + PROD_NODE_NOBJIMP_POS]);
        if (code_array[pp + PROD_NODE_NOBJIMP_POS] != 0)
        {
          /* Space for implied objects positions*/
          fprintf(trace_file, "={ ");
          for (m = 0; m < code_array[pp + PROD_NODE_NOBJIMP_POS]; m++)
            fprintf(trace_file, "%lu ", code_array[pp + PROD_START_OBJ_POS + m]);
          fprintf(trace_file, "}");
          pp += LEN_BASIC_PROD_NODE + code_array[pp + PROD_NODE_NOBJIMP_POS] - 1;
        }
        else
          pp += LEN_BASIC_PROD_NODE - 1;
        fprintf(trace_file, ")\n");
        break;

      case AND:
        fprintf(trace_file, "%lu\tAND\tLR=%lu/%lu CODE=%lu FLAGS=", pp,
              (code_array[pp + AND_NODE_N_ITEMS_POS] >> 16) & 0xFF,
              code_array[pp + AND_NODE_N_ITEMS_POS] & 0xFF,
              code_array[pp + AND_NODE_CODELEN_POS]);
        print_flags(code_array[pp + AND_NODE_FLAGS_POS]);
        fprintf(trace_file, " KEYS=%lu", code_array[pp + AND_NODE_NKEYS_POS]);
        fprintf(trace_file, "\n");
        /*
        * There are two other data used to store the memories
        */
        pp += LEN_AND_NODE - 1 + code_array[AND_NODE_NKEYS_POS];
        break;

      case NAND:
        fprintf(trace_file, "%lu\tNAND\tLR=%lu/%lu CODE=%lu FLAGS=", pp,
              (code_array[pp + AND_NODE_N_ITEMS_POS] >> 16) & 0xFF,
              code_array[pp + AND_NODE_N_ITEMS_POS] & 0xFF,
              code_array[pp + AND_NODE_CODELEN_POS]);
        print_flags(code_array[pp + AND_NODE_FLAGS_POS]);
        fprintf(trace_file, " KEYS=%lu", code_array[pp + AND_NODE_NKEYS_POS]);
        fprintf(trace_file, "\n");
        /*
        * There are two other data used to store the memories
        */
        pp += LEN_AND_NODE - 1 + code_array[AND_NODE_NKEYS_POS];
        break;
      case OAND:
        fprintf(trace_file, "%lu\tOAND\tLR=%lu/%lu CODE=%lu FLAGS=", pp,
              (code_array[pp + AND_NODE_N_ITEMS_POS] >> 16) & 0xFF,
              code_array[pp + AND_NODE_N_ITEMS_POS] & 0xFF,
              code_array[pp + AND_NODE_CODELEN_POS]);
        print_flags(code_array[pp + AND_NODE_FLAGS_POS]);
        fprintf(trace_file, " KEYS=%lu", code_array[pp + AND_NODE_NKEYS_POS]);
        fprintf(trace_file, "\n");
        /*
        * There are two other data used to store the memories
        */
        pp += LEN_AND_NODE - 1 + code_array[AND_NODE_NKEYS_POS];
        break;
      case WAND:
        fprintf(trace_file, "%lu\tWAND\tLR=%lu/%lu CODE=%lu WINDOW=%lu FLAGS=", pp,
              (code_array[pp + AND_NODE_N_ITEMS_POS] >> 16) & 0xFF,
              code_array[pp + AND_NODE_N_ITEMS_POS] & 0xFF,
              code_array[pp + AND_NODE_CODELEN_POS],
              code_array[pp + WAND_NODE_WTIME_POS]);
        print_flags(code_array[pp + AND_NODE_FLAGS_POS]);
        fprintf(trace_file, " KEYS=%lu", code_array[pp + AND_NODE_NKEYS_POS]);
        fprintf(trace_file, "\n");
        /*
        * There are two other data used to store the memories
        */
        pp += LEN_WAND_NODE - 1 + code_array[AND_NODE_NKEYS_POS];
        break;
      case NWAND:
        fprintf(trace_file, "%lu\tNWAND\tLR=%lu/%lu CODE=%lu WINDOW=%lu FLAGS=", pp,
              (code_array[pp + AND_NODE_N_ITEMS_POS] >> 16) & 0xFF,
              code_array[pp + AND_NODE_N_ITEMS_POS] & 0xFF,
              code_array[pp + AND_NODE_CODELEN_POS],
              code_array[pp + WAND_NODE_WTIME_POS]);
        print_flags(code_array[pp + AND_NODE_FLAGS_POS]);
        fprintf(trace_file, " KEYS=%lu", code_array[pp + AND_NODE_NKEYS_POS]);
        fprintf(trace_file, "\n");
        /*
        * There are two other data used to store the memories
        */
        pp += LEN_WAND_NODE - 1 + code_array[AND_NODE_NKEYS_POS];
        break;
      case OWAND:
        fprintf(trace_file, "%lu\tOWAND\tLR=%lu/%lu CODE=%lu WINDOW=%lu FLAGS=", pp,
              (code_array[pp + AND_NODE_N_ITEMS_POS] >> 16) & 0xFF,
              code_array[pp + AND_NODE_N_ITEMS_POS] & 0xFF,
              code_array[pp + AND_NODE_CODELEN_POS],
              code_array[pp + WAND_NODE_WTIME_POS]);
        print_flags(code_array[pp + AND_NODE_FLAGS_POS]);
        fprintf(trace_file, " KEYS=%lu", code_array[pp + AND_NODE_NKEYS_POS]);
        fprintf(trace_file, "\n");
        /*
        * There are two other data used to store the memories
        */
        pp += LEN_WAND_NODE - 1 + code_array[AND_NODE_NKEYS_POS];
        break;
      case TOR:
        fprintf(trace_file, "%lu\tTOR\t", pp++);
        pp_inic = pp + code_array[pp] + 1;
        for (m = code_array[pp]; m > 0; m--)
        {
          pp++;
          fprintf(trace_file, "%lu ", pp_inic + code_array[pp]);
        }
        puts("");
        break;
      case TTRUE:
        fprintf(trace_file, "%lu\tTTRUE\n", pp);
        break;
      case TFALSE:
        fprintf(trace_file, "%lu\tTFALSE\n", pp);
        break;
      case NOT:
        fprintf(trace_file, "%lu\tNOT\n", pp);
        break;
      case EVAL:
        fprintf(trace_file, "%lu\tEVAL\tCODE=%lu\n", pp, code_array[pp + 1]);
        pp += 1;
        break;
      case FCALL:
        fprintf(trace_file, "%lu\tFCALL\t%s\n", pp, (char *)code_array[pp + 2]);
        /* There is an additional data that is the number of arguments in pp+2 */
        pp += 3;
        break;
      case PCALL:
        fprintf(trace_file, "%lu\tPCALL\t%s (%lu nargs, %c%c%c tags, %lu code)\n", pp,
              (char *)code_array[pp + 2], code_array[pp + 3],
              code_array[pp + 4] & EXEC_INSERT ? 'I' : '-',
              code_array[pp + 4] & EXEC_MODIFY ? 'M' : '-',
              code_array[pp + 4] & EXEC_RETRACT ? 'R' : '-',
              code_array[pp + 5]);
        pp += 5;
        break;
      case OBJNEW:
        fprintf(trace_file, "%lu\tCREATE\t%s (%lu attrs, %c%c%c tags, %lu code)\n", pp,
              objstr(code_array[pp + 1]), code_array[pp + 2],
              code_array[pp + 3] & EXEC_INSERT ? 'I' : '-',
              code_array[pp + 3] & EXEC_MODIFY ? 'M' : '-',
              code_array[pp + 3] & EXEC_RETRACT ? 'R' : '-',
              code_array[pp + 4]);
        pp += 4;
        break;
      case OBJMOD:
        fprintf(trace_file, "%lu\tMODIFY\t%s (%lu attrs, %c%c%c tags, %lu code)\n", pp,
              objstr(code_array[pp + 1]), code_array[pp + 2],
              code_array[pp + 3] & EXEC_INSERT ? 'I' : '-',
              code_array[pp + 3] & EXEC_MODIFY ? 'M' : '-',
              code_array[pp + 3] & EXEC_RETRACT ? 'R' : '-',
              code_array[pp + 4]);
        pp += 4;
        break;
      case OBJCHG:
        fprintf(trace_file, "%lu\tCHANGE\t%s (%lu attrs, %c%c%c tags, %lu code)\n", pp,
              objstr(code_array[pp + 1]), code_array[pp + 2],
              code_array[pp + 3] & EXEC_INSERT ? 'I' : '-',
              code_array[pp + 3] & EXEC_MODIFY ? 'M' : '-',
              code_array[pp + 3] & EXEC_RETRACT ? 'R' : '-',
              code_array[pp + 4]);
        pp += 4;
        break;
      case OBJDEL:
        fprintf(trace_file, "%lu\tDELETE\t%s (%c%c%c tags)\n", pp,
              objstr(code_array[pp + 1]),
              code_array[pp + 2] & EXEC_INSERT ? 'I' : '-',
              code_array[pp + 2] & EXEC_MODIFY ? 'M' : '-',
              code_array[pp + 2] & EXEC_RETRACT ? 'R' : '-');
        pp += 2;
        break;
      case OBJIMP:
        fprintf(trace_file, "%lu\tOBJIMP\t%s (%lu attrs, %lu code)\n", pp,
              objstr(code_array[pp + 1]), code_array[pp + 2], code_array[pp + 4]);
        pp += 4;
        break;
      case ENDRULE:
        fprintf(trace_file, "%lu\tENDRULE\n", pp);
        break;
      case TCLASS:
        fprintf(trace_file, "%lu\tTCLASS\t%s\n", pp, (char *)code_array[pp + 1]);
        pp++;
        break;
      case TNSOBJ:
        fprintf(trace_file, "%lu\tTNSOBJ\t%s", pp, objstr(code_array[pp + 1] >> 16));
        fprintf(trace_file, ", %s\n", objstr(code_array[pp + 1] & 0xFFFF));
        pp++;
        break;
      case PUSHO:
        fprintf(trace_file, "%lu\tPUSHO\t%s\n", pp, objstr(code_array[pp + 1]));
        pp++;
        break;
      case PUSHT:
        fprintf(trace_file, "%lu\tPUSHT\t%s\n", pp, objstr(code_array[pp + 1]));
        pp++;
        break;
      case COUNT:
        fprintf(trace_file, "%lu\tCOUNT\t%s\n", pp, objstr(code_array[pp + 1]));
        pp++;
        break;
      case CONCS:
        fprintf(trace_file, "%lu\tCONCS\t%s\n", pp, objstr(code_array[pp + 1]));
        pp++;
        break;
      case MAKESET:
        fprintf(trace_file, "%lu\tSET\t%s NOBJS=%lu CODE=%lu MASKS=[", pp,
              objstr(code_array[pp + SET_NODE_FIRST_ITEM_POS]),
              code_array[pp + SET_NODE_N_ITEMS_POS],
              code_array[pp + SET_NODE_CODELEN_POS]);
        Node::printPatternMask(code_array + pp + SET_NODE_PATT_MASK);
        fprintf(trace_file, "]\n");
        pp += LEN_SET_NODE - 1;
        break;
      case TIMER:
        fprintf(trace_file, "%lu\tTIMER\tW=%lu\n", pp, code_array[pp + TIMER_NODE_WINDOW_POS]);
        pp += LEN_TIMER_NODE - 1;
        break;

      default:

        type = code & 0x3;
        code &= ~0x3;

        switch (code)
        {
        case ADD:
          fprintf(trace_file, "%lu\tADD", pp);
          break;
        case SUB:
          fprintf(trace_file, "%lu\tSUB", pp);
          break;
        case MUL:
          fprintf(trace_file, "%lu\tMUL", pp);
          break;
        case DIV:
          fprintf(trace_file, "%lu\tDIV", pp);
          break;
        case MINUS:
          fprintf(trace_file, "%lu\tMINUS", pp);
          break;
        case TEQ:
          fprintf(trace_file, "%lu\tTEQ", pp);
          break;
        case TNE:
          fprintf(trace_file, "%lu\tTNE", pp);
          break;
        case TLT:
          fprintf(trace_file, "%lu\tTLT", pp);
          break;
        case TGT:
          fprintf(trace_file, "%lu\tTGT", pp);
          break;
        case TLE:
          fprintf(trace_file, "%lu\tTLE", pp);
          break;
        case TGE:
          fprintf(trace_file, "%lu\tTGE", pp);
          break;
        case CMP:
          fprintf(trace_file, "%lu\tCMP", pp);
          break;
        case PUSH:
          fprintf(trace_file, "%lu\tPUSH", pp);
          break;
        case PUSHS:
          fprintf(trace_file, "%lu\tPUSHS", pp);
          break;
        case POPS:
          fprintf(trace_file, "%lu\tPOPS", pp);
          break;
        case SUMS:
          fprintf(trace_file, "%lu\tSUMS", pp);
          break;
        case PRDS:
          fprintf(trace_file, "%lu\tPRDS", pp);
          break;
        case MINS:
          fprintf(trace_file, "%lu\tMINS", pp);
          break;
        case MAXS:
          fprintf(trace_file, "%lu\tMAXS", pp);
          break;
        default:
          fprintf(trace_file, "%lu\t(%lX)", pp, code | type);
          break;
        }

        print_type(type);

        if (code == PUSHS || code == POPS ||
            code == SUMS || code == PRDS ||
            code == MINS || code == MAXS)
        {
          pp++;
          fprintf(trace_file, "%s\n", objstr(code_array[pp]));
        }

        else if (code == PUSH)
        {
          pp++;
          switch (type)
          {
          case TYPE_STR:
            if (code_array[pp] == 0)
              fprintf(trace_file, "NULL\n");
            else
              fprintf(trace_file, "\"%s\"\n", (char *)code_array[pp]);
            break;
          case TYPE_NUM:
            fprintf(trace_file, "%lu\n", code_array[pp]);
            break;
          case TYPE_FLO:
            memcpy((char *)&float_num, &(code_array[pp]),
                  sizeof(float));
            fprintf(trace_file, "%.3g\n", float_num);
            break;
          }
        }

        else
          puts("");
    }
  }
}

PRIVATE
void print_type(ULong type)
{

  switch (type)
  {
  case TYPE_STR:
    fprintf(trace_file, "A\t");
    break;
  case TYPE_NUM:
    fprintf(trace_file, "N\t");
    break;
  case TYPE_FLO:
    fprintf(trace_file, "F\t");
    break;
  default:
    fprintf(trace_file, "Tipo desconocido %lX\n", type);
  }
}

/**
 * @brief This function prints the reference to an attribute of an object in the rule, in a compact way
 *      The reference is stored in 16 bits: MPPP PPPP AAAA AAAA 
 *      M is Memory 0: Left, 1 Right, P Position of Object in the rule, A Attribute number
 * 
 * @param data the reference
 * @return char * the equivalent more readable string
 */
PRIVATE
char *objstr(unsigned long data)
{
  UChar pos, attr;
  const char *mem;

  static char buffer[40];

  mem = ((((data >> 15) & 0x1) == 1) ? "R" : "");

  pos = ((data >> 8) & 0x7F);

  attr = (data & 0xFF);

  sprintf(buffer, "%s{%u}.%u", mem, pos, attr);

  return (buffer);
}

/**
 * @brief This function prints the flags related to every object in the LHS of the rule
 *        The flags are stored in different bits and are checked during matching in nodes where 
 *        some objects come to the node by its left side and other by its right 
 *        (the pairs that would pass the filter condition of the node will go deeper 
 *        and coupled in a Compound Meta Object, maintaining their side positions)
 *        The upper 16 bits are the flags of the object by left and the lower 16 bits are those of the object by right
 * 
 * @param double_flags The flags of the objects that arrive the node by left or by right
 */
PRIVATE
void print_flags(ULong double_flags)
{
  ULong nflags;
  ULong flags;

  flags = (double_flags >> 16);
  nflags = 0;

  if (flags & IS_TRIGGER)
  {
    fprintf(trace_file, "%sTRG", (nflags == 0) ? "(" : ", ");
    nflags++;
  }
  if (flags & IS_TEMPORAL)
  {
    fprintf(trace_file, "%sTEMP", (nflags == 0) ? "(" : ", ");
    nflags++;
  }

  if (flags & IS_TIMED)
    fprintf(trace_file, "%sW", (nflags == 0) ? "(" : ", ");
  else
    fprintf(trace_file, "%sNW", (nflags == 0) ? "(" : ", ");

  fprintf(trace_file, ")/");

  flags = (double_flags & 0xFFFF);
  nflags = 0;

  if (flags & IS_TRIGGER)
  {
    fprintf(trace_file, "%sTRG", (nflags == 0) ? "(" : ", ");
    nflags++;
  }
  if (flags & IS_TEMPORAL)
  {
    fprintf(trace_file, "%sTEMP", (nflags == 0) ? "(" : ", ");
    nflags++;
  }

  if (flags & IS_TIMED)
    fprintf(trace_file, "%sW", (nflags == 0) ? "(" : ", ");
  else
    fprintf(trace_file, "%sNW", (nflags == 0) ? "(" : ", ");

  fprintf(trace_file, ")");
}
