/**
 * @file codes.hpp
 * @author Francisco Alcaraz
 * @brief Operation Codes Constants
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef PUBLIC
#define PUBLIC
#endif

#ifndef PRIVATE
#define PRIVATE static
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef ERROR
#define ERROR -1
#endif

#ifndef _CODES_H
#define _CODES_H

#define FORK 0x00
#define MERGE 0x01
#define GOTO 0x02
#define AND 0x03
#define NAND 0x04
#define OAND 0x05
#define WAND 0x06
#define NWAND 0x07
#define OWAND 0x08
#define MAKESET 0x09
#define PROD 0x0A
#define TIMER 0x0B

#define ADD 0x100
#define SUB 0x104
#define MUL 0x108
#define DIV 0x10C
#define MINUS 0x110

#define TCLASS 0x200
#define TOR 0x201
#define TNSOBJ 0x202
#define TTRUE 0x203
#define TFALSE 0x204
#define EVAL 0x205
#define NOT 0x206
#define TEQ 0x210
#define TNE 0x214
#define TLT 0x218
#define TGT 0x21C
#define TLE 0x220
#define TGE 0x224
#define CMP 0x228

#define FCALL 0x300
#define PCALL 0x301

#define PUSH 0x400
#define PUSHS 0x404
#define POPS 0x408

#define PUSHO 0x410
#define PUSHT 0x411

#define COUNT 0x500
#define SUMS 0x504
#define PRDS 0x508
#define MINS 0x50C
#define MAXS 0x510
#define CONCS 0x514

#define OBJMOD 0x1000
#define OBJDEL 0x1001
#define OBJNEW 0x1002
#define OBJIMP 0x1003
#define ENDRULE 0x1004
#define OBJCHG 0x1005

/* To manage expressions at compiling time */

#define TAND 0xFF00
#define TNOT 0xFF01
#define CAST_INT 0xFF02
#define CAST_OBJ 0xFF03
#define CAST_BOO 0xFF04
#define CAST_CHA 0xFF05

/* Additional constants for the offsets of the information stored in nodes with length > 1 */

#define AND_NODE_CODELEN_POS 1
#define AND_NODE_N_ITEMS_POS 2
#define AND_NODE_FLAGS_POS 3
#define AND_NODE_NKEYS_POS 4
#define AND_NODE_MEM_START_POS 5

#define WAND_NODE_WTIME_POS 5
#define WAND_NODE_MEM_START_POS 6

#define LEN_AND_NODE 7
#define LEN_WAND_NODE 8

#define SET_NODE_MEM_POS 1
#define SET_NODE_CODELEN_POS 2
#define SET_NODE_N_ITEMS_POS 3
#define SET_NODE_FIRST_ITEM_POS 4
#define SET_NODE_PATT_MASK 5 // 4 positions = 128 patterns / 32 bits per long
#define LEN_SET_NODE 9

#define LEN_TIMER_NODE 3
#define TIMER_NODE_WINDOW_POS 1
#define TIMER_NODE_MEM_POS 2

#define PROD_NODE_RULENAME_POS 1
#define PROD_NODE_RULESETNM_POS 2
#define PROD_NODE_RULECAT_POS 3
#define PROD_NODE_NOBJIMP_POS 4
#define PROD_NODE_MEM_POS 5   // there are always memory (used only when implied objects)
#define PROD_NODE_FLAGS_POS 6 // FLAGS of PROD_NODE (is TRIGGER ?)
#define LEN_BASIC_PROD_NODE 7

/* ... If the number of implied objects is not 0, the position of them */
#define PROD_START_OBJ_POS 7

#endif
