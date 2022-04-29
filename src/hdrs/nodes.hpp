/**
 * @file nodes.hpp
 * @author Francisco Alcaraz
 * @brief Function exported by the nodes module, Class Node and related structs and constants definitions
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



#include <stdio.h>



#include "engine.h"

#include "codes.h"
#include "error.hpp"
#include "expr.hpp"
#include "metaobj.hpp"
#include "status.hpp"
#include "btree.hpp"
#include "dasm_rete.hpp"

#ifndef NODES___HH_INCLUDED
#define NODES___HH_INCLUDED


#define LEFT_MEM  0
#define RIGHT_MEM 1
 
#define INTRA         0
#define INTRA_SET     1
#define INTER_AND     2
#define INTER_NAND    3
#define INTER_OAND    4
#define INTER_PROD    5

#define DATA_STACK_SIZE 50

#define REACHED_BY_RIGHT 128
#define REACHED_BY_LEFT 64
#define IN_RULE_MARK 32
#define IN_PATH_MARK 16
#define INVALID_NODE  8
#define VALID_NODE    4
#define EQ_NODE       2
#define SIMPLE_MARK   1
#define NO_MARK       0
#define ANY_CONTROL_MARK (INVALID_NODE|VALID_NODE|EQ_NODE|SIMPLE_MARK)

#define NODE_REACHED -1

class Node;


struct NodeLink
{
    Node *node;
    int side;
    NodeLink *next;
};

struct NodeIter
{
    NodeLink *curr;
    NodeLink *next;
};


struct ExecData
{
    Status *st;
    MetaObj *left;
    MetaObj *right;
    int tag;
    int side;
    int pos;

    inline ExecData(Status &St, MetaObj *Left, MetaObj *Right, int Tag, int Side, int Pos)
    {
        st = &St;
        left = Left;
        right = Right;
        tag = Tag;
        side = Side;
        pos = Pos;
    };
};

struct MatchCount
{
    MetaObj *item;
    int count;

    MatchCount(MetaObj *it) { count=0; item=it; };
};

struct ItemOut
{
    void *item;
    ItemOut *next;
};



class Node
{
   private :

     int 	_type;
     ULong 	*_code;
     int 	_lcode;
     int	_n_items_l;
     int 	_n_items_r;
     int 	_curr_pos;
     int	_mark;
     int        _n_paths;
     Node       *_eq_node_left;
     Node       *_eq_node_right;
     Node 	*_parent_left;
     Node 	*_parent_right;
     NodeLink 	*_fork;

     static Value data_stack[DATA_STACK_SIZE]; /* Stack de datos              */
     static Value *dstack_p;                   /* Puntero al stack de datos   */
     static BTree conflict_set_mem[3];          /* Memorias para los CSET      */
     static int cmp_result;                    /* Para relaciones de orden en conj */
     static int checkingScope;                 /* Flag de exploracion en modificaciones */
     static ULong *code_p;                     /* posicion de ejecucion       */


   public :

     typedef int (*IntFunction)(Node *node, ExecData &data);

     Node(int type, int lcode, ULong *codes);
     Node(Node const &copia);
     ~Node();

     int        type()				{ return _type; };
     int        is_inter()   { return ((_type == INTER_AND) || (_type == INTER_NAND) || (_type == INTER_OAND)); };
     int        is_inter_sym()  { return (_type == INTER_AND); };
     int        is_inter_asym() { return ((_type == INTER_NAND) || (_type == INTER_OAND)); };
     int        is_inter_w() { ULong code = dasm_code(_code[0]);
                               return ((code == WAND) || (code == NWAND) || (code == OWAND)); };

     int        has_mark(int mark) { return (_mark & mark); };
     void       set_mark(int mark) { _mark |= mark; };
     void       reset_mark(int mark) { _mark &= ~mark; };

     Node * 	parent_node(int side);
     Node * 	first_child(int &fork_side_conn, NodeIter &iter);
     Node * 	current_child(int &fork_side_conn, NodeIter &iter);
     Node * 	next_child(int &fork_side_conn, NodeIter &iter);
     Node *     last_child();
     void 	connect_node(Node *parent, int side);
     void 	disconnect_node(Node *parent, int side);
     void 	update_n_items(int side);

     int        len()                           { return _lcode; };
     int        offset(ULong *code)             { return code - _code; };
     void       set_code(int pos, ULong value)	{ _code[pos] = value; }
     ULong      get_code(int pos)		{ return _code[pos]; }
     void 	add_code(int code_len, ULong *code, int check_set_node = FALSE);
     void       add_code(Node *source, int check_set_node = FALSE);
     ULong *    getInterKeysInCode(ULong *code, int code_len, int *nkey);
     void       insert_code(int pos, int code_len);
     int 	eq_code(Node const * const node);
     ULong & 	code(int n)			{ return _code[n]; };

     void       optimize_connection(Node *node);
     int        try_to_join(Node *curr_parent, Node *node, Node *first_eq);
     void       move_up_to(Node *parent);
     int        can_reach(Node *target, int paths);
     void       move_up(Node *root);
     void       ungroup_parent_node();
     void       join_node(Node *equal_node);
     Node *     find_eq_node(Node *muestra);
     void       optimize_connection_down();

     Node * 	last_intra_node();
     Node * 	last_inter_node(int only_and_nodes, int init = TRUE);
     Node *     last_inter_node_ussing(int init = TRUE);
     void	set_path_down(int init = TRUE);
     void       mark_as_invalid(int init = TRUE);
     void       delete_mark_as_invalid(int init = TRUE);

     void	mark_path_down(Node *target);
     int        can_go_down(Node* node, int side, int init = TRUE);
     void 	clear_path();
     Node *     new_path();
     void 	clear_rule();
     void 	set_obj_pos(int pos = 0);
     void 	get_obj_pos(int *side, int *pos);
     void 	get_obj_pos_after(int *pos);
     Node * 	find_node_in_path(int type, int init = TRUE);
     Node * 	insert_intra_node(Node **last_intra, Node *real_root,
                                  int code_len, ULong *list_codes);
     void       insert_class_check(Node **last_intra, char *class_name, 
                                   int own_check);
     static void new_inter_assoc(int time);
     Node * 	add_to_inter_assoc(Node *last_intra_set, StValues st_this, ULong flags_this);
     static Node * create_AND_node(int type, ULong l_flags, ULong r_flags, Node *p_left, Node *p_right);
     static Node * create_SET_node(int first_pos, int n_objs);
     Node *     create_TIMER_node(Node **last_intra, Node *real_root, int window);

     void       move_down_asym_children(Node *new_parent, int side);
     void	shift_code(int side, int begin_shift, int how_many);
     void       move_code_mem(int side, int new_pos);
     void       optimize_asym_children();
     int        in_asym_left_chain(Node *node1, Node *node2);
     void       move_down_set_nodes();
     Node *     last_in_INTRA_SET_chain();
     void       delete_AND_shortcut(Node *set_node_parent);
     int        double_path(int side);

     Node *     insert_prod_node();

     /* EJECUCION */
     void	propagate_modify(ExecData &data, ULong *codep = NULL);
     int 	propagate(ExecData &data, ULong *codep = NULL);
     void	propagate_reached_nodes(ExecData &data, ULong *codep=NULL);

     static void 	setChecking(int value);
     int 	execute_code(ExecData &data, ULong *codep);

     static int 	user_func_call(Node *node, ExecData &data);
     static int 	user_proc_call(Node *node, ExecData &data);
     static int 	test_class_call(Node *node, ExecData &data);
     static int 	tnsobj_call(Node *node, ExecData &data);
     static int 	ttrue_call(Node *node, ExecData &data);
     static int 	tfalse_call(Node *node, ExecData &data);
     static int 	eval_call(Node *node, ExecData &data);
     static int 	not_call(Node *node, ExecData &data);
     static int 	addn_call(Node *node, ExecData &data);
     static int 	addf_call(Node *node, ExecData &data);
     static int 	subn_call(Node *node, ExecData &data);
     static int 	subf_call(Node *node, ExecData &data);
     static int 	muln_call(Node *node, ExecData &data);
     static int 	mulf_call(Node *node, ExecData &data);
     static int 	divn_call(Node *node, ExecData &data);
     static int 	divf_call(Node *node, ExecData &data);
     static int 	minusn_call(Node *node, ExecData &data);
     static int 	minusf_call(Node *node, ExecData &data);
     static int 	teqa_call(Node *node, ExecData &data);
     static int 	tnea_call(Node *node, ExecData &data);
     static int 	tlta_call(Node *node, ExecData &data);
     static int 	tlea_call(Node *node, ExecData &data);
     static int 	tgea_call(Node *node, ExecData &data);
     static int 	tgta_call(Node *node, ExecData &data);
     static int 	cmpa_call(Node *node, ExecData &data);
     static int 	teqn_call(Node *node, ExecData &data);
     static int 	tnen_call(Node *node, ExecData &data);
     static int 	tltn_call(Node *node, ExecData &data);
     static int 	tlen_call(Node *node, ExecData &data);
     static int 	tgen_call(Node *node, ExecData &data);
     static int 	tgtn_call(Node *node, ExecData &data);
     static int 	cmpn_call(Node *node, ExecData &data);
     static int 	teqf_call(Node *node, ExecData &data);
     static int 	tnef_call(Node *node, ExecData &data);
     static int 	tltf_call(Node *node, ExecData &data);
     static int 	tlef_call(Node *node, ExecData &data);
     static int 	tgef_call(Node *node, ExecData &data);
     static int 	tgtf_call(Node *node, ExecData &data);
     static int 	cmpf_call(Node *node, ExecData &data);
     static int 	sumsn_call(Node *node, ExecData &data);
     static int 	sumsf_call(Node *node, ExecData &data);
     static int 	prdsn_call(Node *node, ExecData &data);
     static int 	prdsf_call(Node *node, ExecData &data);
     static int 	minsn_call(Node *node, ExecData &data);
     static int 	minsf_call(Node *node, ExecData &data);
     static int 	minsa_call(Node *node, ExecData &data);
     static int 	maxsn_call(Node *node, ExecData &data);
     static int 	maxsf_call(Node *node, ExecData &data);
     static int 	maxsa_call(Node *node, ExecData &data);
     static int 	count_call(Node *node, ExecData &data);
     static int 	concat_call(Node *node, ExecData &data);
     static int 	pusha_call(Node *node, ExecData &data);
     static int 	push_call(Node *node, ExecData &data);
     static int 	pushs_call(Node *node, ExecData &data);
     static int 	pusho_call(Node *node, ExecData &data);
     static int 	pusht_call(Node *node, ExecData &data);
     static int 	tor_call(Node *node, ExecData &data);
     static int 	and_call(Node *node, ExecData &data);
     void 	and_call_by_left(ExecData &data);
     void 	and_call_by_right(ExecData &data);
     void 	check_and_cond(MetaObj *item, ExecData &data);
     static int 	nand_call(Node *node, ExecData &data);
     void 	nand_call_by_left(ExecData &data);
     void 	nand_call_by_right(ExecData &data);
     void 	check_nand_cond(MatchCount *left, MetaObj *right, ExecData &data, int *result_tag);
     static int 	oand_call(Node *node, ExecData &data);
     void 	oand_call_by_left(ExecData &data);
     void 	oand_call_by_right(ExecData &data);
     void 	check_oand_cond(MatchCount *counter, MetaObj *item, ExecData &data,  int *how_many);

     int 	store_in_and_node_by_LEFT(ExecData &data);
     int 	store_in_and_node_by_RIGHT(ExecData &data);
     int 	store_in_asym_node_by_LEFT(ExecData &data, MatchCount *&count);
     static int 	wand_call(Node *node, ExecData &data);
     void 	wand_call_by_left(ExecData &data);
     void 	wand_call_by_right(ExecData &data);
     void 	check_wand_cond(MetaObj *item, ExecData &data);
     static int 	nwand_call(Node *node, ExecData &data);
     void 	nwand_call_by_left(ExecData &data);
     void 	nwand_call_by_right(ExecData &data);
     void 	check_nwand_cond(MatchCount *counter, MetaObj *item, ExecData &data, int *result_tag);
     static int 	owand_call(Node *node, ExecData &data);
     void 	owand_call_by_left(ExecData &data);
     void 	owand_call_by_right(ExecData &data);
     void 	check_owand_cond(MatchCount *counter, MetaObj *item, ExecData &data,  int *how_many);
     int 	store_in_wand_node_by_LEFT(ExecData &data);
     int 	store_in_wand_node_by_RIGHT(ExecData &data);
     int 	store_in_Wasym_node_by_LEFT(ExecData &data, MatchCount *&count);
	 void 	push_to_remove_old_item(Single *item, int side);
     static int 	set_call(Node *node, ExecData &data);
     void 	set_call_INSERT(ExecData &data, MetaObj **LeftItemInMem_p=NULL);
     void 	set_call_RETRACT(ExecData &data, MetaObj *LeftItemInMem=NULL);
     void 	set_call_MODIFY(ExecData &data);
     static int 	timer_call(Node *node, ExecData &data);
     int 	timer_refresh(ULong *codep, long time_mark, bool extern_refresh);
     static int 	prod_call(Node *node, ExecData &data);
     static int 	run_cs();
     void 	perform_execution(int tag, Compound *ProdCompound);
     static int 	new_call(Node *node, ExecData &data);
     static int 	mod_call(Node *node, ExecData &data);
     static int 	mod_wp_call(Node *node, ExecData &data);
     static int 	del_call(Node *node, ExecData &data);
     static int 	objimp_call(Node *node, ExecData &data);
     static int 	endrule_call(Node *node, ExecData &data);
     static int        popsa_call(Node *node, ExecData &data);
     static int        pops_call(Node *node, ExecData &data);

     static int 	execute_cmp(ULong *code, ULong *end_of_code, MetaObj *left, MetaObj *right);
     static int 	cmp_count_with_object(const void *c_obj1, const void *c_obj2, va_list list);
     static int 	cmp_count_with_object_t(const void *c_obj1, const void *c_obj2, va_list list);
     static int 	cmp_count_with_object_tw(const void *c_obj1, const void *c_obj2, va_list list);

     static void    free_rule(void *node, va_list);
     void	free_nodes_up(int en_intra);
     void	free_from_last_intra();
     void       free_no_intra();


     static void    reset_rule(const void *node, va_list);
     void	reset_nodes_up();

     void 	print(const char *prefix, int child_chain = TRUE);
     void       print_code();
     void       print_parents();

     static void setPatternMask(ULong *begin_mask, int n);
     static bool getPatternMask(ULong *begin_mask, int n);
     static void printPatternMask(ULong *begin_mask);

	 void 	mark_set_positions(int pos, Node *last_intra_set);

};


#endif
