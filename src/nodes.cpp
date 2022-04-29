/**
 * @file nodes.cpp
 * @author Francisco Alcaraz
 * @brief Node class methods oriented to compilation and net formation
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"

#include "codes.h"
#include "dasm_rete.hpp"
#include "patterns.hpp"
#include "nodes.hpp"
#include "rules.hpp"
#include "classes.hpp"
#include "load.hpp"
#include "eng.hpp"
#include "status.hpp"

struct Context
{
   ULong flags;
   StValues status;
   Node *last_intra;
   Node *last_intra_set;
   Context *next;
 
   Context(ULong Flags, StValues Status, Node *Last_Intra, Node *Last_Intra_Set)
   {
     flags=Flags; status=Status; last_intra=Last_Intra; last_intra_set=Last_Intra_Set; next = NULL;
   }
};
 
static Node *_common_node_of_assoc = NULL;
static int _curr_window_time;
Context *_assoc=NULL;
Node *_curr_node = NULL;


/**
 * @brief Construct a new Node:: Node object
 * 
 * @param type Type of Node
 * @param lcode Length of Code
 * @param codes Code array
 */
Node::Node(int type, int lcode, ULong *codes)
{

   _type 	    = type;
   _lcode	    = 0;
   _code	    = NULL;
   _n_items_l   = 1;		// Ready to work as root
   _n_items_r   = 0;
   _curr_pos    = 0;
   _mark        = (IN_RULE_MARK | IN_PATH_MARK);
   _n_paths     = 1;
   _eq_node_left  = NULL;
   _eq_node_right = NULL;
   _fork	    = NULL;
   _parent_left   = NULL;
   _parent_right  = NULL;

   add_code(lcode, codes);

}

/**
 * @brief Construct a new Node:: Node object. Copy Constructor
 * 
 * @param copy The node to copy from
 */
Node::Node(Node const &copy)
{
    int n;
    Node *eq;

    _type        = copy._type;
    _lcode       = copy._lcode;
    _n_items_l   = copy._n_items_l;
    _n_items_r   = copy._n_items_r;
    _curr_pos    = copy._curr_pos;
    _mark        = (IN_RULE_MARK | IN_PATH_MARK);
    _n_paths     = 1;
    _fork          = NULL;  // Will control all the children node. Are structs with { side, node and next }
    _parent_left   = NULL;
    _parent_right  = NULL;

    _code = (ULong *)malloc(_lcode * sizeof(ULong));

    for (n = 0; n<_lcode; n++)
        _code[n] = copy._code[n];

    // We insert in the list in the first position to not interfere in the walk through

    for (eq=(Node*)&copy; eq->_eq_node_right!= NULL; eq = eq->_eq_node_right);

    eq->_eq_node_right = this;
    _eq_node_right  = NULL;
    _eq_node_left = eq;

    if (copy._parent_left!= NULL)
        connect_node(copy._parent_left, LEFT_MEM);

    if (copy._parent_right!= NULL)
        connect_node(copy._parent_right, RIGHT_MEM);
}

/**
 * @brief Destroy the Node:: Node object
 * 
 */
Node::~Node()
{

    // The children are inherithed by the parent by LEFT

    Node *child, *p_node;
    int side;
    NodeIter iterator;

    p_node = parent_node(LEFT_MEM);
    disconnect_node(parent_node(LEFT_MEM), LEFT_MEM);
    disconnect_node(parent_node(RIGHT_MEM), RIGHT_MEM);

    if (p_node != NULL)
    {
        for (child = first_child(side, iterator);
                child != NULL;
                child = next_child(side, iterator))
        {
            // If the children are own, move them to the parent by left
            // The children nodes of GOTO nodes are not own, in this case, jus disconnect them

            if (child->parent_node(side) == this)
            {
                child->disconnect_node(this, side);
                child->connect_node(p_node, side);
            }
            else
            {
                child->disconnect_node(this, side);
            }
        }
    }

    if (_eq_node_left != NULL)
        _eq_node_left -> _eq_node_right = _eq_node_right;

    if (_eq_node_right != NULL)
        _eq_node_right -> _eq_node_left = _eq_node_left;

    if (_code != NULL)
        reset_code(_code, _lcode, TRUE);
}

//
// METHODS RELATED TO PANTENT AND CHILD RELATION
//

/**
 * @brief Returns the parent node of a node by a given side
 * 
 * @param side side of the parent
 * @return Node* The parent by this side
 */
Node *
Node::parent_node(int side)
{
    if (side == LEFT_MEM)
        return _parent_left;
    else
        return _parent_right;
}

/**
 * @brief Initialize an iterator to walk over the children nodes and returns the 
 *        first child and the side of the connection in fork_side_conn
 * 
 * @param fork_side_conn Where to store the side where this node enters in this first child
 * @param iter The iterator
 * @return Node* The first node (or NULL)
 */
Node *
Node::first_child(int &fork_side_conn, NodeIter &iter)
{
    iter.curr = _fork;

    if (_fork != NULL)
        iter.next = _fork->next;
    else
        iter.next = NULL;

    return current_child(fork_side_conn, iter);
}

/**
 * @brief Get the next child in the Iterator
 * 
 * @param fork_side_conn Where to store the side where this node enters in this child
 * @param iter The iterator
 * @return Node* The first node (or NULL)
 */
Node *
Node::next_child(int &fork_side_conn, NodeIter &iter)
{

    iter.curr = iter.next;

    if ( iter.next != NULL)
    {
        iter.next = iter.next->next;
    }

    return current_child(fork_side_conn, iter);
}

/**
 * @brief Gets the last child of a Node
 * 
 * @return Node* The last node
 */
Node *
Node::last_child()
{
    NodeLink *last;

    if (_fork == NULL)
        return NULL;

    else
    {
        for (last=_fork; last->next != NULL; last = last->next);
        return last->node;
    }
}

/**
 * @brief Get the side
 * 
 * @param fork_side_conn Where to store the side where this node enters in this child
 * @param iter The Iterator
 * @return Node* the current child
 */
Node *
Node::current_child(int &fork_side_conn, NodeIter &iter)
{

    if ( iter.curr == NULL)
        return NULL;
    else
    {
        fork_side_conn = iter.curr->side;
        return iter.curr->node;
    }
}

/**
 * @brief Connects this node as a new child of a given parent Node and entering by a side
 * 
 * @param parent The parent node
 * @param side the side how the parent enters in this
 */
void
Node::connect_node(Node *parent, int side)
{
    NodeLink **pos_ins;
    int has_children;

    has_children = (parent->_fork != NULL);

    for (pos_ins = &(parent->_fork);
            (*pos_ins)!= NULL;
            pos_ins = &((*pos_ins)->next));

    (*pos_ins) = new NodeLink;
    (*pos_ins)->node = this;
    (*pos_ins)->side = side;
    (*pos_ins)->next = NULL;

    if (side == LEFT_MEM && _parent_left==NULL)
        _parent_left = parent;

    if (side == RIGHT_MEM && _parent_right==NULL)
        _parent_right = parent;

    update_n_items(side);

    if (parent->_type == INTRA && has_children)
    {
        while(parent != NULL)
        {
            parent->_n_paths += _n_paths;
            parent = parent-> parent_node(LEFT_MEM);
        }
    }
}

/**
 * @brief Disconnect this node as child of a given parent Node and entering by a side
 * 
 * @param parent The parent node
 * @param side the side how the parent enters in this
 */
void 
Node::disconnect_node(Node *parent, int side)
{
    NodeLink **pos_ins, *next_node;

    if (parent == NULL)
        return;

    if (parent == parent_node(side))
    {
        if (side == LEFT_MEM)
            _parent_left = NULL;
        else
            _parent_right = NULL;
    }

    for (pos_ins = &(parent->_fork); 
            (*pos_ins)!= NULL && 
            ((*pos_ins)->node != this || (*pos_ins)->side != side); 
            pos_ins = &((*pos_ins)->next));

    if ((*pos_ins) != NULL)
    {
        next_node = (*pos_ins)->next;

        delete (*pos_ins);
        (*pos_ins) = next_node;

        if (parent->_type == INTRA && parent->_n_paths>1)
        {
            while(parent != NULL)
            {
                parent->_n_paths -= _n_paths;
                parent = parent-> parent_node(LEFT_MEM);
            }
        }
    }

}

/**
 * @brief Update the number of items that come to the node by a side. This must be done when moving nodes
 * 
 * @param side 
 */
void
Node::update_n_items(int side)
{
    int n_items;
    Node *parent;
    Node *child;
    int child_side;
    NodeIter iterator;

    parent = parent_node(side);
    if (parent->_type == INTER_NAND)
        n_items = parent->_n_items_l;
    else
        n_items = parent->_n_items_l + parent->_n_items_r;

    if (side == LEFT_MEM)
        _n_items_l = n_items;
    else
        _n_items_r = n_items;

    if ( is_inter() )
        _code[AND_NODE_N_ITEMS_POS] = ((_n_items_l<<16) | (_n_items_r & 0xFF));

    for (child = first_child(child_side, iterator);
            child != NULL;
            child = child->next_child(child_side, iterator))
    {
        child->update_n_items(child_side);
    }
}

/**
 * @brief Trace using the attribute _curr_pos the position of an object in this node and in its children
 *      Positions over _n_items_l are right site positions
 * 
 * @param pos 
 */
void
Node::set_obj_pos(int pos)
{
    Node *child;
    int child_side;
    NodeIter iterator;

    _curr_pos = pos;

    for (child =  first_child(child_side, iterator);
            child != NULL;
            child = child->next_child(child_side, iterator))
    {
        if (child->has_mark(IN_RULE_MARK))
        {
            if (child_side == LEFT_MEM)
                child->set_obj_pos(pos);
            else
                child->set_obj_pos(pos + child->_n_items_l);
        }
    }
}

/**
 * @brief Gets position and the side or the traced object in this node
 * 
 * @param side 
 * @param pos 
 */
void
Node::get_obj_pos(int *side, int *pos)
{
    if (_curr_pos >= _n_items_l)
    {
        (*side) = RIGHT_MEM;
        (*pos) =  _curr_pos - _n_items_l;
    }
    else
    {
        (*side) = LEFT_MEM;
        (*pos) =  _curr_pos;
    }
}

/**
 * @brief Get the position after the node
 *        This is _curr_pos due it is n_item_left + n_item_right
 * 
 * @param pos 
 */
void
Node::get_obj_pos_after(int *pos)
{
    // cur_pos is the offset of the traced object when the compound tuple goes to the children node
    *pos = _curr_pos;
}

//
// NODE CODE MANAGEMENT
//

/**
 * @brief add a code of length code_len to this node
 * 
 * @param code_len Length of code
 * @param code The code array
 * @param check_set_node If this code is internal to SET to order the elements (code based ordering)
 */
void
Node::add_code(int code_len, ULong *code, int check_set_node)
{
   
    if (code_len>0)
    {
        int copy_offset=0;
        if (is_inter() && _lcode > AND_NODE_CODELEN_POS)
        {
            int nkeys;
            ULong *keysInfo;
            keysInfo = getInterKeysInCode(code, code_len, &nkeys);
            int len_and = (is_inter_w() ? LEN_WAND_NODE : LEN_AND_NODE);

            if (nkeys>0)
            {
                int inc_len = nkeys + code_len - (5 * nkeys);
                _code = (ULong *)realloc((void*)(_code), (_lcode+inc_len) * sizeof(ULong));
                
                ULong *lastKeyOffset = _code+ len_and + _code[AND_NODE_NKEYS_POS];
                if (_lcode - len_and -_code[AND_NODE_NKEYS_POS])
                    memmove(lastKeyOffset+nkeys, lastKeyOffset, (_lcode - len_and -_code[AND_NODE_NKEYS_POS])*sizeof(ULong));
                memcpy(lastKeyOffset, keysInfo, nkeys*sizeof(ULong));
                _code[AND_NODE_NKEYS_POS] += nkeys;

                if (code_len - (5 * nkeys))
                {
                    memcpy(_code + _lcode + nkeys, code + (5 * nkeys), code_len - (5 * nkeys));
                    _code[AND_NODE_CODELEN_POS] += code_len - (5 * nkeys);
                    load_code(_code + _lcode+ nkeys, code_len - (5 * nkeys), this);
                }
                _lcode += inc_len;

                return;
            }

            _code[AND_NODE_CODELEN_POS] += code_len;
        }

        if (check_set_node && _type == INTRA_SET && _lcode > SET_NODE_CODELEN_POS)
            _code[SET_NODE_CODELEN_POS] += code_len;

        _code = (ULong *)realloc((void*)(_code), (_lcode+code_len) * sizeof(ULong));

        memcpy(_code + _lcode, code, code_len * sizeof(ULong));
        load_code(_code + _lcode, code_len, this);
        _lcode += code_len;
    }
}

/**
 * @brief add to this node the code from another node
 * 
 * @param source The node where to take the code form
 * @param check_set_node If this code is internal to SET to order the elements (code based ordering)
 */
void
Node::add_code(Node *source, int check_set_node)
{
    ULong *src_code;
    int src_len;

    src_code=source->_code;
    src_len =source->_lcode;

    if (source->is_inter())
    {
        if (source->is_inter_w())
        {
            src_code += LEN_WAND_NODE;
            src_len  -= LEN_WAND_NODE;
        }
        else
        {
            src_code += LEN_AND_NODE;
            src_len  -= LEN_AND_NODE;
        }
    }

    if (is_inter() && _lcode > AND_NODE_CODELEN_POS)
        _code[AND_NODE_CODELEN_POS] += src_len;

    if (check_set_node && _type == INTRA_SET && _lcode > SET_NODE_CODELEN_POS)
        _code[SET_NODE_CODELEN_POS] += src_len;

    _code = (ULong *)realloc((void *)(_code), 
            (_lcode+src_len) * sizeof(ULong));

    memcpy(_code + _lcode, src_code, src_len * sizeof(ULong));
    _lcode += src_len;
}

/**
 * @brief Insert some position of code and set them to 0, starting from a given position 
 * 
 * @param pos Initial position
 * @param code_len How many positions
 */
void
Node::insert_code(int pos, int code_len)
{
    int n;

    if (code_len>0)
    {
        _code = (ULong *)realloc((void*)(_code), (_lcode+code_len) * sizeof(ULong));

        memmove(_code+pos+code_len, _code+pos, (_lcode-pos) * sizeof(ULong)); 

        for (n = 0; n<code_len; n++)
            _code[pos+n] = 0L;

        _lcode += code_len;

        if (is_inter() && _lcode > AND_NODE_CODELEN_POS)
            _code[AND_NODE_CODELEN_POS] += code_len;
    }
}


/**
 * @brief Determine if two nodes have the same code
 * 
 * @param node The other node 
 * @return int TRUE/FALSE
 */
int
Node::eq_code(Node const * const node)
{
    int n;

    if (node == this)
        return TRUE;

    if (node == NULL || this == NULL)
        return FALSE;

    if (_lcode != node->_lcode)
        return FALSE;

    for (n=0; n<_lcode && _code[n] == node->_code[n]; n++);

    return (n ==_lcode);
}



//
// During compilation all INTRA conditions are added forming a tree alike structure of nodes where each branch is the
// chaining of filters of a rule (they are indeed a logical AND of conditions)
// This structure of tree is optimized due the most common conditions are going to move up in the tree and,
// if possible, their branches are merged.
// During compilation of a rule, a control is done for every object pattern in the LHS, about what is his last INTRA node;
// the branch that is being made in the tree taking all the conditions of this pattern

//
// FUNCTION RELATED TO LAST INTRA NODE MANAGEMENT
//

/**
 * @brief Return the last intra node, the last in the intra conditions chain
 * 
 * @return Node* 
 */
Node *
Node::last_intra_node()
{
    Node *node;
    int side;
    NodeIter iterator;

    // The intra branch followed is marked with IN_PATH_MARK
    
    for (node = this->first_child(side, iterator); 
            node != NULL && (!node->has_mark(IN_PATH_MARK) || node->_type != INTRA);
            node = this->next_child(side, iterator));

    if (node == NULL)   // No children, so this is the last
        return this;
    else
        return node->last_intra_node();
}

/**
 * @brief Return the last INTER node where the object is present. 
 *        Asymmetric INTER nodes by right cannot be followed due the object may not be available
 *        SET nodes also cannot be passed by
 *        This method should be called from the last intra node
 * 
 * @param only_and_nodes: Only AND nodes are accepted 
 * @param init Will make can_go_down to consider the item position is 0 (init the position)
 * @return Node* 
 */
Node *
Node::last_inter_node(int only_and_nodes, int init)
{
    Node *node;
    int side;
    NodeIter iterator;

    for (node = this->first_child(side, iterator);
            node != NULL && 
            ( !node->has_mark(IN_RULE_MARK) ||
              node->has_mark(INVALID_NODE) ||
              (only_and_nodes && node->_type != INTER_AND && node->_type != INTRA_SET) ||
              !can_go_down(node, side, init));
            node = this->next_child(side, iterator));

    if (node == NULL)                            // It is the last node
        return this;
    else
        return node->last_inter_node(only_and_nodes, FALSE);
}

/**
 * @brief This function returns the last INTER node using a object. 
 *        It can return by example a NAND node by right (the previous won't do it)
 * 
 * @param init Will make can_go_down to consider the item position is 0 (init the position)
 * @return Node* 
 */
Node *
Node::last_inter_node_ussing(int init)
{
    Node *node, *no_output_node;
    int side;
    NodeIter iterator;

    no_output_node = NULL;
    for (node = this->first_child(side, iterator);
            node != NULL && (!node->has_mark(IN_RULE_MARK) || !can_go_down(node, side, init));
            node = this->next_child(side, iterator))
    {
        if (node->has_mark(IN_RULE_MARK) && !can_go_down(node, side, init))
            no_output_node = node;
    }

    if (node == NULL)                            // It is the last node
    {
        if (no_output_node != NULL)
            return no_output_node;
        else
            return this;
    }
    else
    {
        return node->last_inter_node_ussing(FALSE);
    }
}

/**
 * @brief This function determines if an object coming to a node bt some side will be still present
 *        in the tuples that go deeper in the Nodes Net
 * 
 * @param node a child of this
 * @param side entering side
 * @param init if the positions of the objects is set to 0 or maintained
 * @return int TRUE/FALSE
 */
int 
Node::can_go_down(Node *node, int side, int init)
{

    static int init_intra_set;
    static ULong *intra_set_code;
    static Node *prev_node = NULL;
    static ULong prev_pos = 0;
    static ULong pos;
    int is_converted_to_set;


    if (init)  // Initially is determined if we are in a INTRA o INTRA-SET node
    {

        if (_type != INTRA_SET)
        {
            pos=0;  // normal INTRA node
            init_intra_set = FALSE;
        }
        else
        {
            init_intra_set = TRUE;
        }
    }

    // Position tracking
    if (!init_intra_set)
    {
        // The object IS NOT A SET
        if (!init && this == prev_node)
            pos = prev_pos;
        else
        {
            prev_node=this;
            prev_pos = pos;
        }

        if (side == RIGHT_MEM)
            pos += ((node->_n_items_l) << 8);

        if (node->_type == INTRA_SET)
        {
            // see if the object is what is converted to a SET
            is_converted_to_set = (pos >= node->_code[SET_NODE_FIRST_ITEM_POS] && 
                    pos < node->_code[SET_NODE_FIRST_ITEM_POS] + 
                    node->_code[SET_NODE_N_ITEMS_POS]);
        }

    }

    return (node->has_mark(IN_RULE_MARK) &&
            ((node->_type == INTRA_SET && (init_intra_set || !is_converted_to_set)) ||
             (node->is_inter_sym() || (node->is_inter_asym() && side == LEFT_MEM))));
}


//
// PATHS and MARKS
//

/**
 * @brief Clean previous paths an creates a new INTRA node (first node of an object class) 
 * 
 * @return Node* 
 */
Node *
Node::new_path()
{
    Node *node;

    clear_path();
    node = new Node(INTRA, 0,0);
    node->connect_node(this, LEFT_MEM);
    return node;
}

/**
 * @brief Mark the children where the object is present (IN_PATH_MARK), and belonging to the rule
 *        with the mark SIMPLE_MARK until the node target is reached
 * 
 * @param target Final node
 */
void
Node::mark_path_down(Node *target)
{
    Node *node;
    int side;
    NodeIter iterator;

    set_mark(SIMPLE_MARK);

    if (this == target)
        return;

    for (node = this->first_child(side, iterator);
            node != NULL ;
            node = this->next_child(side, iterator))
    {

        if (node->has_mark(IN_RULE_MARK) && node->has_mark(IN_PATH_MARK) &&
                (node == target || node->is_inter_sym() || side == LEFT_MEM))
        {
            node->mark_path_down(target);
        }
    }
}

/**
 * @brief Set the mark IN_PATH_MARK to all the nodes where the current object is present
 * 
 * @param init. On first call should be TRUE to initialize with the position 0 at left
 *        (this node should be an intra node of current object)
 */
void
Node::set_path_down(int init)
{
    Node *node;
    int side;
    NodeIter iterator;

    if (_type == INTRA_SET)
        set_mark(IN_PATH_MARK);

    for (node = this->first_child(side, iterator);
            node != NULL ;
            node = this->next_child(side, iterator))
    {

        if (node->has_mark(IN_RULE_MARK))
        {
            node->set_mark(IN_PATH_MARK);
            if (can_go_down(node, side, init))
                node->set_path_down(FALSE);
        }
    }
}

/**
 * @brief Set the mark INVALID_NODE to all the nodes where the current object is present
 * 
 * @param init. On first call should be TRUE to initialize with the position 0 at left
 *        (this node should be an intra node of current object)
 */
void
Node::mark_as_invalid(int init)
{
    Node *node;
    int side;
    NodeIter iterator;

    if (_type == INTRA_SET)
        set_mark(INVALID_NODE);

    for (node = this->first_child(side, iterator);
            node != NULL ;
            node = this->next_child(side, iterator))
    {

        if (node->has_mark(IN_RULE_MARK))
        {
            node->set_mark(INVALID_NODE);
            if (can_go_down(node, side, init))
                node->mark_as_invalid(FALSE);
        }
    }
}

/**
 * @brief Reset the mark INVALID_NODE to all the nodes where the current object is present
 * 
 * @param init. On first call should be TRUE to initialize with the position 0 at left
 *        (this node should be an intra node of current object)
 */
void
Node::delete_mark_as_invalid(int init)
{
    Node *node;
    int side;
    NodeIter iterator;

    if (_type == INTRA_SET)
        reset_mark(INVALID_NODE);

    for (node = this->first_child(side, iterator);
            node != NULL ;
            node = this->next_child(side, iterator))
    {

        if (node->has_mark(IN_RULE_MARK))
        {
            node->reset_mark(INVALID_NODE);
            if (can_go_down(node, side, init))
                node->delete_mark_as_invalid(FALSE);
        }
    }
}

/**
 * @brief Clears the marks IN_PATH_MARK and INVALID_NODE in all the nodes of the rule
 * 
 */
void
Node::clear_path()
{
    Node *node;
    int side;
    NodeIter iterator;

    if (_type == INTRA_SET)
        reset_mark(IN_PATH_MARK | INVALID_NODE);

    for (node = this->first_child(side, iterator); 
            node != NULL ;
            node = this->next_child(side, iterator))
    {
        if (node->has_mark(IN_RULE_MARK))
        {
            node->reset_mark(IN_PATH_MARK | INVALID_NODE);
            node->clear_path();
        }
    }
}

/**
 * @brief Clears all the marks (including IN_RULE_MARK) in all the nodes belonging to the rule
 * 
 */
void
Node::clear_rule()
{
    Node *node;
    int side;
    NodeIter iterator;


    for (node = this->first_child(side, iterator);
            node != NULL ;
            node = this->next_child(side, iterator))
    {
        if (node->has_mark(IN_RULE_MARK))
        {
            node->_mark = NO_MARK;
            node->clear_rule();
        }
    }

}

/**
 * @brief Search a node where the object we want to join to is present (whose path is signed)
 *      The node found will arrive the joint node by the left side, and this will arrive by the right 
 * 
 * @param type INTRA or INTER_AND NODE, finding last INTRA, or last INTER if exists (or last INTRA elsewhere)
 * @param init TRUE on first call
 * @return Node* 
 */
Node *
Node::find_node_in_path(int type, int init /* = TRUE */)
{
    Node *node, *found;
    int side;
    NodeIter iterator;


    if (type == INTRA) 
    {
        // The root node will be always in the path so going up we will always find at least the root node in path

        if (this->has_mark(IN_PATH_MARK) && type == this->_type)
            return this;
        else
            return this->parent_node(LEFT_MEM)->
                find_node_in_path(type, FALSE);
    }
    else
    {
        // An INTRA_SET node is Ok
        if (init && has_mark(IN_PATH_MARK) && !has_mark(INVALID_NODE) &&
                (type == INTER_AND || type == INTER_OAND) && _type == INTRA_SET)
            return this;

        // We have to continue while a valid node is not found. This must be in the PATH
        // and if arriving by right side, we dont pass through NAND nodes cause the object
        // is not present any more in the children   
        
        for (node = first_child(side, iterator); 
                node != NULL; 
                node = next_child(side, iterator))
        {
            // TODO: Why the rest requires the side to be RIGHT MEM?
            if (node->has_mark(IN_PATH_MARK) && !node->has_mark(INVALID_NODE) &&
                    (type == node->_type || ((type == INTER_AND || type == INTER_OAND) && node->_type == INTRA_SET)) &&
                    (node->_type==INTER_AND || node->_type==INTRA_SET || side==RIGHT_MEM ))
                return node;

            if (can_go_down(node, side, init))
            {
                found = node->find_node_in_path(type, FALSE);
                if (found != NULL)
                    return found;
            }
        }

        return NULL;
    }
}


//
// GENERATION OF INTRA NODES
// 


/**
 * @brief Insert an INTRA node at last_intra position and move all the INTER nodes children below it
 *      The last_intra node is an EMPTY node that is before any INTER
 *      this is the root of the tree
 * 
 * @param last_intra Position where stored last intra node
 * @param real_root Real root node
 * @param code_len Length of code
 * @param codes Array of code
 * @return Node* Last intra node 
 */
Node *
Node::insert_intra_node(Node **last_intra, Node *real_root, int code_len, ULong *codes) 
{
    Node *node, *equal_node, *inter_node, *new_last_intra;
    int side;
    NodeIter iterator;

    // First we fill the previous last intra with the code
    node = *last_intra;
    node->add_code(code_len, codes);

    if (node->_type == INTRA)
    {
        // Then create a new empty node, connect it as children and make it the new last_intra
        new_last_intra = new Node(INTRA, 0, 0);
        new_last_intra -> connect_node(node, LEFT_MEM);
        *last_intra = new_last_intra;

        // finally we move all the inter node below the new last intra

        for (inter_node = node->first_child(side, iterator);
                inter_node != NULL;
                inter_node = node->next_child(side, iterator))
        {
            if (inter_node->_type != INTRA && inter_node->has_mark(IN_RULE_MARK))
            {
                inter_node->disconnect_node(node, side);
                inter_node->connect_node(new_last_intra, side);
            }
        }

        // The first optimization is to see if that INTRA condition is already in the path

        equal_node = real_root->find_eq_node(node);  

        if (equal_node!= NULL)
        {
            Node *last;

            while (equal_node->_eq_node_left != NULL)
                equal_node = equal_node->_eq_node_left;

            do
            {
                if (node->can_reach(equal_node, -1))
                {
                    delete node;
                    return *last_intra;
                }
                last = equal_node;
                equal_node = equal_node->_eq_node_right;

            } while (equal_node != NULL);
            last->_eq_node_right = node;
            node->_eq_node_left = last;
        }

        // Note that the qual nodes are searched from the real root but the 
        // optimization is made from the this node
        // This node must be the beginning of the subnet below the node with the TCLASS code
        // So inside the INTRA NODES two areas are defined: one on top with the TCLASS checks
        // and the other are with the rest of nodes with intra restrictions
        // The optimization is only done in this second area
        
        this->optimize_connection(node);
    }

    return node;
}

/**
 * @brief Insert a node with a classname check
 *      this must be the root node
 * 
 * @param last_intra Last intra node
 * @param class_name 
 * @param own_check In case of FALSE it joins the rest of the INTRA nodes below last_intra even
 *      if this may represent that that last_intra has two parents by left. 
 *      In case of a subclass the last_intra are the INTRA nodes below the superclass checks
 *      and the class_name will be the subclassname
 *          A1 is a subclass of A
 *                  A A1
 *                   B  (empty node, both joining by left)
 *                   C
 */
void
Node::insert_class_check(Node **last_intra, char *class_name, int own_check)
{
    Node *last, *tclass_node;
    ULong codes[2];
    int no_nodes;

    no_nodes = ((*last_intra) == NULL);

    if (no_nodes || !own_check)
        last = new_path();    // Inserts an empty node hunging from root
    else
        last = (*last_intra); // There are some intra nodes below that class
    codes[0] = TCLASS;
    codes[1] = (ULong)class_name;

    // we add the TCLASS code in the empty node and we create a new empty node below (new last_intra)
    tclass_node = insert_intra_node(&last, this, 2, codes); 

    if (!own_check && !no_nodes)
    {
        // Left delete that new created last node amd make *last_intra
        // the same last_intra node after this new classname check node
        // This is the inheritance among classes taken to the nodes net
        
        delete last;
        (*last_intra)->connect_node(tclass_node, LEFT_MEM);

    }
    else
    {
        (*last_intra)=last;
    }
}


//
// OPTIMIZATION OF INTRA NODES
//

/**
 * @brief Optimize an INTRA node.
 *      just mark all the equal nodes to 'node' with EQ_NODE mark, call to try_to_join() and uncheck them after
 * 
 * @param node 
 */
void
Node::optimize_connection(Node *node)
{

    Node *eq, *first_equal;

    if (node->_eq_node_left != NULL || node->_eq_node_right != NULL)
    {

        // Let's mark all the equal nodes but the current node

        eq = node;
        while (eq->_eq_node_left != NULL)
            eq = eq->_eq_node_left;

        first_equal = eq;
        while (eq != NULL)
        {
            if (eq!=node)  // Current is not marked
                eq->set_mark(EQ_NODE);
            eq = eq->_eq_node_right;
        }

        try_to_join(node->parent_node(LEFT_MEM), node, first_equal);

        // Let's uncheck all the remaining equal nodes

        eq = node;
        while (eq->_eq_node_left != NULL)
            eq = eq->_eq_node_left;

        while (eq!= NULL)
        {
            eq->reset_mark(EQ_NODE|VALID_NODE);
            eq = eq->_eq_node_right;
        }
    }
}

/**
 * @brief Try to find another equal node to "node", going up in the ancestors 
 *          or going down and leave the node more on top in the net
 *          this is the root of the tree
 *      When inserting a new node B', find other equal nodes (B) and if they 
 *      have a common ancestor (A) move it just below that common ancestor and delete the other equal (B')
 *        A                     A
 *      B   C      --->         B
 *      D   B'                D   C
 * 
 * @param curr_parent Ancestor of node
 * @param node the node to optimize 
 * @param first_eq First Equal Node
 * @return int TRUE means no other node capable to join was found
 */
int
Node::try_to_join(Node *curr_parent, Node *node, Node *first_eq)
{
    
    int no_one_could = TRUE;

    // Go up in the hierarchy until reaching root (this)
    // and then go from top to down
    if (this != curr_parent)
        no_one_could = try_to_join(curr_parent->parent_node(LEFT_MEM),
                node, first_eq);

    
    // If was not possible to find a common ancestor at higher levels try at this level now
    if (no_one_could)
    {
        int total_n_paths;
        Node *eq;
        int all_nodes_checked, no_one_can, node_can;

        total_n_paths = node->_n_paths;

        // Let's find those equal nodes to "node" than have as common ancestor to curr_parent
        // and mark them with VALID_NODE mark
        // Remember that "node" is not marked with EQ_NODE
        // Let's add in total_n_paths the total number of paths joined
        // 

        for (eq=first_eq; eq!= NULL; eq=eq->_eq_node_right)
        {

            if (eq->has_mark(EQ_NODE) && eq->can_reach(curr_parent, -1))
            {
                total_n_paths += eq->_n_paths;
                eq->set_mark(VALID_NODE);
            }
            else eq->reset_mark(EQ_NODE);   // Not valid no more

        }

        do {

            all_nodes_checked = TRUE;
            no_one_can = TRUE;
            node_can = FALSE;

            for (eq=first_eq; eq!= NULL; eq=eq->_eq_node_right)
            {
                if (eq->has_mark(VALID_NODE) || eq == node)
                {
                    if (!eq->can_reach(curr_parent, total_n_paths))
                    {
                        eq->reset_mark(VALID_NODE);
                        total_n_paths -= eq->_n_paths;
                        all_nodes_checked = FALSE;
                    }
                    else
                    {
                        if (eq == node)
                            node_can = TRUE;
                        else
                            no_one_can = FALSE;
                    }
                }
            }
        } while (!all_nodes_checked && node_can);

        if (!no_one_can && node_can)
        {
            Node *next;

            node->move_up_to(curr_parent);

            for (eq=first_eq; eq!= NULL; eq=next)
            {
                next = eq->_eq_node_right;
                if (eq->has_mark(VALID_NODE))
                {
                    // eq and node has curr_parent as common ancestor
                    // so move eq just blow curr_parent and join node to it

                    if (! eq->can_reach(node, -1))
                    {
                        eq->move_up_to(curr_parent);
                        node->join_node(eq);
                    }
                }
            }

            return FALSE;     // Was found at this level
        }

        return TRUE;        // Nothing found
    }

    return FALSE;         // It was found at higher levels
}

/**
 * @brief Move this node to be just a direct child of the node "parent"
 *      All the nodes in the middle must be duplicated (ungrouped)
 *          A                          A                    A
 *          B       When compiling     C    it results    C  B        C has scaled in the hierarchy
 *        C   D                                           B  D
 * 
 * @param parent The parent to reach
 */
void
Node::move_up_to(Node *parent)
{
    while (parent_node(LEFT_MEM) != parent)
    {
        move_up(parent);
        if (parent_node(LEFT_MEM) != parent)
            ungroup_parent_node();
    }
}

/**
 * @brief Try to check if climbing the ancestors of this, going by LEFT_MEM, the target node can be reached
 *      also the process may be pruned if path is set to the maximum paths that a parent may have (or -1 if no prune) 
 * 
 * @param target Target node to find
 * @param paths 
 * @return int TRUE or FALSE
 */
int
Node::can_reach(Node *target, int paths)
{
    Node *parent;

    for (parent = parent_node(LEFT_MEM);
            parent != NULL && parent != target;
            parent = parent->parent_node(LEFT_MEM))
    {
        if (paths>0 && parent->_n_paths >= paths)
            return FALSE;
    }

    return (parent == target);
}

/**
 * @brief Move this node up in the hierarchy
 *      All it children (D E) are now of its old parent (B)
 *      The node has now as child the child node (B) where we reached the final_parent node (A)
 *      and node is now a new child of final_parent (A)
 * 
 *        A                                     A
 *      B   F      Moving ahead C up to A     C   F
 *      C                                     B
 *     D E                                   D E
 * 
 * @param final_parent 
 */
void
Node::move_up(Node *final_parent)
{
    Node *old_parent, *parent, *child, *ch;
    int side;
    NodeIter iterator;

    old_parent = parent = parent_node(LEFT_MEM);
    parent->first_child(side, iterator);

    while ( parent->next_child(side, iterator) == NULL &&     // just 1 child
            parent != final_parent)                           // No final_parent
    {
        child = parent;
        parent = parent->parent_node(LEFT_MEM);
        parent->first_child(side, iterator);
    }

    if (parent != old_parent)
    {
        // Lets connect all our children to the previous parent of the node 
        for (ch = first_child(side, iterator);
                ch != NULL;
                ch = next_child(side, iterator))
        {
            ch->disconnect_node(this, LEFT_MEM);
            ch->connect_node(old_parent, LEFT_MEM);
        }
        disconnect_node(old_parent, LEFT_MEM);

        // Left's insert node between final_parent and its child
        child->disconnect_node(parent, LEFT_MEM);
        connect_node(parent, LEFT_MEM);
        child->connect_node(this, LEFT_MEM);
    }
}

/**
 * @brief Create an copy B' of the parent node B with only 'this' node as child
 *      then move 'this' node up over the created copy B'
 * 
 *            A             A
 *            B     -->  B   this
 *        this  F   -->  F   B'
 *        C  D              C D
 */
void
Node::ungroup_parent_node()
{
    Node *p_node;
    Node *child, *child_of_this;
    int side;
    int only_one_child;
    NodeIter iterator;

    p_node = parent_node(LEFT_MEM);
    only_one_child = (p_node->_n_paths == _n_paths);

    for (child = p_node->first_child(side, iterator);
            child != NULL && child != this;
            child = p_node->next_child(side, iterator));

    if (child == this)
    {
        Node *copy;

        if (only_one_child)
            copy = p_node;
        else
            copy = new Node(*p_node);

        // Disconnect this form its parent (B) and connect it to the parent of its parent (A)
        
        disconnect_node(p_node, LEFT_MEM);
        connect_node(p_node->parent_node(LEFT_MEM), LEFT_MEM);

        // The copy (B') inherit all the previous children of this

        for (child_of_this = first_child(side, iterator);
                child_of_this != NULL;
                child_of_this = next_child(side, iterator))
        {
            child_of_this->disconnect_node(this, side);
            child_of_this->connect_node(copy, side);
        }

        // And finally the copy B' is now a child of this
        
        copy->disconnect_node(copy->parent_node(LEFT_MEM), LEFT_MEM);
        copy->connect_node(this, LEFT_MEM);

    }
}

/**
 * @brief Make all the children of equal_node children of this node and then delete equal_node
 * 
 * @param equal_node 
 */
void
Node::join_node(Node *equal_node)
{
    Node *child;
    int side;
    NodeIter iterator;

    // All children of equal node are now children of this

    for (child = equal_node->first_child(side, iterator);
            child != NULL;
            child = equal_node->next_child(side, iterator))
    {
        child->disconnect_node(equal_node, side);
        child->connect_node(this, side);
    }

    // optimize connection from this downwards (taken this as root)
    optimize_connection_down(); 

    // delete equal node
    delete equal_node;
}

/**
 * @brief Find down in the net from this node for an node equal to "model" and return it
 * 
 * @param model 
 * @return Node* 
 */
Node *
Node::find_eq_node(Node *model)
{
    Node *node, *found;
    int conn_side;
    NodeIter iterator;

    for (node = first_child(conn_side, iterator);
            node != NULL;
            node = next_child(conn_side, iterator))
    {
        if (node != model && node->_type == INTRA)
        {
            if (model->eq_code(node))
                return node;
            else
                if (node->_type == INTRA &&
                        (found = node -> find_eq_node(model)) != NULL)
                    return found;
        }
    }
    return NULL;
}

/**
 * @brief Try to optimize down from this node.
 * 
 */
void
Node::optimize_connection_down()
{
    NodeIter iterator;
    Node *node, *last;
    int conn_side;

    last = first_child(conn_side, iterator);

    while (last != NULL)
    {
        optimize_connection(last);
        node = first_child(conn_side, iterator);
        while (node != last)
        {
            node = next_child(conn_side, iterator);
        }
        last = next_child(conn_side, iterator);
    }
}

//
// GENERATION OF INTER NODES
//

/**
 * @brief Remove all the previous assoc and stablish the time window of the rule
 *      The associations are unions of contexts that represent each of the objects in the LHS of rule
 *      When adding a new object to the rule assoc, physically will corresponds to an INTER node that join this
 *      object to the rest of the previous objects in the rule
 * 
 * @param window Window time of the rule
 */
void
Node::new_inter_assoc(int window)
{
    _curr_window_time = window;
    _common_node_of_assoc = NULL;

    while (_assoc != NULL)
    {
        Context *first = _assoc;
        _assoc = _assoc->next;

        if (first->last_intra_set != NULL && first->last_intra != first->last_intra_set)
            first->last_intra_set->delete_mark_as_invalid();

        delete first;
    }
}

/**
 * @brief Add a new object to the assoc what means to insert an INTER node 
 *          below the last_intra and update _common_node_of_assoc
 *          this must be the last_intra node of the object
 * 
 * @param last_intra_set The last intra set in case the object is also ending in a set
 * @param st_this Status of the object ST_NORMAL, ST_NEGATED, ST_OPTIONAL, ST_DELETED
 * @param flags_this Timed flags
 * @return Node* Returns the _common_node_of_assoc (to add code to it)
 */
Node *
Node::add_to_inter_assoc(Node *last_intra_set, StValues st_this, ULong flags_this)
{
    Node *last_inter_right, *last_inter_left, *node;
    int type;
    static Context *main, *other;
    Context * _this, **last;

    // Try to find the object in the assoc
    for (last = &_assoc; *last != NULL && (*last)->last_intra != this; last = &((*last)->next));

    // if the object is already in the assoc, return _common_node_of_assoc
    if (*last != NULL) return _common_node_of_assoc;

    _this = (*last) = new Context(flags_this, st_this, this, last_intra_set);

    if (_this == _assoc)  // The assoc was empty and it is the first object in the assoc
    {
        // Nothing to do cause there is no more that an object
        main = _this; other = NULL;
        _common_node_of_assoc = this;
        return _common_node_of_assoc;
    }

    if (last_intra_set != NULL && last_intra_set!=this)
        last_intra_set->mark_as_invalid();

    // We will maintain to assoc elements: main and other
    //     main will be an affirmative pattern if possible
    //     other is the previous to this
    //  The idea is to join main with this but maintaining the accessibility to other
    //
    // In case of Negated patterns we have to try to maintain only one NAND node as the last node, 
    // to allow the accessibility of all the negated objects al much as possible
    //   (A & !B) & (A & !C) = A & !(B&C)       
    //   (!A & B) & (!A & C) = !A & (B&C)
    //
    // Beware that & (AND) is indeed a Cartesian product filtered, this is not the logical AND
    // This way we are able to maintain the accessibility to A B and C objects in the last NAND node
    //      main   other   _this
    // neg:  AFF   NEG     NEG  -> Two NAND nodes agains main by left side
    //       NEG   AFF     AFF  -> Double NAND against main by right.
    //
    // If the assoc is already done between a main and an other with a NAND
    // We have to join to that with equal sign to _this
    // (M & !O) & !T -> M & !(O & T)  y (M & !O) & T -> (M & T) & !O

    // Case of double negation (M & !O) & !T -> M & !(O & T)
    if (other != NULL &&
            main->status == ST_NORMAL && other->status == ST_NEGATED && _this->status == ST_NEGATED)
    {
        main = other;
    }

    // Let's put in main and other the nodes to join

    // Maintain in "main" the affirmative pattern
    if (main->status != ST_NORMAL && _this->status == ST_NORMAL)
    {
        other = main;
        main = _this;
    }
    else
    {
        // General case : other will be _this
        other = _this;
    }

    if (main->status == ST_OPTIONAL)
        comp_err("Cannot associate optional pattern with non affirmative\n");

    
    if (_common_node_of_assoc == other->last_intra)
        _common_node_of_assoc = main->last_intra;

    if (other->status == main->status)
        type = INTER_AND;
    else
    {
        switch(other->status)
        {
            case ST_NEGATED : type = INTER_NAND; break;
            case ST_OPTIONAL: type = INTER_OAND; break;
        }
    }

    other->last_intra->clear_path();
    main->last_intra->set_path_down();
    main->last_intra->mark_path_down(_common_node_of_assoc);

    if ((node = other->last_intra-> find_node_in_path(type)) == NULL)
    {

        last_inter_left  = main->last_intra ->last_inter_node(type == INTER_AND);
        last_inter_right = other->last_intra->last_inter_node(type == INTER_AND);

        node = create_AND_node(type, main->flags, other->flags, last_inter_left, last_inter_right);
        node -> connect_node(last_inter_left, LEFT_MEM);
        node -> connect_node(last_inter_right, RIGHT_MEM);

        if (type == INTER_AND)
        {
            last_inter_left->move_down_asym_children(node,  LEFT_MEM);
            last_inter_right->move_down_asym_children(node, RIGHT_MEM);
            node->optimize_asym_children();
        }

        node->move_down_set_nodes();
    }

    if (!node->has_mark(ANY_CONTROL_MARK))
        _common_node_of_assoc = node;

    return _common_node_of_assoc;
}     

/**
 * @brief Create an AND node (any variant) and fill it up with code 
 * 
 * @param type Type of node
 * @param l_flags Flags of the left side
 * @param r_flags Flags of the right side
 * @param p_left Parent by left
 * @param p_right Parent by right
 * @return Node* the created node
 */
Node *
Node::create_AND_node(int type, ULong l_flags, ULong r_flags, Node *p_left, Node *p_right)
{

    Node *node;

    if (p_left->is_inter())
    {
        l_flags = p_left->_code[AND_NODE_FLAGS_POS];
        l_flags = ((l_flags >> 16) | l_flags ) & 0xFFFF;
    }

    if (p_right->is_inter())
    {
        r_flags = p_right->_code[AND_NODE_FLAGS_POS];
        r_flags = ((r_flags >> 16) | r_flags ) & 0xFFFF;
    }

    if (_curr_window_time != NO_TIMED && 
            (l_flags & IS_TIMED) && (r_flags & IS_TIMED))
    {
        ULong codes[LEN_WAND_NODE];

        switch (type)
        {
            case INTER_AND : codes[0]=WAND; break;
            case INTER_NAND: codes[0]=NWAND; break;
            case INTER_OAND: codes[0]=OWAND; break;
        }
        codes[AND_NODE_CODELEN_POS] = 0;              // Code length
        codes[AND_NODE_N_ITEMS_POS] = 0;              // Number of items by left and right will be set at connection
        codes[AND_NODE_NKEYS_POS] =0;                 // Number of keys in the memories
        codes[AND_NODE_FLAGS_POS]   = ((l_flags << 16) | (r_flags & 0xFFFF)); // Flags L and R
        codes[WAND_NODE_WTIME_POS]  = _curr_window_time;
        // Memories
        codes[WAND_NODE_MEM_START_POS]     = (ULong) new BTree(); 
        codes[WAND_NODE_MEM_START_POS + 1] = (ULong) new BTree(); 
        node = new Node(type, LEN_WAND_NODE, codes);
    }
    else
    {
        ULong codes[LEN_AND_NODE];

        switch (type)
        {
            case INTER_AND : codes[0]=AND; break;
            case INTER_NAND: codes[0]=NAND; break;
            case INTER_OAND: codes[0]=OAND; break;
        }
        codes[AND_NODE_CODELEN_POS] = 0;              // Code length
        codes[AND_NODE_N_ITEMS_POS] = 0;              // Number of items by left and right will be set at connection
        codes[AND_NODE_NKEYS_POS] =0;                 // Number of keys in the memories
        codes[AND_NODE_FLAGS_POS]   = ((l_flags << 16) | (r_flags & 0xFFFF)); // Flags L and R
        // Memories
        codes[AND_NODE_MEM_START_POS]     = (ULong) new BTree(); 
        codes[AND_NODE_MEM_START_POS + 1] = (ULong) new BTree();
        node = new Node(type, LEN_AND_NODE, codes);
    }
    return node;
}

/**
 * @brief Create a TIMER node. A node that is capable to maintain simple objects during its window interval
 * 
 * @param last_intra Last intra node of this objects
 * @param real_root Root of the tree
 * @param window Window Time
 * @return Node* Timer node created
 */
Node *
Node::create_TIMER_node(Node **last_intra, Node *real_root, int window)
{
    Node *node;
    ULong code[LEN_TIMER_NODE];

    code[0] = TIMER;
    code[TIMER_NODE_WINDOW_POS] = window;
    code[TIMER_NODE_MEM_POS] = (ULong) new BTree();

    node = insert_intra_node(last_intra, real_root, LEN_TIMER_NODE, code);
    return node;
}

/**
 * @brief Create a SET node
 * 
 * @param first_pos Position of the first object in the set
 * @param n_objs Number of objects in each element of the set
 * @return Node* Resulting SET node
 */
Node *
Node::create_SET_node(int first_pos, int n_objs)
{

    Node *node;
    ULong code[LEN_SET_NODE];

    code[0] = MAKESET;
    code[SET_NODE_MEM_POS] = (ULong) new BTree();
    code[SET_NODE_CODELEN_POS] = 0L;  // Length of internal code
    code[SET_NODE_N_ITEMS_POS] = n_objs;
    code[SET_NODE_FIRST_ITEM_POS] = (first_pos << 8);
	code[SET_NODE_PATT_MASK]=0;
	code[SET_NODE_PATT_MASK+1]=0;
	code[SET_NODE_PATT_MASK+2]=0;
	code[SET_NODE_PATT_MASK+3]=0;
    node = new Node(INTRA_SET, LEN_SET_NODE, code);

    return node;
}

/**
 * @brief Move the asymmetric children nodes of this (NAND and OWAN) as children of the last inter node from 
 *      new_parent that is an AND node child of this.
 *      The asymmetric nodes are better at the end to guarantee the accessibility of the objects that enter
 *      in the asym nodes by right and are not available below them.
 * 
 *      this is the parent of the AND node, connected by the side "side"
 * 
 * @param new_parent - AND node created below this
 * @param side - side where this is entering in the AND node "new_parent"
 */
void
Node::move_down_asym_children(Node *new_parent, int side)
{
    Node *node, *node_AND;
    int conn_side;
    NodeIter iterator;

    node_AND = new_parent;
    new_parent = new_parent->last_inter_node(FALSE);

    for (node = first_child(conn_side, iterator); 
            node != NULL;
            node = next_child(conn_side, iterator))
    {
        if (node->has_mark(IN_RULE_MARK) && node->is_inter_asym())
        {

            if (side == RIGHT_MEM)
                node->shift_code(conn_side, 0, node_AND->_n_items_l);
            else
                // In fact it will affect to the children of node
                node->shift_code(conn_side, 
                        (conn_side == LEFT_MEM) ? node->_n_items_l : node->_n_items_r,
                        node_AND->_n_items_r);

            // If the child was marked the parent should be also

            if (node->has_mark(SIMPLE_MARK))
                new_parent->set_mark(SIMPLE_MARK);

            node->disconnect_node(this, conn_side);
            if (conn_side == LEFT_MEM)
            {
                node->connect_node(new_parent, conn_side);
                new_parent = node;
                node->move_down_set_nodes();
            }
            else node->connect_node(node_AND, conn_side);

            node->set_mark(IN_PATH_MARK);
            if (conn_side == LEFT_MEM)
                node->set_path_down();

        }

    }
}

/**
 * @brief In some cases two asymmetric nodes may be converted in a single one
 * 
 */
void
Node::optimize_asym_children()
{
    Node *node1, *node2, *child;
    int side1, side2, side_child;
    int deleted_1, ok_to_delete;
    NodeIter iter1, iter2, iter3;

    node1 = first_child(side1, iter1);

    while (node1 != NULL)
    {

        ok_to_delete = deleted_1 = FALSE;
        iter2 = iter1;
        node2 = next_child(side2, iter2);

        if (node1->_type != INTER_AND && node1->has_mark(IN_RULE_MARK) && node2 != NULL)
        {
            do {
                ok_to_delete = FALSE;
                if (node2->_type == node1->_type && 
                        node2->has_mark(IN_RULE_MARK) && side1 == side2)
                {
                    if (side1 == RIGHT_MEM)
                        ok_to_delete = in_asym_left_chain(node1, node2);
                    else
                    {
                        ok_to_delete = ( node1->parent_node(RIGHT_MEM) ==
                                node2->parent_node(RIGHT_MEM));
                    }
                }

                if (ok_to_delete)
                {

                    Node *to_delete, *to_preserve;

                    deleted_1 = !(node1->has_mark(ANY_CONTROL_MARK));

                    to_delete = (deleted_1)? node1 : node2;
                    to_preserve = (deleted_1)? node2 : node1;

                    to_preserve->add_code(to_delete);

                    if (deleted_1)
                        node1 = next_child(side1, iter1);
                    else
                        node2 = next_child(side2, iter2);

                    if (to_delete->_type == INTRA_SET)
                        Pattern::set_last_intra_set(to_delete, to_preserve);

                    for (child = to_delete->first_child(side_child, iter3);
                            child!=NULL;
                            child= to_delete->next_child(side_child, iter3))
                    {
                        if (child != to_preserve)
                        {
                            child->disconnect_node(to_delete, side_child);
                            child->connect_node(to_preserve, side_child);
                        }
                    }

                    delete to_delete;

                }

                else
                {
                    node2 = next_child(side2, iter2);
                }

            }
            while (node2 != NULL && (!ok_to_delete || !deleted_1));
        }

        if (!ok_to_delete || !deleted_1)
            node1 = next_child(side1, iter1);

    }
}

/**
 * @brief This function check if two nodes are connected following the parents by left side
 *      the check is done in both directions: from node1 to node2 and from node2 to node1
 * 
 * @param node1 
 * @param node2 
 * @return int 
 */
int
Node::in_asym_left_chain(Node *node1, Node *node2)
{
    Node *node_aux;

    // Let's try to reach from node1 to node2 going by the parents by left
    for (node_aux = node1; 
            node_aux!=NULL && node_aux!=node2 && node_aux->_type != INTER_AND; 
            node_aux = node_aux->parent_node(LEFT_MEM));

    if (node_aux != node2)
    {
        // Let's then try to reach from node2 to node1 going by the parents by left
        for (node_aux = node2; 
                node_aux!=NULL && node_aux!=node1 && node_aux->_type != INTER_AND; 
                node_aux = node_aux->parent_node(LEFT_MEM));

        return node_aux == node1;
    }

    else
        return TRUE;
}

/**
 * @brief Detects that this node has other brothers (from the parent of side "side")
 *      that belongs to the same rule too. This node is the last added children
 * 
 * @param side The side to inspect
 * @return int TRUE or FALSE
 */
int 
Node::double_path(int side)
{
    Node *parent, *child;
    NodeIter iterator;

    parent = parent_node(side);

    for (child = parent->first_child(side, iterator);
            child != NULL && (!child->has_mark(IN_RULE_MARK) || child == this);
            child = parent->next_child(side, iterator));

    return (child != NULL && child != this);
}

/**
 * @brief Alter the code of the node shiftting in 'how_many' the positions of certain objects from 
 *      a starting positions at a given side
 *      When moving the INTER nodes, the positions of the objects in the resulting tuplas are affected
 * 
 * @param side Side of the object to shift 
 * @param begin_shift Starting positions where to move from
 * @param how_many Number of positions to shift
 */
void
Node::shift_code(int side, int begin_shift, int how_many)
{
    Node *node;
    int conn_side;
    NodeIter iterator;
    ULong m, i;
    ULong curr, last;
    int n;

    last = 0;
    for (n=0; n<_lcode; n++)
    {
        curr = dasm_code(_code[n]);
        switch (curr)
        {
            // Special Codes
            case TOR :
                n+=(int)(_code[n+1]) + 1; break;
            case TNSOBJ:
                n++;
                // TNSOBJ is followed by a Longwith two object addresses (16+16bits)
                // encoded in the usual way SPPP PPPP AAAA AAAA
                // In this case the attribute number will be 0 due this code references an object
                
                if (side == ((_code[n] >> 31) & 0x1) )  // Higher Object
                {
                    if (((_code[n] >> 24) & 0x7F) >= begin_shift)
                        _code[n] += (how_many << 24);
                }

                if (side == ((_code[n] >> 15) & 0x1) ) // Lower Object
                {
                    if (((_code[n] >> 8) & 0x7F) >= begin_shift)
                        _code[n] += (how_many << 8);
                }
                break;
            case MAKESET:
                n+= (int)_code[SET_NODE_CODELEN_POS] + LEN_SET_NODE - 1;	
                // We move to the end of the code
                // The internal code of SET node is used to compare to objects in the set to order them and is not affected by shift
                m = SET_NODE_FIRST_ITEM_POS;
                for (i = _code[SET_NODE_N_ITEMS_POS]; i>0; i--)
                {
                    if (side == (_code[m]>>15) &&
                            ((_code[m]>>8) & 0x7F) >= begin_shift)
                    {
                        _code[m] = _code[m] + (how_many<<8);
                    }
                    m++;
                }
                break;
            case AND :
            case NAND:
            case OAND:
                n+= LEN_AND_NODE + _code[AND_NODE_NKEYS_POS] - 1; 

				// Keys
				for (i =0; i<_code[AND_NODE_NKEYS_POS]; i++)
				{
					if (side == LEFT_MEM)
						_code[LEN_AND_NODE+i] += (how_many << (8+7+8));
					else
						_code[LEN_AND_NODE+i] += (how_many << 8);
				}

				// N_items
                if (side == LEFT_MEM)
                    _code[AND_NODE_N_ITEMS_POS] += (how_many << 16);
                else
                    _code[AND_NODE_N_ITEMS_POS] += how_many;
                break;
            case WAND:
            case NWAND:
            case OWAND:
                n+= LEN_WAND_NODE + _code[AND_NODE_NKEYS_POS] - 1;  

				// Keys
				for (i =0; i<_code[AND_NODE_NKEYS_POS]; i++)
				{
					if (side == LEFT_MEM)
						_code[LEN_WAND_NODE+i] += (how_many << (8+7+8));
					else
						_code[LEN_WAND_NODE+i] += (how_many << 8);
				}

				// N_items
                if (side == LEFT_MEM)
                    _code[AND_NODE_N_ITEMS_POS] += (how_many << 16);
                else
                    _code[AND_NODE_N_ITEMS_POS] += how_many;
                break;

                // Codes of length 3
            case FCALL :
                n+=3;
                break;

                // Codes of length 2
            case TCLASS:
            case EVAL  :
                n++; 
                break;

                // CCodes of length  1
            case TTRUE :
            case TFALSE:
            case NOT   :
                break;

            default :

                // Typed codes
                switch (curr & ~0x3)
                {
                    case ADD : break;
                    case SUB : break;
                    case MUL : break;
                    case DIV : break;
                    case MINUS:break;
                    case TEQ : break;
                    case TNE : break;
                    case TLT : break;
                    case TGT : break;
                    case TLE : break;
                    case TGE : break;
                    case CMP : break;
                    case PUSH: n++; break;

                    case PUSHS:
                    case PUSHO:
                    case PUSHT:
                    case COUNT:
                    case CONCS:
                    case SUMS:
                    case PRDS:
                    case MINS:
                    case MAXS:
                               n++;
                               if (side == (_code[n]>>15) &&
                                       ((_code[n]>>8) & 0x7F) >= begin_shift)
                               {
                                   _code[n] = _code[n] + (how_many<<8);
                               }
                               break;

                    default :
                               comp_err("Unexpected code %X in shift_code(), last = %X\n", curr, last);
                }
        }
        last = curr;
    }

    if (! is_inter_asym() || side == LEFT_MEM)
    {
        for (node = first_child(conn_side, iterator);
                node != NULL && node->has_mark(IN_RULE_MARK);
                node = next_child(conn_side, iterator))
        {
            node->shift_code(conn_side, (side == LEFT_MEM) ? begin_shift : begin_shift + _n_items_l, how_many);
        }
    }
}

/**
 * @brief Change the code so the object at a certain side now is at the opposite side and with a new position 
 * 
 * @param side Side of the object
 * @param offset_pos The new position at the other side
 */
void
Node::move_code_mem(int side, int offset_pos)
{
    ULong m, i;
    ULong curr, last;
    int n;

    last = 0;
    for (n=0; n<_lcode; n++)
    {
        curr = dasm_code(_code[n]);
        switch (curr)
        {

            // Special codes
            case TOR :
                n+=(int)(_code[n+1]) + 1; break;
            case TNSOBJ:
                n++;

                // TNSOBJ is followed by a Longwith two object addresses (16+16bits)
                // encoded in the usual way SPPP PPPP AAAA AAAA
                // In this case the attribute number will be 0 due this code references an object

                if (side == ((_code[n] >> 31) & 0x1))  // Objeto alto
                    _code[n] = ((_code[n] & 0x7FFFFFFF) | ((!side) << 31)) + (offset_pos<<24);
                if (side == ((_code[n] >> 15) & 0x1))  // Objeto bajo
                    _code[n] = ((_code[n] & 0xFFFF7FFF) | ((!side) << 15)) + (offset_pos<<8);
                break;
            case MAKESET:
                n+= LEN_SET_NODE - 1;	// Move to the end of code
                // A SET CANNOT CHANGE ITS SIDE
                break;
            case AND :
            case NAND:
            case OAND:
                // AND nodes cannot be moved of memory side 
                n+= LEN_AND_NODE + _code[AND_NODE_NKEYS_POS] - 1;
                break;
            case WAND:
            case NWAND:
            case OWAND:
                n+= LEN_WAND_NODE + _code[AND_NODE_NKEYS_POS] - 1;
                break;

                // Codes of length 3
            case FCALL :
                n+= 3;
                break;

                // Codes of length 2
            case TCLASS:
            case EVAL  :
                n++; 
                break;

                // Codes of length 1
            case TTRUE :
            case TFALSE:
            case NOT   :
                break;

            default :

                // Typed Codes
                switch (curr & ~0x3)
                {
                    case ADD : break;
                    case SUB : break;
                    case MUL : break;
                    case DIV : break;
                    case MINUS:break;
                    case TEQ : break;
                    case TNE : break;
                    case TLT : break;
                    case TGT : break;
                    case TLE : break;
                    case TGE : break;
                    case CMP : break;
                    case PUSH: n++; break;

                    case PUSHS:
                    case PUSHO:
                    case PUSHT:
                    case COUNT:
                    case CONCS:
                    case SUMS:
                    case PRDS:
                    case MINS:
                    case MAXS:
                               n++;
                               if ((_code[n]>>15) == side)
                                   _code[n] = ((_code[n] & 0xFFFF7FFF) | ((!side)<<15)) + (offset_pos<<8);
                               break;

                    default :
                               comp_err("Unexpected code %X in move_code_mem(), last = %X\n", curr, last);
                }
        }
        last = curr;
    }

}

/**
 * @brief Move down in the net, below the INTER nodes, all the SET nodes, this way the item to be 
 *      converted in set is available as simple item in the INTER nodes and the conditions over the SET
 *      may go in the SET node after
 * 
 *      this is the last INTER node created
 */
void
Node::move_down_set_nodes()
{
    Node *parent;
    Node *node;
    Node *last_node;
    NodeIter iter;
    int side;

    // When two INTRA nodes and joint by an AND node, the SET nodes must be
    // moved below that AND node
    //
    //        A    B            A    B
    //       / \  / \            \  /
    //  SET(A)  \/   SET(B)       \/
    //        AND(A,B)          AND(A,B)
    //                             |
    //                           SET(A)
    //                             |
    //                           SET(B)

    last_node = this->last_in_INTRA_SET_chain(); 

    // Let's move the INTRA_SET brother nodes from the left to below the node
    parent = parent_node(LEFT_MEM);
    if (parent != NULL)
    {
        for (node = parent->first_child(side, iter);
                node != NULL ;
                node = parent->next_child(side, iter))
        {
            if (node != this && node->has_mark(IN_RULE_MARK) && node->_type == INTRA_SET) 
            {
                node->disconnect_node(parent, side);
                node->connect_node(last_node, side);
                last_node = node->last_in_INTRA_SET_chain();
            }
        }
    }

    // Let's move the INTRA_SET brother nodes from the right to below the node
    parent = parent_node(RIGHT_MEM);
    if (parent != NULL)
    {
        for (node = parent->first_child(side, iter);
                node != NULL ;
                node = parent->next_child(side, iter))
        {
            if (node != this && node->has_mark(IN_RULE_MARK) && node->_type == INTRA_SET)
            {
                node->disconnect_node(parent, side);
                node->connect_node(last_node, side);
                node->shift_code(LEFT_MEM, 0, this->_n_items_l);
                last_node = node->last_in_INTRA_SET_chain();
            }
        }
    }
}

/**
 * @brief Starting from this node this function go down the SET child nodes that will 
 *      be moved to form a kind of chain after the INTER nodes 
 * 
 * @return Node* The last node (belonging to the rule) in the chain
 */
Node *
Node::last_in_INTRA_SET_chain()
{
    int side, deleted;
    Node *last, *next_last;
    Node *child;
    NodeIter iter;

    last = this;
    do {
        next_last = NULL;
        deleted = FALSE;

        for (child = last->first_child(side, iter);
                child != NULL && !deleted;
                child = last->next_child(side, iter))
        {
            if (child->has_mark(IN_RULE_MARK))
            {
                // Detecting the next in the chain 
                if (side == LEFT_MEM && child->_type == INTRA_SET)
                    next_last = child;

                if (child->_type == INTER_AND || child->_type == INTER_OAND)
                {
                    if (child->double_path(!side))
                    {
                        child->delete_AND_shortcut(last);
                        deleted = TRUE;
                    }

                }
            }

        }

        // go ahead over the next last node in the chain
        if (!deleted && next_last != NULL)
            last= next_last;

    } while (deleted || next_last!= NULL);
    return last;
}

/**
 * @brief Due to the movements with SET nodes the same object may arrive to an AND node by right and by left
 *      In this case the node must be deleted and its code moved to tha last SET node
 * 
 * @param set_node_parent 
 */
void
Node::delete_AND_shortcut(Node *set_node_parent)
{
    // El this es el nodo hijo del set a liquidar

    Node *other_parent, *child;
    int pos;
    int side, side2;
    NodeIter iter;


    // Vemos cuanto hay que desplazar el codigo que hacen referencia al lado
    // derecho en nodo AND ya que ahora estara todo asociado al lado izquierdo
    // pues se anadira al codigo del ultimo nodo SET

    // 1.- Encontrar el otro camino (el definitivo)

    if (set_node_parent == parent_node(LEFT_MEM))
        side = LEFT_MEM;
    else
        side = RIGHT_MEM;

    other_parent = parent_node(!side);

    // See for other child belonging to the rule 
    for (child = other_parent->first_child(side2, iter);
            child != NULL && (!child->has_mark(IN_RULE_MARK) || child == this);
            child = other_parent->next_child(side2, iter));

    // Then continue the chain to calculate position
    // until reaching set_node_parent (the original parent by the other side) 
    pos = 0;
    do
    {
        if (side2 == RIGHT_MEM)
            pos += child->_n_items_l;

        child = child->first_child(side2, iter);

    } while (child != NULL && child != set_node_parent);

    // Adapt the code of the AND node and add it to set_node_parent
    if (side == RIGHT_MEM)
    {
        shift_code(LEFT_MEM, 0, pos);
        move_code_mem(RIGHT_MEM, 0);
    }
    else
    {
        move_code_mem(RIGHT_MEM, pos);
    }
    set_node_parent->add_code(this);

    // Finally delete the INTER node
    delete this;
}

//
// ORDER KEYS GENERATION
//

/**
 * @brief Identify, into the code, structures of equality between 2 attributes (PUSH + PUSH + TEQ) and
 *        generate from them Keys to be used to order de objects on the values of these attributes
 *        This way makes incredibly faster, given al object, to find the other that accomplish with
 *        these conditions  
 * 
 * @param code Node code array
 * @param code_len length of code
 * @param nkey number of keys found
 * @return ULong* Arrays of keys (to be copied some where due this is an static memory)
 */
ULong *
Node::getInterKeysInCode(ULong *code, int code_len, int *nkey)
{

    /* Codification side + position + attr
    SPPPPPPPAAAAAAAA   S=side(1), P=position(7), A=attrnum(8)
    ULong mem = ((*code_p) >> 15) & 0x1;
    ULong pos = ((*code_p) >> 8) & 0x7F;
    ULong attr = (*code_p) & 0xFF;
    */

    static ULong Keys[128]; // This is the maximun number of objects in a rule
    int offset=0;
    *nkey=0;

    while(offset+5<=code_len)
    {
        bool ok=false;
        ULong op;
        op = code[offset];
        if (op == (PUSHS | TYPE_STR) ||
            op == (PUSHS | TYPE_NUM) ||
            op == (PUSHS | TYPE_FLO))
        {
            op = code[offset + 2];
            if (op == (PUSHS | TYPE_STR) ||
                op == (PUSHS | TYPE_NUM) ||
                op == (PUSHS | TYPE_FLO))
            {
                op = code[offset + 4];
                if (op == (TEQ | TYPE_STR) ||
                    op == (TEQ | TYPE_NUM) ||
                    op == (TEQ | TYPE_FLO))
                {
                    if ((code[offset+1] & 0x8000) != (code[offset+3] & 0x8000))
                    {
                        if (((code[offset+1] >>15) & 0x1) == RIGHT_MEM)
                            Keys[(*nkey)++] = (((code[offset+3] & 0x7FFF) << 15) | (code[offset+1] & 0x7FFF) | ((code[offset]&0x3)<<30));
                        else
                            Keys[(*nkey)++] = (((code[offset+1] & 0x7FFF) << 15) | (code[offset+3] & 0x7FFF) | ((code[offset]&0x3)<<30));
                        ok=true;
                        offset+=5;
                    }
                }
            }
        }
        if (!ok) break;
    }
    return Keys;
}

//
// PRODUCTION NODES
//

/**
 * @brief Insert a new production node
 * 
 * @return Node* The node created
 */
Node *
Node::insert_prod_node()
{
    Node *node;

    node = new Node(INTER_PROD, 0, 0);
    node -> connect_node(this, LEFT_MEM);
    return node;
}



//
// DELETION OF A RULE
//

/**
 * @brief Free a rule
 * 
 * @param node The production Node of the rule
 */
void
Node::free_rule(void *node, va_list)
{
    Node *the_node = (Node *)node;

    if (trace >= 2)
        fprintf(trace_file, "FREEING RULE %s/%s\n",
                (char *)the_node->_code[PROD_NODE_RULENAME_POS],
                (char *)the_node->_code[PROD_NODE_RULESETNM_POS]);

    the_node->free_nodes_up(FALSE);
}

/**
 * @brief It goes up recursively in the net from the production node.  
 * 
 * @param in_intra  If the node is an intra 
 */
void
Node::free_nodes_up(int in_intra)
{
    Node *p_node_left = parent_node(LEFT_MEM);
    Node *p_node_right = parent_node(RIGHT_MEM);

    // If a node has other children it does not continue up
    // Also, it will stop in the empty node after TCLASS checks (second empty node found)
    // The first empty node found is the last_intra and it is also deleted 
    
    if (_fork != NULL || (in_intra && _type == INTRA && _lcode == 0))
    {
        return;
    }
    else
    {
        if (_type == INTRA && _lcode == 0) in_intra=TRUE;

        delete this;

        if (p_node_left != NULL)
            p_node_left->free_nodes_up(in_intra);
        if (p_node_right != NULL)
            p_node_right->free_nodes_up(in_intra);
    }
}

/**
 * @brief Free all the nodes from the last_intra node, first it goes down deleting the children and then up
 *      this must be an the last intra node of a pattern
 * 
 */
void
Node::free_from_last_intra()
{
    NodeIter iter;
    Node *child;
    int side;

    // Freeing of all the children of the last_intra node (INTER or SET)
    for (child = first_child(side, iter); child != NULL; child = next_child(side,iter))
    {
        child->free_no_intra();
    }

    // Then going up deleting the intra nodes
    free_nodes_up(FALSE);
}

/**
 * @brief Delete all the children going deeply and the deleting when going back 
 * 
 */
void
Node::free_no_intra()
{
    NodeIter iter;
    Node *child;
    int side;

    for (child = first_child(side, iter); child != NULL; child = next_child(side,iter))
    {
        child->free_no_intra();
    }

    delete this;
}


/**
 * @brief Reset all the memories of a rule, going up from the production node
 * 
 * @param node 
 */
void
Node::reset_rule(const void *node, va_list)
    // Esta funcion es llamada desde un nodo de produccion
{
    Node *the_node = (Node *)node;
    if (trace >= 2)
        fprintf(trace_file, "RESET OF RULE %s/%s\n",
                (char *)the_node->_code[PROD_NODE_RULENAME_POS],
                (char *)the_node->_code[PROD_NODE_RULESETNM_POS]);

    ((Node *)node)->reset_nodes_up();
}

/**
 * @brief Reset all the memories from the production node upwards until the last intra nodes are reached
 *      INTRA nodes have no memory, so there is no need to go beyond the last intra
 * 
 */
void
Node::reset_nodes_up()
{

    Node *p_node_left = parent_node(LEFT_MEM);
    Node *p_node_right = parent_node(RIGHT_MEM);

    if (_lcode == 0) // HWe have reached the last_intra
    {
        return;
    }
    else
    {

        reset_code(_code, _lcode, FALSE);

        if (p_node_left != NULL)
            p_node_left->reset_nodes_up();
        if (p_node_right != NULL)
            p_node_right->reset_nodes_up();
    }
}

//
// TRACE HELPERS FUNCTIONS
//

/**
 * @brief Print the node
 * 
 * @param prefix spacing to reflect the node hierarchy
 * @param child_chain if TRUE it will continue with its children
 */
void
Node::print(const char *prefix, int child_chain)
{

    Node *child;
    int side;
    NodeIter iterator;
    char *new_prefix;
    Node *equal_node;
    int n_eq;

    if (this == 0)
        return;

    if (strlen(prefix)>0)
        fprintf(trace_file, "%.*s#------>", (int)(strlen(prefix) - 8), prefix);
    fprintf(trace_file, "NODE %s  (%s/%s)", 
            clave(this),
            (_parent_left == 0)?"-":clave(_parent_left),
            (_parent_right == 0)?"-":clave(_parent_right)
          );

    equal_node = this;
    while (equal_node->_eq_node_left != NULL)
    {
        equal_node = equal_node->_eq_node_left;
    }
    n_eq = 0;
    while (equal_node!= NULL)
    {
        n_eq++;
        equal_node = equal_node->_eq_node_right;
    }

    fprintf(trace_file, "  Type = %s, Items=%d/%d Paths=%d%s Eq=%d len=%d\n", 
            (_type == INTRA)? "INTRA" : 
            (_type == INTRA_SET)? "INTRA_SET" : 
            (_type == INTER_AND)? "INTER_AND" : 
            (_type == INTER_NAND)? "INTER_NAND" :
            (_type == INTER_OAND)? "INTER_OAND" :
            (_type == INTER_PROD)? "INTER_PROD" : "???",
            _n_items_l, _n_items_r ,
            _n_paths,
            has_mark(IN_PATH_MARK) ? (_mark ? "  (!!)" : "  (*)") : "",
            n_eq,
            _lcode);

    dasm_rete(prefix, _code, _lcode);

    if (_fork)
        fprintf(trace_file, "%s#\n", prefix);
    else
        puts(prefix);

    if (child_chain)
    {

        new_prefix = new char [strlen(prefix) + 8 + 1];

        for (child = first_child(side, iterator);
                child != NULL;
                child = next_child(side, iterator))
        {
            if (side == LEFT_MEM)
            {
                if (iterator.next != NULL)
                    sprintf(new_prefix, "%s#       ", prefix);
                else
                    sprintf(new_prefix, "%s        ", prefix);
                child->print(new_prefix, child->parent_node(LEFT_MEM) == this);
            }
        }
        delete [] new_prefix;
    }
}

/**
 * @brief Set a mask of an object that is converted to set
 *      The mask is a bit signaling what patterns will be converted to set
 *      As it can be up to 128 patterns the masks are stored as bits of 4 longs od 32 bits
 * 
 * @param begin_mask Where the masks are stored
 * @param n the number of pattern
 */
void
Node::setPatternMask(ULong *begin_mask, int n)
{
	int offset = n >> 5; // ( n / 32 )  In what of the 4 longs the correspondant bit will bis 
	n = (n & 0x1F);  // ( last 5 bits ) What bit is
	fprintf(trace_file, "Offset=%d, n=%d\n", offset, n);
	begin_mask[offset] |= (1 << n);
}

/**
 * @brief Check if a mark is set for an object converted to SET
 *      The mask is a bit signaling what patterns will be converted to set
 *      As it can be up to 128 patterns the masks are stored as bits of 4 longs od 32 bits
 * 
 * @param begin_mask Where the masks are stored
 * @param n the number of pattern. 
 * @return true is SET
 * @return false is not SET
 */
bool
Node::getPatternMask(ULong *begin_mask, int n)
{
	int offset = n >> 5; // ( n / 32 )
	n = (n & 0x1F);  // ( last 5 bits )
	return ((begin_mask[offset] & (1 << n)) != 0);
}

/**
 * @brief Print the masks of a MAKESET code
 * 
 * @param begin_mask Array de masks
 */
void
Node::printPatternMask(ULong *begin_mask)
{
    int offset = 0;
    int first = TRUE;

    for (int n=0; n<4; n++)
	{
    	ULong mask = 1;
    	for (int i=0; i<32; i++)
    	{
        	if (begin_mask[n] & mask) {
                fprintf(trace_file, "%s%d", first ? "": ", ", offset+i);
                first = FALSE;
            }
        	mask = mask << 1;
    	}
        offset += 32;
	}
}

/**
 * @brief Mark the position of an object that is converted to SET
 * 
 * @param pos Initial position of the object
 * @param last_intra_set last node to end the process when is reached
 */
void
Node::mark_set_positions(int pos, Node *last_intra_set)
{
    Node *child;
    int child_side;
    NodeIter iterator;

    for (child =  first_child(child_side, iterator);
            child != NULL;
            child = child->next_child(child_side, iterator))
    {
		if (_lcode > 0 && MAKESET == dasm_code(_code[0]))
			setPatternMask(_code+SET_NODE_PATT_MASK, pos);

		if (this == last_intra_set) return;

        if (child->has_mark(IN_RULE_MARK))
        {
            if (child_side == LEFT_MEM)
                child->mark_set_positions(pos, last_intra_set);
            else
                child->mark_set_positions(pos + child->_n_items_l, last_intra_set);
        }
    }
}

/**
 * @brief Print the INTRA parent nodes of a node (going up by LEFT)
 * 
 */
void
Node::print_parents()
{
    fprintf(trace_file, "%s ", clave(this));
    if (parent_node(LEFT_MEM) != NULL)
        parent_node(LEFT_MEM)->print_parents();
    else
        puts("");
}

/**
 * @brief Disassemble the code (print in a readable manner) in a Node
 * 
 */
void
Node::print_code()
{
    dasm_rete("\t", _code, _lcode);
}


