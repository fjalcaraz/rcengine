/**
 * @file patterns.hpp
 * @author Francisco ALcaraz
 * @brief Class Pattern definition
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

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

#ifndef PUBLIC
#define PUBLIC
#define PRIVATE static
#endif

#include "engine.h"

#include "error.hpp"
#include "expr.hpp"
#include "nodes.hpp"

#ifndef PATTERN_HH_INCLUDED
#define PATTERN_HH_INCLUDED

typedef enum
{
  NO_ACCESS,
  AS_SINGLE,
  AS_SET
} AccessMode;

class ObjClass;

class Pattern
{
  private:
    ObjClass *_class;
    StValues _life_st;
    Node *_root;
    AccessMode _access;
    Node *_last_intra;
    Node *_last_intra_set;
    ULong _flags;
    int _is_set;

  public:
    Pattern(ObjClass *class_p, StValues status, Node *root, int create_path);
    Pattern(ObjClass *class_p);
    ~Pattern();

    StValues life_st() { return _life_st; };
    void set_life_st(StValues st) { _life_st = st; };
    inline Node *last_intra();
    int is_negated() { return _life_st == ST_NEGATED; };
    int is_set() { return _is_set; };
    ObjClass *class_of_patt() { return _class; };
    ULong get_flags_of_patt() { return _flags; };

    static Pattern *curr_pattern();
    static void set_curr_pattern(Pattern *patt, AccessMode access);
    static void reset_access();
    static Pattern *get_pattern(int patt_no);
    static void set_pattern(int patt_no);
    ULong setupFlags();
    static void initFlags(ULong value, int from_rule);
    void add_intra_node(int code_len, ULong *code);
    void add_class_check(char *name);
    static void new_inter_assoc(int time);
    void add_to_assoc();
    void add_to_assoc(AccessMode access);
    static void add_code_to_curr_node(int code_len, ULong *code);
    static void insert_code_in_curr_node(int pos,
                                        int code_len);
    static void set_code_in_curr_node(int pos, ULong code);
    static ULong get_code_in_curr_node(int pos);
    static int get_len_of_curr_node();
    void get_inter_pos(int *mem, int *pos);
    static void new_set_context();
    static void add_set_node();
    void assume_equality_in_set(int attr);
    static void end_set_context();
    static Node *add_prod_node();
    int pattern_no();
    static void set_last_intra_set(Node *from, Node *to);
    static void delete_patterns();
    static void free_patterns();

    void print();
};

inline Node *Pattern::last_intra()
{
  if (_access == AS_SINGLE)
    return _last_intra;
  else
    return _last_intra_set;
}

void allow_same_obj();

#endif
