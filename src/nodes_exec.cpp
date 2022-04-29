/**
 * @file nodes_exec.cpp
 * @author Francisco Alcaraz
 * @brief This module implements all the functions that correspond to the operation codes 
 * @version 1.0
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "engine.h"

#include "codes.h"
#include "dasm_rete.hpp"
#include "patterns.hpp"
#include "nodes.hpp"
#include "rules.hpp"
#include "classes.hpp"
#include "load.hpp"
#include "status.hpp"
#include "metaobj.hpp"
#include "set.hpp"
#include "compound.hpp"
#include "single.hpp"
#include "eng.hpp"
#include "confset.hpp"
#include "keys.hpp"

ULong *Node::code_p;						/* Execution pointer					*/
Value Node::data_stack[DATA_STACK_SIZE]; 	/* Data stack							*/
Value *Node::dstack_p;						/* Pointer to the Data Stack			*/
BTree Node::conflict_set_mem[3];			/* Memories for teh Conflict Set		*/
int Node::cmp_result;						/* For the order relations in Sets	 	*/
int Node::checkingScope = FALSE;			/* Flag to expore scope on modifications*/


//
// NODE'S CODE EXECUTION CONTROL (PROPAGATION)
//

/**
 * @brief Propagate from an INTRA node a modification of an ObjType
 * 
 * @param data struct that maintain the structure by left, by right, the tab, 
 * 		  the position of the main user object affected of the tag, etc.
 * @param codep Current execution ofsset inside _codep array
 */
void Node::propagate_modify(ExecData &data, ULong *codep)
{
	if (codep == NULL) codep = _code;

	if (trace >= 2)
	{
		fprintf(trace_file, "*** NODES : PMOD MODIFY BY %s AT NODE %s OFFSET %ld\n", data.side == RIGHT_MEM ? "RIGHT": "LEFT", clave(this), codep - _code);
		print_code();
	}
	setChecking(TRUE);

	if (trace >= 2) fprintf(trace_file, "PROPAGATE MODIFY: RETRACTION\n");
	data.left->set_state(OLD_ST, data.st, data.pos);
	data.tag=RETRACT_TAG;
	propagate(data, codep);

	if (trace >= 2) fprintf(trace_file, "PROPAGATE MODIFY: INSERTION\n");
	data.left->set_state(NEW_ST, data.st, data.pos);
	data.tag=INSERT_TAG;
	propagate(data, codep);

	setChecking(FALSE);
	data.tag=MODIFY_TAG;

	propagate_reached_nodes(data, codep);

	if (trace >= 2)
	{
		fprintf(trace_file, "*** NODES : PMOD END MODIFY\n");
	}
}

/**
 * @brief Basic Propagation (not modify tag that is a double)
 * 		The modify tag is an RETRATION of the old version and a INSERTION of the new
 * 		this function is marking the path where the object has passed with the tag it was going
 * 		(MODIFY_TAG, when set in the node is due both: RETRACT AND INSERT versions, have passed through)
 * 		When reached a INTER node mark=NODE_REACHED and also is marked the side where it arrived
 * 
 * @param data struct that maintain the structure by left, by right, the tab, 
 * 		  the position of the main user object affected of the tag, etc.
 * @param codep Current execution ofsset inside _codep array
 * @return int NODE_REACHED or 0
 */
int Node::propagate(ExecData &data, ULong *codep)
{

	int res, res_global;
	Node *child;
	int side_child;
	NodeIter iter;

	if (codep == NULL) codep = _code;

	if (codep < _code + _lcode)
	{
		if (trace >= 2)
		{
			fprintf(trace_file, "*** NODES : PROP %s BY %s AT NODE %s OFFSET %ld\n", data.tag == INSERT_TAG ? "INSERT" : data.tag == RETRACT_TAG ? "RETRACT": "MODIFY", data.side == RIGHT_MEM ? "RIGHT": "LEFT", clave(this), codep - _code);
			print_code();
		}

		res = execute_code(data, codep);
	} else res=1;

	res_global = 0;
	if (res >0)
	{
		for (child = first_child(side_child, iter); child != NULL; child = next_child(side_child, iter))
		{
			// When reached a memory node (INTER) is not needed to propagate beyond
			// due they know how to propagate MODIFY_TAG as a single operation

			// data.side store the side where we have reached the memory node (INTER)
					
			data.side = side_child;
			res = child->propagate(data);

			if (res == NODE_REACHED)
			{
				res_global = NODE_REACHED;
				_mark |= data.tag;
			}
		}

	}
	else 
	{
		if (res == NODE_REACHED)
		{
			// In modification we can reach a memory node (INTER) by both sides so we make an OR |
			_mark |= (data.side == LEFT_MEM ? REACHED_BY_LEFT : REACHED_BY_RIGHT);
		}
		res_global = res;
	}

	return res_global;
}

/**
 * @brief Execute the nodes with _mark == NODE_REACHED with the tag from tha parent (data.tag)
 * 
 * @param data struct that maintain the structure by left, by right, the tab, 
 * 		  the position of the main user object affected of the tag, etc.
 * @param codep Current execution ofsset inside _codep array
 */
void Node::propagate_reached_nodes(ExecData &data, ULong *codep)
{
	Node *child;
	int side_child;
	NodeIter iter;

	if (codep == NULL) codep = _code;

	if (_mark == NO_MARK) return;

	if ((_mark & REACHED_BY_LEFT) || (_mark & REACHED_BY_RIGHT))
	{
		// The memory nodes are able to continue the propagation by themshelf
		if (trace >= 2)
		{
			fprintf(trace_file, "*** NODES : PROP-RN %s BY %s AT NODE %s OFFSET %ld\n", data.tag == INSERT_TAG ? "INSERT" : data.tag == RETRACT_TAG ? "RETRACT": "MODIFY", data.side == RIGHT_MEM ? "RIGHT": "LEFT", clave(this), codep - _code);
			print_code();
		}

		// Reset of the mark deppending of the side we are arriving the node at
		_mark &= ~(data.side == LEFT_MEM ? REACHED_BY_LEFT : REACHED_BY_RIGHT);

		if (codep < _code + _lcode)
		{
			if (data.tag == RETRACT_TAG)
				data.left->set_state(OLD_ST, data.st, data.pos);

			execute_code(data, codep);

			if (data.tag == RETRACT_TAG)
				data.left->set_state(NEW_ST, data.st, data.pos);
		}
	}
	else
	{
		for (child = first_child(side_child, iter); child != NULL; child = next_child(side_child, iter))
		{
			// we take the propagation tag from the _mark
			data.tag = _mark;
			data.side = side_child;
			child->propagate_reached_nodes(data);
		}
		_mark = NO_MARK;
	}
}

/**
 * @brief Enable a flags that stops the execution in every node with memories (e.g. INTER)
 * 
 * @param value The value TRU/FALSE of the flag
 */
void Node::setChecking(int value)
{
	checkingScope = value;
}

/**
 * @brief Execute the code in a Node while its execution is returning > 0 (the object passes the code)
 * 
 * @param data Execution data
 * @param codep Initial pointer of code inside the _code array of this Node
 * @return int 
 */
int Node::execute_code(ExecData &data, ULong *codep)
{
	int res = 1;

	code_p		= codep;
	dstack_p	= data_stack;

	ULong *end_of_code = _code + _lcode;
	
	while (res>0 && code_p<end_of_code)
	{
		res = (*((IntFunction)(*code_p)))(this, data);
	}
	return res;
}

//
// RETE OP-CODES EXECUTION
//


/**
 * @brief Executes an external function
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 Execution must continue
 */
int Node::user_func_call(Node *node, ExecData &data)
{
	ExternFunction func;
	ULong n_args;

	code_p++;
	func = (ExternFunction)(*code_p++);
	code_p++; // The name of the function
	n_args = (*code_p++);

	(*func)(dstack_p - n_args, data.tag);

	// The function must return the result in the stack !!

	dstack_p -= (n_args - 1); // Estara el resultado en el stack

	return(1);
}
	
/**
 * @brief Executes an external procedure (at the RHS)
 * 		The execution is done if the flags on_tags is according with the current tag
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 Execution must continue
 */
int Node::user_proc_call(Node *node, ExecData &data)
{
	ExternFunction func;
	ULong n_args, on_tags;
	int exec = 0;
	int len_code;
	ULong *end_pcall;
	int res;

	code_p++;
	func = (ExternFunction)(*code_p++);
	code_p++; // The procedure name that is no longer needed
	n_args = (*code_p++);
	on_tags = (*code_p++);
	len_code = (int)(*code_p++);
	end_pcall = code_p + len_code;

	switch (data.tag)
	{
		case INSERT_TAG :
			exec = (on_tags & EXEC_INSERT);
			break;
		case MODIFY_TAG :
			exec = (on_tags & EXEC_MODIFY);
			break;
		case RETRACT_TAG :
			exec = (on_tags & EXEC_RETRACT);
			break;
	}

	if (exec){

		res=1;
		while ( res>0 && code_p != end_pcall )
		{
			res = (*((IntFunction)(*code_p)))(node, data);
		}
		(*func)(dstack_p - n_args, data.tag);
		dstack_p -= n_args; // Wont be any result the stack

	} else {
		code_p += len_code;
	}

	return(1);
}

/**
 * @brief Execution of code TCLASS. Tests if an object belongs to a class
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::test_class_call(Node *node, ExecData &data)
{
	
	char * classname;

	code_p++;
	classname = (char*)(*code_p++);

	// The class name is the attribute 0

	return (strcmp((*data.left)[0]->single()->obj()->attr[0].str.str_p, 
						classname) == 0);
}
		

/**
 * @brief Execution of code TNSOBJ. Tests if two objects are not the same
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tnsobj_call(Node *node, ExecData &data)
{
	
	code_p++;
	ULong mem1 = ((*code_p) >> 31) & 0x1;
	ULong pos1 = ((*code_p) >> 24) & 0x7F;
	ULong mem2 = ((*code_p) >> 15) & 0x1;
	ULong pos2 = ((*code_p) >> 8) & 0x7F;
	ObjectType *obj1, *obj2;

	obj1 = (* ((mem1==LEFT_MEM) ? data.left : data.right))[(int)pos1]->single()->obj();
	obj2 = (* ((mem2==LEFT_MEM) ? data.left : data.right))[(int)pos2]->single()->obj();

	code_p++;
	return (obj1 != obj2);
}
		
/**
 * @brief Execution of code TTRUE. Tests if the result of an expression is TRUE
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::ttrue_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p--;
	return (dstack_p->num != 0);
}
 
/**
 * @brief Execution of code TFALSE. Tests if the result of an expression is FALSE
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tfalse_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p--;
	return (dstack_p->num == 0);
}
 

/**
 * @brief Execution of code EVAL. Tests if the object passes some code and puts the result in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int Node::eval_call(Node *node, ExecData &data)
{
	int res;
	ULong *end_of_code;

	code_p+=2;
	end_of_code = code_p + code_p[-1];

	res = 1;
	while (res>0	&& code_p < end_of_code)
	{
		res = (*((IntFunction)(*code_p)))(node, data);
	}
	code_p = end_of_code;

	dstack_p -> num = (long)res;
	dstack_p++;

	return 1;
}
	
/**
 * @brief Execution of code NOT. Negates a boolean value in the stack a leaves the result there
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int Node::not_call(Node *node, ExecData &data)
{
	code_p++;
	(dstack_p - 1)->num = !((dstack_p - 1)->num);
	return 1;
}

/**
 * @brief Execution of code TOR.
 * 		  TOR try to execute each of the set of conditions (bool expressions) until one of them are fully satisfied or no more are available
 * 		  This is called short circuit evaluation.
 * 		  The execution of a set of bool conditions is equivalent to the logic AND of all of them  		
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 if any of the alternatives was evaluated as TRUE and further execution will continue, = 0 otherwise and execution stops
 */
int Node::tor_call(Node *node, ExecData &data)
{

	ULong n_jumps;
	ULong *jumps;
	int res;

	code_p++;
	n_jumps = (*code_p++);
	jumps	= code_p;

	code_p += n_jumps;

	do
	{
		n_jumps --;

		res=1;
		while ( res>0 && code_p != (ULong*)(*jumps) )
		{
			res = (*((IntFunction)(*code_p)))(node, data);
		} 

		if (res<=0)
		{
			code_p = (ULong*)(*jumps++);
		}

	} while(n_jumps>0 && res<=0);

	if (res>0)
		code_p = (ULong*)(*(jumps+n_jumps));

	return(res);
}

/**
 * @brief Execution of code ADDN. Adds two integers in the stack and leaves the result there
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int
Node::addn_call(Node *node, ExecData &data)
{
	code_p++;
	dstack_p--;

	(dstack_p - 1)->num += dstack_p->num;
	return 1;
}

/**
 * @brief Execution of code ADDF. Adds two floats in the stack and leaves the result there
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int Node::addf_call(Node *node, ExecData &data)
{
	code_p++;
	dstack_p--;

	(dstack_p - 1)->flo += dstack_p->flo; 
	return 1;
}

/**
 * @brief Execution of code SUBN. Substracts two integers in the stack and leaves the result there
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int Node::subn_call(Node *node, ExecData &data)
{
	code_p++;
	dstack_p--;

	(dstack_p - 1)->num -= dstack_p->num; 
	return 1;
}

/**
 * @brief Execution of code SUBF. Substracts two floats in the stack and leaves the result there
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int Node::subf_call(Node *node, ExecData &data)
{
	code_p++;
	dstack_p--;

	(dstack_p - 1)->flo -= dstack_p->flo; 
	return 1;
}

/**
 * @brief Execution of code MULN. Multiplies two integers in the stack and leaves the result there
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int Node::muln_call(Node *node, ExecData &data)
{
	code_p++;
	dstack_p--;

	(dstack_p - 1)->num *= dstack_p->num; 
	return 1;
}

/**
 * @brief Execution of code MULF. Multiplies two floats in the stack and leaves the result there
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int Node::mulf_call(Node *node, ExecData &data)
{
	code_p++;
	dstack_p--;

	(dstack_p - 1)->flo *= dstack_p->flo; 
	return 1;
}

/**
 * @brief Execution of code DIVN. Divides two integers in the stack and leaves the result there
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int Node::divn_call(Node *node, ExecData &data)
{
	code_p++;
	dstack_p--;

	(dstack_p - 1)->num /= dstack_p->num;
	return 1;
}

/**
 * @brief Execution of code MULF. Multiplies two floats in the stack and leaves the result there
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int Node::divf_call(Node *node, ExecData &data)
{
	code_p++;
	dstack_p--;

	(dstack_p - 1)->flo /= dstack_p->flo; 
	return 1;
}

/**
 * @brief Execution of code MINUSN (Unary Minus). Change the sign of a integer in the stack and leaves the result there
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int Node::minusn_call(Node *node, ExecData &data)
{
	code_p++;
	(dstack_p - 1)->num = -(dstack_p - 1)->num;
	return 1;
}

/**
 * @brief Execution of code MINUSF (Unary Minus). Changes the sign of a float in the stack and leaves the result there
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 as execution must continue
 */
int Node::minusf_call(Node *node, ExecData &data)
{

	code_p++;
	(dstack_p - 1)->flo = -(dstack_p - 1)->flo; 
	return 1;
}

/**
 * @brief Execution of code TEQA. Tests if two strings in the stack are equal
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::teqa_call(Node *node, ExecData &data)
{
	int res;

	code_p++;
	dstack_p-=2;

	if (dstack_p->str.str_p == NULL)
		if ((dstack_p+1)->str.str_p == NULL)
			res = 1;
		else 
			res = 0;
	else
		if((dstack_p+1)->str.str_p == NULL)
			res = 0;
		else
			res = (strcmp(dstack_p->str.str_p, (dstack_p+1)->str.str_p) == 0 );

	if (dstack_p->str.dynamic_flags == DYNAMIC)	// NO PROTECTED TOO
		free( dstack_p->str.str_p);

	if ((dstack_p+1)->str.dynamic_flags == DYNAMIC)	// NO PROTECTED TOO
		free( (dstack_p+1)->str.str_p);

	return(res);
}

/**
 * @brief Execution of code TEQN. Tests if two integers in the stack are equal
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::teqn_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->num == (dstack_p+1)->num );
}

/**
 * @brief Execution of code TEQF. Tests if two floats in the stack are equal
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::teqf_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->flo == (dstack_p+1)->flo );
}

/**
 * @brief Execution of code TNEA. Tests if two strings in the stack are distinct
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tnea_call(Node *, ExecData &/*data*/)
{
	int res;

	code_p++;
	dstack_p-=2;

	if (dstack_p->str.str_p == NULL)
		if ((dstack_p+1)->str.str_p == NULL)
			res = 0;
		else 
			res = 1;
	else
		if((dstack_p+1)->str.str_p == NULL)
			res = 1;
		else
			res = ( strcmp(dstack_p->str.str_p, (dstack_p+1)->str.str_p) != 0 );

	if (dstack_p->str.dynamic_flags == DYNAMIC)	
		free( dstack_p->str.str_p);
	if ((dstack_p+1)->str.dynamic_flags == DYNAMIC)
		free( (dstack_p+1)->str.str_p);

	return(res);
}

/**
 * @brief Execution of code TNEN. Tests if two integers in the stack are distinct
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tnen_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->num != (dstack_p+1)->num );
}

/**
 * @brief Execution of code TNEF. Tests if two floats in the stack are distinct
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tnef_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->flo != (dstack_p+1)->flo );
}

/**
 * @brief Execution of code TLTA. Tests if two strings in the stack are the first lower that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tlta_call(Node *, ExecData &/*data*/)
{
	int res;

	code_p++;
	dstack_p-=2;

	if ((dstack_p+1)->str.str_p == NULL)
			res = 0;
	else
		if (dstack_p->str.str_p == NULL)
			res = 1;
		else
			res = ( strcmp(dstack_p->str.str_p, (dstack_p+1)->str.str_p) < 0 );

	if (dstack_p->str.dynamic_flags == DYNAMIC)
		free( dstack_p->str.str_p);
	if ((dstack_p+1)->str.dynamic_flags == DYNAMIC)
		free( (dstack_p+1)->str.str_p);

	return(res);
}

/**
 * @brief Execution of code TLTN. Tests if two integers in the stack are the first lower that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tltn_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->num < (dstack_p+1)->num );
}

/**
 * @brief Execution of code TLTF. Tests if two floats in the stack are the first lower that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tltf_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->flo < (dstack_p+1)->flo );
}

/**
 * @brief Execution of code TLEA. Tests if two strings in the stack are the first lower or equal that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tlea_call(Node *, ExecData &/*data*/)
{
	int res;

	code_p++;
	dstack_p-=2;

	if (dstack_p->str.str_p == NULL)
			res = 1;
	else
		if ((dstack_p+1)->str.str_p == NULL)
			res = 0;
		else
			res = ( strcmp(dstack_p->str.str_p, (dstack_p+1)->str.str_p) <= 0 );

	if (dstack_p->str.dynamic_flags == DYNAMIC)	
		free( dstack_p->str.str_p);
	if ((dstack_p+1)->str.dynamic_flags == DYNAMIC)
		free( (dstack_p+1)->str.str_p);

	return(res);
}

/**
 * @brief Execution of code TLEN. Tests if two integers in the stack are the first lower or equal that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tlen_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->num <= (dstack_p+1)->num );
}

/**
 * @brief Execution of code TLEF. Tests if two floats in the stack are the first lower or equal that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tlef_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->flo <= (dstack_p+1)->flo );
}

/**
 * @brief Execution of code TGEA. Tests if two strings in the stack are the first greater or equal that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tgea_call(Node *, ExecData &/*data*/)
{
	int res;

	code_p++;
	dstack_p-=2;

	if ((dstack_p+1)->str.str_p == NULL)
			res = 1;
	else
		if (dstack_p->str.str_p == NULL)
			res = 0;
		else
			res = ( strcmp(dstack_p->str.str_p, (dstack_p+1)->str.str_p) >= 0 );

	if (dstack_p->str.dynamic_flags == DYNAMIC)
		free( dstack_p->str.str_p);
	if ((dstack_p+1)->str.dynamic_flags == DYNAMIC)
		free( (dstack_p+1)->str.str_p);

	return(res);
}

/**
 * @brief Execution of code TGEN. Tests if two integers in the stack are the first greater or equal that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tgen_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->num >= (dstack_p+1)->num );
}

/**
 * @brief Execution of code TGEF. Tests if two floats in the stack are the first greater or equal that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tgef_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->flo >= (dstack_p+1)->flo );
}

/**
 * @brief Execution of code TGTA. Tests if two strings in the stack are the first greater that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tgta_call(Node *, ExecData &/*data*/)
{
	int res;

	code_p++;
	dstack_p-=2;

	if (dstack_p->str.str_p == NULL)
			res = 0;
	else
		if ((dstack_p+1)->str.str_p == NULL)
			res = 1;
		else
			res = ( strcmp(dstack_p->str.str_p, (dstack_p+1)->str.str_p) > 0 );

	if (dstack_p->str.dynamic_flags == DYNAMIC)
		free( dstack_p->str.str_p);
	if ((dstack_p+1)->str.dynamic_flags == DYNAMIC)
		free( (dstack_p+1)->str.str_p);

	return(res);
}

/**
 * @brief Execution of code TGTN. Tests if two integers in the stack are the first greater that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tgtn_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->num > (dstack_p+1)->num );
}

/**
 * @brief Execution of code TGTF. Tests if two floats in the stack are the first greater that the second
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int The result of the comparison
 */
int Node::tgtf_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	return( dstack_p->flo > (dstack_p+1)->flo );
}

/**
 * @brief Execution of code CMPA. Compare two strings in the stack are equal that the second
 * 	      Used to order elements based on code execution, due cmp_result is available
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 if are equal and execution must be continue
 */
int Node::cmpa_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	if (dstack_p->str.str_p == NULL)
		if ((dstack_p+1)->str.str_p == NULL)
			cmp_result = 0;
		else
			cmp_result = -1;
	else if ((dstack_p+1)->str.str_p == NULL)
			cmp_result = 1;
		else
			cmp_result = strcmp(dstack_p->str.str_p, (dstack_p+1)->str.str_p);
 
	if (dstack_p->str.dynamic_flags == DYNAMIC)	// NO PROTECTED TOO
		free( dstack_p->str.str_p);
	if ((dstack_p+1)->str.dynamic_flags == DYNAMIC)	// NO PROTECTED TOO
		free( (dstack_p+1)->str.str_p);
 
	return(cmp_result == 0);
}

/**
 * @brief Execution of code CMPN. Compare two integers in the stack are equal that the second
 * 	      Used to order elements based on code execution, due cmp_result is available
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 if are iqual and execution must be continue
 */
int Node::cmpn_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	cmp_result = (int)(dstack_p->num - (dstack_p+1)->num);
	return (cmp_result == 0);
}

/**
 * @brief Execution of code CMPF. Compare two floats in the stack are equal that the second
 * 	      Used to order elements based on code execution, due cmp_result is available
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 if are iqual and execution must be continue
 */
int Node::cmpf_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p-=2;

	if (dstack_p->flo == (dstack_p+1)->flo)
		cmp_result = 0;
	else if (dstack_p->flo < (dstack_p+1)->flo)
		cmp_result = -1;
	else
		cmp_result = 1;

	return (cmp_result == 0);
}

/**
 * @brief Execution of code SUMSN. Adds the integer values of the same attribute of all the elements in a Set
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::sumsn_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();
 
	dstack_p->num = set->sum_set_n((int)attr);
	dstack_p++;

	return(1);
}

/**
 * @brief Execution of code SUMSF. Adds the float values of the same attribute of all the elements in a Set
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::sumsf_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();
 
	dstack_p->flo = set->sum_set_f((int)attr);
	dstack_p++;
	return(1);
}

/**
 * @brief Execution of code PRODSN. Multiplies the integer values of the same attribute of all the elements in a Set
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::prdsn_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();
 
	dstack_p->num = set->prod_set_n((int)attr);
	dstack_p++;
	return(1);
}

/**
 * @brief Execution of code PRODSF. Multiplies the float values of the same attribute of all the elements in a Set
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::prdsf_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();
 
	dstack_p->flo = set->prod_set_f((int)attr);
	dstack_p++;
	return(1);
}

/**
 * @brief Execution of code MINSN. Find the minimum integer value for an attribute for all the elements in a Set
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::minsn_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();
 
	dstack_p->num = set->min_set_n((int)attr);
	dstack_p++;
	return(1);
}
 
/**
 * @brief Execution of code MINSF. Find the minimum float value for an attribute for all the elements in a Set
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::minsf_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();
 
	dstack_p->flo = set->min_set_f((int)attr);
	dstack_p++;
	return(1);
}
 
/**
 * @brief Execution of code MINSA. Find the minimum string value for an attribute for all the elements in a Set
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */int Node::minsa_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();
 
	dstack_p->str.str_p = set->min_set_a((int)attr);
	dstack_p->str.dynamic_flags = DYNAMIC;
	dstack_p++;
	return(1);
}
 
/**
 * @brief Execution of code MAXSN. Find the maximum integer value for an attribute for all the elements in a Set
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::maxsn_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();
 
	dstack_p->num = set->max_set_n((int)attr);
	dstack_p++;
	return(1);
}
 
/**
 * @brief Execution of code MAXSF. Find the maximum float value for an attribute for all the elements in a Set
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::maxsf_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();
 
	dstack_p->flo = set->max_set_f((int)attr);
	dstack_p++;
	return(1);
}

/**
 * @brief Execution of code MAXSA. Find the maximum string value for an attribute for all the elements in a Set
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::maxsa_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();
 
	dstack_p->str.str_p = set->max_set_a((int)attr);
	dstack_p->str.dynamic_flags = DYNAMIC;
	dstack_p++;
	return(1);
}
 
/**
 * @brief Execution of code COUNT. Counts the elements in a Set
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::count_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p++) >> 8) & 0x7F;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();

	dstack_p->num = set->n_objs();
	dstack_p++;
	return(1);
}

/**
 * @brief Execution of code CONCAT. Concat the values of an attribute for all the elements in a Set, using a separator
 * 		The result is left in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::concat_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	int attr = (int)((*code_p++) & 0xFF);
	char * separator;
	Set *set;
 
	if (mem == LEFT_MEM)
		set = (*data.left)[(int)pos]->set();
	else
		set = (*data.right)[(int)pos]->set();
	
	dstack_p--;

	separator = dstack_p->str.str_p;
	dstack_p->str.str_p = set->concat(attr, separator);

	if (dstack_p->str.dynamic_flags == DYNAMIC)
		free(separator);

	dstack_p->str.dynamic_flags = DYNAMIC;
	dstack_p++;
	return(1);
}

/**
 * @brief Execution of code PUSHA. Store a string in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::pusha_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p->str.str_p = (char *)(*code_p++);
	dstack_p->str.dynamic_flags = FALSE;		// The attribute value is this way protected and never will we freed unless in POPSA
	dstack_p++;
	return(1);
}

/**
 * @brief Execution of code PUSH. Store a number, integer or float, in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::push_call(Node *, ExecData &/*data*/)
{
	code_p++;
	dstack_p->num = (long)(*code_p++);
	dstack_p++;
	return(1);
}

/**
 * @brief Execution of code PUSHS. Store the value of an attribute of an object in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::pushs_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	MetaObj *meta;

	if (mem == LEFT_MEM)
		meta = (*data.left)[(int)pos];
	else
		meta = (*data.right)[(int)pos];

	if (meta->class_type() == SINGLE)
		(*dstack_p) = meta->single()->obj()->attr[attr];
	else // SET	!! SETS OF SIMPLES
		(*dstack_p) = meta->set()->first_item_of_set()->single()->obj()->attr[attr];

	(*dstack_p++).str.dynamic_flags |= PROTECTED;

	return 1;
}

/**
 * @brief Execution of code PUSHO. Store the pointer of an object (at a certain position) in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::pusho_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	MetaObj *meta;
 
	if (mem == LEFT_MEM)
		meta = (*data.left)[(int)pos];
	else
		meta = (*data.right)[(int)pos];

	if (meta->class_type() == SINGLE)
		(*dstack_p++).num = (long)meta->single()->obj();
	else
	{
		BTState *st = new BTState;
		*st = meta->set()->get_tree()->getIterator();
		(*dstack_p++).num =(long)st;
	}
	return 1;
}

/**
 * @brief Execution of code PUSHT. Store the time of a certain object in the stack
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 1 execution continues
 */
int Node::pusht_call(Node *node, ExecData &data)
{
	code_p++;
	ULong mem = ((*code_p) >> 15) & 0x1;
	ULong pos = ((*code_p) >> 8) & 0x7F;
	ULong attr = (*code_p++) & 0xFF;
	MetaObj *meta;
 
	if (mem == LEFT_MEM)
		meta = (*data.left)[(int)pos];
	else
		meta = (*data.right)[(int)pos];
 
	// meta debe ser single (t1==t2)
	(*dstack_p++).num = meta->t1();
	return 1;
}

/**
 * INTER NODES OPERATIONS
 *
 * In an INTER node, when a object arrives by some of the sides, is checked against all the object that came 
 * by the other side previously. Every couple made is tested against the conditions in the node (code). Those couples 
 * that satisfied the code will progress down to the children nodes.
 * The couples are made by means of the Compound MetaObjs that are able to join an MetaObj by right and another MetaObj by left.
 * Further INTER nodes will make complex structures with Compounds with other Compounds as children
 */

/**
 * @brief Execution of code AND.
 * 		The couples that verify the node's code progress toward its children nodes
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 0 execution does not continue any more (the propagation continued with the couples found)
 */
int Node::and_call(Node *node, ExecData &data)
{

	if (checkingScope)
		return NODE_REACHED;

	if (data.side == LEFT_MEM)
		node->and_call_by_left(data);
	else
		node->and_call_by_right(data);

	return(0);
}

/**
 * @brief Execution of code AND by left side.
 * 		The object that arrived is firstable stored in the memory of its side, then the conditions 
 * 		are tried between it and the items to the other side.
 * 		The couples that verify the node's code progress toward its children nodes		
 * 		-- KeyManager makes the selections of the elements of the other side incredibly faster due 
 * 		   conditions has been transformed in ordering criteria for the multilevel BTree ---
 * 
 * @param data Execution data
 */
void Node::and_call_by_left(ExecData &data)
{
	int old_tag = data.tag;
	
	if (store_in_and_node_by_LEFT(data))
	{
		MetaObj *item;
		BTree *tree = (BTree *)code_p[AND_NODE_MEM_START_POS + 1];
		KeyManager keyman(RIGHT_MEM, LEFT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_AND_NODE);
		tree->setKeyManager(&keyman);

		BTState state = tree->FindByKeys(data.left);
		while (item= (MetaObj *)BTree::Walk(state))
			check_and_cond(item, data);
	}

	data.tag=old_tag;

}

/**
 * @brief Execution of code AND by right side.
 * 		The object that arrived is firstable stored in the memory of its side, then the conditions 
 * 		are tried between it and the items to the other side.
 * 		The couples that verify the node's code progress toward its children nodes		
 * 		-- KeyManager makes the selections of the elements of the other side incredibly faster due 
 * 		   conditions has been transformed in ordering criteria for the multilevel BTree ---
 * 
 * @param data Execution data
 */
void Node::and_call_by_right(ExecData &data)
{
	int old_tag = data.tag;
	data.right = data.left;

	if (store_in_and_node_by_RIGHT(data))
	{
		MetaObj *item;
		BTree *tree = (BTree *)code_p[AND_NODE_MEM_START_POS];
		KeyManager keyman(LEFT_MEM, RIGHT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_AND_NODE);
		tree->setKeyManager(&keyman);

		BTState state = tree->FindByKeys(data.right);
		while (item= (MetaObj *)BTree::Walk(state))
			check_and_cond(item, data);
	}

	data.left = data.right;
	data.tag=old_tag;
}

/**
 * @brief Execution of the conditions that every couple made in the AND node must satisfy.
 * 		Be aware that in case of modification the conditions are checked with the OLD state of the object and with NEW state.
 * 		If in both cases the conditions have been satisfied the propagation continues with MODIFY_TAG. If only was in one state
 * 		the propagations will continue as RETRACT_TAG (old), or INSERT_TAG (new) 
 * 
 * @param item The couple (Compound) made
 * @param data Execution data
 */
void Node::check_and_cond(MetaObj *item, ExecData &data)
{

	int new_tag;
	MetaObj *LeftItem, *RightItem, *MainItem;
	int res, res2;
	ULong *begin_and, *end_and;
	int change_to_NEW = FALSE;
	ULong flags;

	if (data.side == LEFT_MEM)
	{
		MainItem = data.left;
		data.right = (MetaObj *)item;
	}
	else
	{
		MainItem = data.right;
		data.left=(MetaObj *)item;
	}

	begin_and	= code_p;
	code_p		= begin_and + LEN_AND_NODE + begin_and[AND_NODE_NKEYS_POS];
	end_and 	= code_p + begin_and[AND_NODE_CODELEN_POS];
	flags		= begin_and[AND_NODE_FLAGS_POS];

	new_tag = data.tag;

	res=1;
	while ( res>0 && code_p != end_and )
	{
		res = (*((IntFunction)(*code_p)))(this, data);
	}
	
	if (data.tag == MODIFY_TAG)
	{
		// Let's try with the old Object
 
		MainItem->set_state(OLD_ST, data.st, data.pos);
	
		code_p = begin_and + LEN_AND_NODE + begin_and[AND_NODE_NKEYS_POS];

		res2=1;
		while ( res2>0 && code_p != end_and )
		{
			res2 = (*((IntFunction)(*code_p)))(this, data);
		}
	
		new_tag = ((res2<<1) | res); 	// 1,1 -> 0x3 -> Modify
										// 1,0 -> 0x2 -> Retract
										// 0,1 -> 0x1 -> Insert
	
		if (new_tag != RETRACT_TAG)
			MainItem->set_state(NEW_ST, data.st, data.pos);
		else
			change_to_NEW = TRUE;
	
	} else res2=0;
	
	if (res>0 || res2>0)
	{
		int n_objs_left =	(int)((begin_and[AND_NODE_N_ITEMS_POS]) >> 16);
		MetaObj * NewLeftItem;
		long t1, t2;

		data.left->comp_window(data.right, t1, t2, (flags & (IS_TIMED<<16)), (flags & IS_TIMED));
		NewLeftItem = new Compound(data.left, data.right, t1, t2);

		if (NewLeftItem == NULL)
			engine_fatal_err("new Compound: %s\n", strerror(errno));

		ExecData new_data(*(data.st), NewLeftItem, (MetaObj *)NULL, new_tag, LEFT_MEM, data.pos);

		if (new_data.side == RIGHT_MEM && new_data.pos>=0)
			new_data.pos += n_objs_left;

		if (trace >= 2)
		{
			fprintf(trace_file,  "AND : PROPAGATION SIDE = %d TAG = %d\nOBJ = ", new_data.side, new_data.tag);
			NewLeftItem->print(stdout, print_objkey);
			fprintf(trace_file,  "\n");
		}

		propagate(new_data, end_and);

		new_data.left->unlink();
	}

	code_p = begin_and;

	if (change_to_NEW)
		MainItem->set_state(NEW_ST, data.st, data.pos);
}

/**
 * @brief Execution of code NAND (negated AND A & !B).
 * 		NAND Nodes are to search those L that there are not any R such as L and R verify some conditions
 * 		By left side if there is no matching with R it progress in the same tag INSERT or RETRACT (Modify is a mix of both)
 * 		By right side if in INSERT and find any L, propagate L in RETRACT
 * 		if in by right with RETRACT and before it was verifying with some L it can continue with INSERT if there is no more matching that L 
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 0 execution does not continue any more (the propagation continued internally)
 */
int Node::nand_call(Node *node, ExecData &data)
{

	if (checkingScope)
		return NODE_REACHED;

	if (data.side == LEFT_MEM)
		node->nand_call_by_left(data);
	else
		node->nand_call_by_right(data);

	return(0);
}

/**
 * @brief Execution of code NAND by left side.
 * 		The object that arrived is firstable stored in the memory of its side, then the conditions 
 * 		are tried between it and the items to the other side.
 * 		If there is no couples that verify the node's code, then the left Object progress toward its children nodes		
 * 
 * @param data Execution data
 */
void Node::nand_call_by_left(ExecData &data)
{
	int tag_mask=0;
	MatchCount *counter;
	ULong flags;
	BTree *tree = (BTree *)code_p[AND_NODE_MEM_START_POS + 1];
	KeyManager keyman(RIGHT_MEM, LEFT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_AND_NODE, true);
	tree->setKeyManager(&keyman);

	flags = code_p[AND_NODE_FLAGS_POS]>>16;

	int old_tag = data.tag;

	if (store_in_asym_node_by_LEFT(data, counter))
	{
		// In INSERT or MODIFY the counter must be calculated 
		// In RETRACT the counter is taken with no extra tests 

		if (data.tag != RETRACT_TAG)
		{
			MetaObj *item;
			BTState state = tree->FindByKeys(data.left);
			while (item= (MetaObj *)BTree::Walk(state))
			{
				check_nand_cond(counter, item, data, &tag_mask);
			}
		}
		else tag_mask = 0;

		// check_nand_cond returns 0 if there is no objets at right side that satisfy the conditions, or the current tag if any verify them,
		// except in MODIFY_TAG that can also return INSERT_TAG when condition are verified only in NEW_STate or RETRACT_TAG when they are 
		// verified only in OLD_STate, with some object of the right side. 
		// 
		// So, this is the table of what tag must be applied in each case:
		//
		//	Initial tag				Returned Tag (mask)			Tag to apply
		// -------------			----------------------		--------------
		//	INSERT_TAG						0	(no match)		    INSERT_TAG		Maintain tag
		//	RETRACT_TAG						0						RETRACT_TAG		Maintain tag
		//	MODIFY_TAG						0						MODIFY_TAG	    Maintain tag
		//	INSERT_TAG					INSERT_TAG						0			No propagation
		//	RETRACT_TAG					RETRACT_TAG						0			No propagation
		//	MODIFY_TAG					INSERT_TAG					RETRACT_TAG		Propagate RETRACT
		//	MODIFY_TAG					RETRACT_TAG					INSERT_TAG		Propagate INSERT
		//	MODIFY_TAG					MODIFY_TAG						0 			No propagation
 

		data.tag ^= tag_mask;	// The tag to apply is the XOR of the returned mask applied to the original tag !!

		if (counter->count == 0 && data.tag != 0) // it is the same!!
		{

			if (trace >= 2)
			{
				fprintf(trace_file,  "NAND : PROPAGATION SIDE = %d TAG = %d\nOBJ = ", data.side, data.tag);
				data.left->print(stdout, print_objkey);
				fprintf(trace_file,  "\n");
			}

			ULong *end_and = code_p + LEN_AND_NODE + code_p[AND_NODE_NKEYS_POS] + code_p[AND_NODE_CODELEN_POS];
	
			// In NAND nodes only is propagated the left side
			propagate(data, end_and);

		}
	}

	if (data.tag == RETRACT_TAG || (flags & IS_TRIGGER))
		delete counter;

	data.tag=old_tag;
}

/**
 * @brief Execution of code NAND by right side.
 * 		The object that arrived is firstable stored in the memory of its side, then the conditions 
 * 		are tried between it and the items to the other side.
 * 		If there is no couples that verify the node's code, nothing is done
 * 		If in INSERT when the confitions are verified with some node of the left that was propagate, then a RETRACT is forwarded
 * 		When in RETRACT and it was the unique that verified the conditions with some node of the left, then an INSERT is forwarded
 * 
 * 		To know exactly how many matches makes any object by right, the BTree stores counters that makes the effort extremely fast
 * 
 * @param data Execution data
 */

void Node::nand_call_by_right(ExecData &data)
{

	int old_tag = data.tag;
	data.right = data.left;

	if (store_in_and_node_by_RIGHT(data))
	{
		MatchCount *item;
		BTree *tree = (BTree *)code_p[AND_NODE_MEM_START_POS];
		KeyManager keyman(LEFT_MEM, RIGHT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_AND_NODE, true);
		tree->setKeyManager(&keyman);
		BTState state = tree->FindByKeys(data.right);
		while (item= (MatchCount *)BTree::Walk(state))
			check_nand_cond(item, data.right, data, NULL);	// The propagation, in case of RIGTH side, is done in this function
	}

	data.left = data.right;
	data.tag=old_tag;

}

/**
 * @brief Checks if the conditions of the node are satisfied between an object at left (MatchCounter) and an object by right (MetaObj)
 * 		A MatchCount is formed by a counter of matching couples and a link to the MetaOBj
 * 
 * @param left MatchCount by right
 * @param right Object by right
 * @param data Execution data
 * @param result_tag the resulting tag that must be applied
 */
void Node::check_nand_cond(MatchCount *left, MetaObj *right, ExecData &data, int *result_tag)
{

	int old_tag, new_tag;
	MetaObj *MainItem;
	int res, res2;
	ULong *begin_and, *end_and;
	int change_to_NEW = FALSE;

	if (data.side == LEFT_MEM)
	{
		MainItem = data.left;
		data.right = right;
	}
	else
	{
		data.left = left->item;
		MainItem = data.right;
	}

	begin_and	= code_p;
	code_p		= begin_and + LEN_AND_NODE + begin_and[AND_NODE_NKEYS_POS];
	end_and		= code_p + begin_and[AND_NODE_CODELEN_POS];
	
	res=1;
	while ( res>0 && code_p != end_and )
	{
		res = (*((IntFunction)(*code_p)))(this, data);
	} 

	new_tag = old_tag = data.tag;
	if (new_tag == MODIFY_TAG)
	{
		// Let's try with the old Object
		
		MainItem->set_state(OLD_ST, data.st, data.pos);

		code_p	= begin_and + LEN_AND_NODE + begin_and[AND_NODE_NKEYS_POS];

		res2=1;
		while ( res2>0 && code_p != end_and )
		{
			res2 = (*((IntFunction)(*code_p)))(this, data);
		}

		new_tag = ((res2<<1) | res); 	// 1,1 -> 0x3 -> Modify
										// 1,0 -> 0x2 -> Retract
										// 0,1 -> 0x1 -> Insert
		if (new_tag != RETRACT_TAG)
			MainItem->set_state(NEW_ST, data.st, data.pos);
		else
			change_to_NEW = TRUE;

	} else res2 = res;

	if ((new_tag & INSERT_TAG) == INSERT_TAG && res == 1)
		left->count++;

	if ((res>0 || res2>0) && data.side == RIGHT_MEM && left->count == 1)
	{
		//	Initial Tag				current Tag				Tag to apply
		// -------------			-------------------		--------------
		//	INSERT_TAG				INSERT_TAG				RETRACT_TAG
		//	RETRACT_TAG				RETRACT_TAG				INSERT_TAG
		//	MODIFY_TAG				INSERT_TAG				RETRACT_TAG
		//	MODIFY_TAG				RETRACT_TAG				INSERT_TAG
		//	MODIFY_TAG				MODIFY_TAG				0 nothing to do, no propagation
 
		// If the object arrived by the right side, position is -1


		new_tag ^= 0x3;		// RETACT=10b, INSERT=01b, MODIFY=11b
							// The final tag is the XOR (invert) of the last two bits
 
		if (new_tag != 0)
		{
			// In NAND only must be propagated the left side
			ExecData new_data((*data.st), data.left, NULL, new_tag, LEFT_MEM, -1);

			if (trace >= 2)
			{
				fprintf(trace_file,  "NAND : PROPAGATION SIDE = %d TAG = %d\nOBJ = ", new_data.side, new_data.tag);
				new_data.left->print(stdout, print_objkey);
				fprintf(trace_file,  "\n");
			}
 
			propagate(new_data, end_and);
		}

	}

	if ((old_tag & RETRACT_TAG) == RETRACT_TAG && res2 == 1)
		left->count--;
 
	// Only is set if a matching was done
	if (result_tag && (res>0 || res2>0))
		*result_tag = new_tag;

	code_p = begin_and;

	if (change_to_NEW)
		MainItem->set_state(NEW_ST, data.st, data.pos);
}

/**
 * @brief Execution of code OAND (Optional AND).
 * 		OAND Nodes work very similar to AND nodes, the unique difference is that on an arrival of an object by left side
 * 		the propagations continues even if no matching are found (with a couple with null value to its right ride)
 * 		This nodes are very useful when the final context of rules are important and you want to get all the related information that
 * 		provide evidence to your inference. In this case some evidence may not be present in all cases but, if they are, you want to
 * 		get it, to consider.
 * 		Imagine you are getting alarms from many interconnected systems and you want to identify the real cause/fault. You are interested in 
 * 		the alarms you are considering as evidence because you are going to present only the final faults and all the alarms that each fault
 * 		is producing in the different systems, it is very possible that some alarms may be optional, but you want them in case they triggered !! 
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 0 execution does not continue any more (the propagation continued internally)
 */
int Node::oand_call(Node *node, ExecData &data)
{
	if (checkingScope)
		return NODE_REACHED;

	if (data.side == LEFT_MEM)
		node->oand_call_by_left(data);
	else
		node->oand_call_by_right(data);

	return(0);
}

/**
 * @brief Execution of code OAND by left side.
 * 		The object that arrived is firstable stored in the memory of its side, then the conditions 
 * 		are tried between it and the items to the other side.
 * 		The couples that verify the node's code progress toward its children nodes		
 * 		In case no matching is found, a couple with NULL as right side is propagated
 * 
 * @param data Execution data
 */
void Node::oand_call_by_left(ExecData &data)
{
	MatchCount *counter;
	int how_many;
	ULong flags;
	BTree *tree = (BTree *)code_p[AND_NODE_MEM_START_POS + 1];
	KeyManager keyman(RIGHT_MEM, LEFT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_AND_NODE, true);
	tree->setKeyManager(&keyman);
 
	flags = code_p[AND_NODE_FLAGS_POS]>>16;
 
	int old_tag = data.tag;

	if (store_in_asym_node_by_LEFT(data, counter))
	{
		// In RETRACTION we rely in the counter to propagate the Null Single 
		// by right in case the count is 0
		// Positive tags (INSERT or MODIFY) oblige to calculate this counter
		
		if (data.tag == RETRACT_TAG && counter->count == 0)
		{
			check_oand_cond(counter, Single::null_single(), data, NULL);
		}
		else
		{
			MetaObj *item;
			how_many=0;
			BTState state = tree->FindByKeys(data.left);
			while (item= (MetaObj *)BTree::Walk(state))
				check_oand_cond(counter, item, data, &how_many);

			if (how_many == 0)
				check_oand_cond(counter, Single::null_single(), data, NULL);
		}
	}

	if (data.tag == RETRACT_TAG || (flags & IS_TRIGGER))
		delete counter;

	data.tag=old_tag;
}

/**
 * @brief Execution of code OAND by right side.
 * 		The object that arrived is firstable stored in the memory of its side, then the conditions 
 * 		are tried between it and the items to the other side.
 * 		The couples that verify the node's code progress toward its children nodes		
 *
 * 		Essentially is quite similar to what is done in a AND node by right
 * 		There is a counter (MatchCount) for every object in the memory by left. It is updated with the matchings found
 * 
 * @param data Execution data
 */
void Node::oand_call_by_right(ExecData &data)
{
	int old_tag = data.tag;

	data.right = data.left;

	if (store_in_and_node_by_RIGHT(data))
	{
		MatchCount *counter;
		BTree *tree = (BTree *)code_p[AND_NODE_MEM_START_POS];
		KeyManager keyman(LEFT_MEM, RIGHT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_AND_NODE, true);
		tree->setKeyManager(&keyman);
		BTState state = tree->FindByKeys(data.right);
		while (counter= (MatchCount *)BTree::Walk(state))
			check_oand_cond(counter, data.right, data, NULL);
	}

	data.left = data.right;
	data.tag=old_tag;
}

/**
 * @brief Execution of the conditions that every couple made in the OAND node must satisfy.
 * 		Be aware that in case of modification the conditions are checked with the OLD state of the object and with NEW state.
 * 		If in both cases the conditions have been satisfied the propagation continues with MODIFY_TAG. If only was in one state
 * 		the propagations will continue as RETRACT_TAG (old), or INSERT_TAG (new) 
 * 
 * @param counter The MatchCount by left
 * @param item The MetaObj by right
 * @param data Execution data
 * @param how_many Number of matching found
 */
void Node::check_oand_cond(MatchCount *counter, MetaObj *item, ExecData &data,	int *how_many)
{

	int new_tag;
	MetaObj *MainItem;
	int res, res2;
	ULong *begin_and, *end_and;
	int change_to_NEW = FALSE;
	ULong flags;

	if (data.side == LEFT_MEM)
	{
		data.right = item;
		MainItem	= data.left;
	}
	else
	{
		data.left = counter->item;
		MainItem	= data.right;
	}

	begin_and	= code_p;
	code_p		= begin_and + LEN_AND_NODE + begin_and[AND_NODE_NKEYS_POS];
	end_and 	= code_p + begin_and[AND_NODE_CODELEN_POS];
	flags		= begin_and[AND_NODE_FLAGS_POS];
	
	new_tag = data.tag;

	res=1;
	if (data.right != Single::null_single())
	{
		while ( res>0 && code_p != end_and )
		{
			res = (*((IntFunction)(*code_p)))(this, data);
		}
	
		if (new_tag == MODIFY_TAG)
		{
			// Let's try with the old object
	
			MainItem->set_state(OLD_ST, data.st, data.pos);
	
			code_p	= begin_and + LEN_AND_NODE + begin_and[AND_NODE_NKEYS_POS];
	
			res2=1;
			while ( res2>0 && code_p != end_and )
			{
				res2 = (*((IntFunction)(*code_p)))(this, data);
			}
	
			new_tag = ((res2<<1) | res); 	// 1,1 -> 0x3 -> Modify
											// 1,0 -> 0x2 -> Retract
											// 0,1 -> 0x1 -> Insert
	
			if (new_tag != RETRACT_TAG)
				MainItem->set_state(NEW_ST, data.st, data.pos);
			else
				change_to_NEW = TRUE;
	
		}
		else res2=0;
	}
	else res2=0;
	
	if (res>0 || res2>0)
	{
		int new_pos = data.pos;
		int n_objs_left =	(int)((begin_and[AND_NODE_N_ITEMS_POS]) >> 16);
		MetaObj *NewLeftItem, *RightItem;
		Status new_st(*(data.st));
		long t1, t2;

		RightItem = data.right;

		if (RightItem != Single::null_single())
		{
			int prev_tag = new_tag;

			if (prev_tag == INSERT_TAG)
				counter->count ++;

			// Only in the transition from count=0 to 1 the propagation is as modification
			// in the rest of cases the tag is maintained

			if (data.side == RIGHT_MEM && counter->count == 1)
			{
 
				new_tag = MODIFY_TAG;

				if (prev_tag == INSERT_TAG)
				{
					new_st._old_single = Single::null_single();
					new_st._single = data.right->single();
				}

				if (prev_tag == RETRACT_TAG)
				{
					new_st._old_single = data.right->single();
					new_st._single = Single::null_single();
					RightItem = Single::null_single();
				}
			}

			if (prev_tag == RETRACT_TAG)
				counter->count --;

		}

		data.left->comp_window(RightItem, t1, t2, (flags & (IS_TIMED<<16)), (flags & IS_TIMED));
		NewLeftItem = new Compound(data.left, RightItem, t1, t2);
 
		if (NewLeftItem == NULL)
			engine_fatal_err("new Compound: %s\n", strerror(errno));
 
		ExecData new_data(new_st, NewLeftItem, (MetaObj *)NULL , new_tag, LEFT_MEM, data.pos);

		if (data.side == RIGHT_MEM)
			new_data.pos += n_objs_left;

		if (trace >= 2)
		{
			fprintf(trace_file,  "OAND : PROPAGATION SIDE = %d TAG = %d\nOBJ = ", new_data.side, new_data.tag);
			NewLeftItem->print(stdout, print_objkey);
			fprintf(trace_file,  "\n");
		}

		propagate(new_data, end_and);

		new_data.left->unlink();

		if (how_many) (*how_many)++;

	}

	code_p = begin_and;

	if (change_to_NEW)
		MainItem->set_state(NEW_ST, data.st, data.pos);
}

/**
 * @brief Store Control in the memory (BTree) an AND node by LEFT
 * 		if stored the MetaObj is linked (increments the #links by 1), if removed it is unlinked (decrements #links by 1)
 * 
 * @param data Execution data
 * @return int If the inference must continue
 */
int Node::store_in_and_node_by_LEFT(ExecData &data)
{
	// LEFT MEMORY = (void**)(code_p + AND_NODE_MEM_START_POS)
	// RIGHT MEMORY	= (void**)(code_p + AND_NODE_MEM_START_POS+1)

	int continue_inference;
	MetaObj * LeftItemInMem;
	ULong flags;
	int WasFound;
	BTree *tree = (BTree *)code_p[AND_NODE_MEM_START_POS];
	KeyManager keyman(LEFT_MEM, LEFT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_AND_NODE);
	tree->setKeyManager(&keyman);

	flags = code_p[AND_NODE_FLAGS_POS]>>16;		//Left side Flags

	switch (data.tag)
	{
		case INSERT_TAG:
			
			// IS_TRIGGER flags imply that no storage is done

			if (!(flags & IS_TRIGGER))
			{
				LeftItemInMem = *(MetaObj **)tree->Insert(data.left, MetaObj::metacmp);

				// The object is linked if it was stored (was not found)
				// also the inference continues due it is something new
				if (continue_inference = !BTree::WasFound()){
					data.left->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_LEFT\n");
				}
			}
			else continue_inference = TRUE;

			break;
		case MODIFY_TAG:
			if (!(flags & IS_TRIGGER))
			{
				int keys_mod;
				keys_mod = keyman.keysModified(data.pos, data.st->_obj, data.st->_old_obj);
				data.left->set_state(OLD_ST, data.st, data.pos);
				if (keys_mod)
					LeftItemInMem = (MetaObj *)tree->Delete(data.left, MetaObj::metacmp);
				else
					LeftItemInMem = (MetaObj *)tree->Find(data.left, MetaObj::metacmp);
 
				if (LeftItemInMem)
				{
					if (LeftItemInMem != data.left)
					{
						data.left->unlink();
						data.left = LeftItemInMem;
						data.left->link();			// this way al least the object will have one link more
													// that will decrease at do_loop
						if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_LEFT\n");
					} 
				}
				else
				{
					data.tag = INSERT_TAG;
					data.left->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_LEFT\n");
				}
				data.left->set_state(NEW_ST, data.st, data.pos);
				if (!LeftItemInMem || keys_mod)
					tree->Insert(data.left, MetaObj::metacmp);

				continue_inference = TRUE;
			}
			else continue_inference = FALSE;		// The triggers are onlypropagated on INSERT
			break;
		case RETRACT_TAG: 
			if (!(flags & IS_TRIGGER))
			{
				LeftItemInMem = (MetaObj *)tree->Delete(data.left, MetaObj::metacmp);

				continue_inference = (LeftItemInMem != NULL);

				// We delete the arrived compound and replace it by the old copy in the memory
				// We have to work with it from now on
			
				if (continue_inference && LeftItemInMem != data.left)
				{
					data.left->unlink();
					data.left = LeftItemInMem;
					data.left->set_state(OLD_ST, data.st, data.pos);
					data.left->link();				// this way al least the object will have one link more
													// that will decrease at do_loop
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_LEFT\n");
				}

				if (continue_inference)
					data.left->unlink();

			}
			else continue_inference = FALSE;		// The triggers are only propagated on INSERT
			break;
	}
	return continue_inference;
}

/**
 * @brief Store Control in the memory (BTree) an AND node by RIGHT
 * 		if stored the MetaObj is linked (increments the #links by 1), if removed it is unlinked (decrements #links by 1)
 * 
 * @param data Execution data
 * @return int If the inference must continue
 */
int Node::store_in_and_node_by_RIGHT(ExecData &data)
{
	// LEFT MEMORY = (void**)(code_p + AND_NODE_MEM_START_POS)
	// RIGHT MEMORY	= (void**)(code_p + AND_NODE_MEM_START_POS+1)

	int continue_inference;
	MetaObj * RightItemInMem;
	ULong flags;
	int WasFound;
	BTree *tree = (BTree *)code_p[AND_NODE_MEM_START_POS + 1];
	KeyManager keyman(RIGHT_MEM, RIGHT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_AND_NODE);
	tree->setKeyManager(&keyman);

	flags = code_p[AND_NODE_FLAGS_POS] & 0xFFFF;	// Right side Flags


	switch (data.tag)
	{
		case INSERT_TAG:
			
			// IS_TRIGGER flags imply that no storage is done

			if (!(flags & IS_TRIGGER))
			{
				RightItemInMem = *(MetaObj **)tree->Insert(data.right, MetaObj::metacmp);
 
				// The object is linked if it was stored (was not found)
				// also the inference continues due it is something new
				if (continue_inference = !BTree::WasFound()){
					data.right->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_RIGHT\n");
				}
			}
			else continue_inference = TRUE;

			break;
		case MODIFY_TAG:
			if (!(flags & IS_TRIGGER))
			{
				int keys_mod;
				keys_mod = keyman.keysModified(data.pos, data.st->_obj, data.st->_old_obj);
				data.right->set_state(OLD_ST, data.st, data.pos);
				if (keys_mod)
					RightItemInMem = (MetaObj *)tree->Delete(data.right, MetaObj::metacmp);
				else
					RightItemInMem = (MetaObj *)tree->Find(data.right, MetaObj::metacmp);
 
				if (RightItemInMem)
				{
					if (RightItemInMem != data.right)
					{
						data.right->unlink();
						data.right = RightItemInMem;
						data.right->link();			// this way al least the object will have one link more
													// that will decrease at do_loop
						if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_RIGHT\n");
					} 
				}
				else
				{
					data.tag = INSERT_TAG;
					data.right->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_RIGHT\n");
				}
				data.right->set_state(NEW_ST, data.st, data.pos);
				if (!RightItemInMem || keys_mod)
					tree->Insert(data.right, MetaObj::metacmp);

				continue_inference = TRUE;
			}
			else continue_inference = FALSE;		// The triggers are onlypropagated on INSERT
			break;
		case RETRACT_TAG: 

			if (!(flags & IS_TRIGGER))
			{
				RightItemInMem = (MetaObj *)tree->Delete(data.right, MetaObj::metacmp);

				continue_inference = (RightItemInMem != NULL);

				// We delete the arrived compound and replace it by the old copy in the memory
				// We have to work with it from now on

				if (continue_inference && RightItemInMem != data.right)
				{
					data.right->unlink();
					data.right = RightItemInMem;
					data.right->set_state(OLD_ST, data.st, data.pos);
					data.right->link();				// this way al least the object will have one link more
													// that will decrease at do_loop
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_RIGHT\n");
				}

				if (continue_inference)
					data.right->unlink();

			} else continue_inference = FALSE;		// The triggers are onlypropagated on INSERT

			break;
	}
	return continue_inference;
}

/**
 * @brief Store Control in the memory (BTree) an NAND od OAND nodes by LEFT (asymmetric nodes)
 * 		In this nodes a MatchCount is maintained with the number of matchings with the right side
 * 		if stored the MetaObj is linked (increments the #links by 1), if removed it is unlinked (decrements #links by 1)
 * 
 * @param data Execution data
 * @param count The counter of matchings
 * @return int If the inference must continue
 */
int Node::store_in_asym_node_by_LEFT(ExecData &data, MatchCount *&count)
{
	// LEFT MEMORY = (void**)(code_p + AND_NODE_MEM_START_POS)
	// RIGHT MEMORY	= (void**)(code_p + AND_NODE_MEM_START_POS+1)
	
	int continue_inference;
	MatchCount * LeftItemInMem;
	MetaObj **LeftItemInMem_p;
	ULong flags;
	BTree *tree = (BTree *)code_p[AND_NODE_MEM_START_POS];
	KeyManager keyman(LEFT_MEM, LEFT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_AND_NODE, true);
	tree->setKeyManager(&keyman);

	flags = code_p[AND_NODE_FLAGS_POS]>>16;		// Right side Flags

	switch (data.tag)
	{
		case INSERT_TAG:
			
			// IS_TRIGGER flags imply that no storage is done

			if (!(flags & IS_TRIGGER))
			{
				LeftItemInMem_p = (MetaObj **)tree->Insert(data.left, (BTCompareFunc)cmp_count_with_object);

				// The object is linked if it was stored (was not found)
				// also the inference continues due it is something new
				if (continue_inference = (LeftItemInMem_p!= NULL && !BTree::WasFound()))
				{
					LeftItemInMem = *((MatchCount **)LeftItemInMem_p) = new MatchCount(data.left);
					data.left->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_asym_node_by_LEFT\n");
				}
			}
			else
			{
				continue_inference = TRUE;
				LeftItemInMem = new MatchCount(data.left);
			}

			break;
		case MODIFY_TAG:
			if (!(flags & IS_TRIGGER))
			{
				int keys_mod;
				keys_mod = keyman.keysModified(data.pos, data.st->_obj, data.st->_old_obj);
				data.left->set_state(OLD_ST, data.st, data.pos);
				if (keys_mod)
					LeftItemInMem = (MatchCount *)tree->Delete(data.left, (BTCompareFunc)cmp_count_with_object);
				else
					LeftItemInMem = (MatchCount *)tree->Find(data.left, (BTCompareFunc)cmp_count_with_object);
 
				if (LeftItemInMem)
				{
					if (LeftItemInMem->item != data.left)
					{
						data.left->unlink();
						data.left = LeftItemInMem->item;
						data.left->link();			// this way al least the object will have one link more
													// that will decrease at do_loop
						if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_LEFT\n");
					} 
				}
				else
				{
					data.tag = INSERT_TAG;
					data.left->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_LEFT\n");
				}
				data.left->set_state(NEW_ST, data.st, data.pos);
				if (!LeftItemInMem || keys_mod)
				{
					LeftItemInMem_p = (MetaObj **)tree->Insert(data.left, (BTCompareFunc)cmp_count_with_object);
					if (!LeftItemInMem)
						LeftItemInMem = *((MatchCount **)LeftItemInMem_p) = new MatchCount(data.left);
					else 
						*((MatchCount **)LeftItemInMem_p) = LeftItemInMem;
				}

				continue_inference = TRUE;
			}
			else continue_inference = FALSE;	// The triggers are onlypropagated on INSERT
			break;
		case RETRACT_TAG: 
			if (!(flags & IS_TRIGGER))
			{
				LeftItemInMem = (MatchCount *)tree->Delete(data.left, (BTCompareFunc)cmp_count_with_object);

				continue_inference = (LeftItemInMem != NULL);

				// We delete the arrived compound and replace it by the old copy in the memory
				// We have to work with it from now on
				if (continue_inference && LeftItemInMem->item != data.left)
				{
					data.left->unlink();
					data.left = LeftItemInMem->item;
					data.left->set_state(OLD_ST, data.st, data.pos);
					data.left->link();				// this way al least the object will have one link more
													// that will decrease at do_loop
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_asym_node_by_LEFT\n");

				}

				if (continue_inference)
					data.left->unlink();

			}
			else continue_inference = FALSE;	// The triggers are only propagated on INSERT
			break;
	}

	count = LeftItemInMem;
	return continue_inference;
}

/**
 * @brief Execution of code WAND.
 * 		The couples must verify not only node's code but the objects are inside the rule's time window.
 * 		Then, they progress toward its children nodes
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 0 execution does not continue any more (the propagation continued internally)
 */
int Node::wand_call(Node *node, ExecData &data)
{

	if (checkingScope)
		return NODE_REACHED;

	if (data.side == LEFT_MEM)
		node->wand_call_by_left(data);
	else
		node->wand_call_by_right(data);

	return(0);
}

/**
 * @brief Execution of code WAND by left side.
 * 		The object that arrived is firstable stored in the memory of its side, and then a selection of the 
 * 		the objects inside rule's time window are selected and tried the conditions of the node.
 * 		The couples that verify the node's code progress toward its children nodes		
 * 		-- KeyManager makes the selections of the elements of the other side incredibly faster due 
 * 		   conditions has been transformed in ordering criteria for the multilevel BTree ---
 * 		The first order criteria is the time
 * 
 * @param data Execution data
 */
void Node::wand_call_by_left(ExecData &data)
{
	BTree *tree	= (BTree *)code_p[WAND_NODE_MEM_START_POS + 1];
	long window = (long)code_p[WAND_NODE_WTIME_POS];
	ULong this_flags	= code_p[AND_NODE_FLAGS_POS] >> 16;
	ULong other_flags = code_p[AND_NODE_FLAGS_POS] & 0xFFFF; 

	int old_tag = data.tag;

	if (store_in_wand_node_by_LEFT(data))
	{
		BTState state;
		MetaObj *item;

		KeyManager keyman(RIGHT_MEM, LEFT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_WAND_NODE);
		tree->setKeyManager(&keyman);

		// If the timing is active between the objects we find objects with t2< t1 + window
		// else we find simply by key
		if ((this_flags & IS_TIMED) && (other_flags & IS_TIMED))
			state = tree->FindBiggerThan(data.left, MetaObj::compare_tw, data.left->t1() + window + 1);
		else
			state = tree->FindByKeys(data.left);

		while (item= (MetaObj *)BTree::Walk(state))
		{
			long t1, t2;
			data.left->comp_window(item, t1, t2, (this_flags & IS_TIMED), (other_flags & IS_TIMED));
			if (t2 - t1 <= window)
				check_wand_cond(item, data);
			else if ((other_flags & IS_TEMPORAL) && item->class_type() == SINGLE)
				push_to_remove_old_item(item->single(), RIGHT_MEM);
		}
	}

	data.tag=old_tag;
}

/**
 * @brief Execution of code WAND by right side.
 * 		The object that arrived is firstable stored in the memory of its side, and then a selection of the 
 * 		the objects inside rule's time window are selected and tried the conditions of the node.
 * 		The couples that verify the node's code progress toward its children nodes		
 * 		-- KeyManager makes the selections of the elements of the other side incredibly faster due 
 * 		   conditions has been transformed in ordering criteria for the multilevel BTree ---
 * 		The first order criteria is the time
 * 
 * @param data Execution data
 */
void Node::wand_call_by_right(ExecData &data)
{
	
	BTree *tree = (BTree *)code_p[WAND_NODE_MEM_START_POS];
	long window = (long)code_p[WAND_NODE_WTIME_POS];
	ULong this_flags	= code_p[AND_NODE_FLAGS_POS] & 0xFFFF;
	ULong other_flags	= code_p[AND_NODE_FLAGS_POS] >> 16;

	int old_tag = data.tag;
	data.right = data.left;

	if (store_in_wand_node_by_RIGHT(data))
	{
		MetaObj *item;
		BTState state;

		KeyManager keyman(LEFT_MEM, RIGHT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_WAND_NODE);
		tree->setKeyManager(&keyman);

		// If the timing is active between the objects we find objects with t2< t1 + window
		// else we find simply by key
		if ((this_flags & IS_TIMED) && (other_flags & IS_TIMED))
			state = tree->FindBiggerThan(data.right, MetaObj::compare_tw, data.right->t1() + window + 1);
		else
			state = tree->FindByKeys(data.right);

		while (item= (MetaObj *)BTree::Walk(state))
		{
			long t1, t2;
			data.right->comp_window(item, t1, t2, (this_flags & IS_TIMED), (other_flags & IS_TIMED));
			if (t2 - t1 <= window)
				check_wand_cond(item, data);
			else if ((other_flags & IS_TEMPORAL) && item->class_type() == SINGLE)
			push_to_remove_old_item(item->single(), LEFT_MEM);
		}
	}

	data.left = data.right;
	data.tag=old_tag;
}

/**
 * @brief Execution of the conditions that every couple made in the WAND node must satisfy.
 * 		Be aware that in case of modification the conditions are checked with the OLD state of the object and with NEW state.
 * 		If in both cases the conditions have been satisfied the propagation continues with MODIFY_TAG. If only was in one state
 * 		the propagations will continue as RETRACT_TAG (old), or INSERT_TAG (new) 
 * 
 * 		it is quite similar to check_and_nodes but now the compound window is managed
 * 
 * @param item The couple (Compound) made
 * @param data Execution data
 */
void Node::check_wand_cond(MetaObj *item, ExecData &data)
{

	// MEMORIA IZQUIERDA = (void**)(code_p + WAND_NODE_MEM_START_POS) 
	// MEMORIA DERECHA	= (void**)(code_p + WAND_NODE_MEM_START_POS+1)

	int new_tag;
	MetaObj *LeftItem, *RightItem, *MainItem;
	int res, res2;
	ULong *begin_and, *end_and;
	int change_to_NEW = FALSE;
	ULong flags;

	if (data.side == LEFT_MEM)
	{
		MainItem = data.left;
		data.right = (MetaObj *)item;
	}
	else
	{
		MainItem = data.right;
		data.left=(MetaObj *)item;
	}

	begin_and	= code_p;
	code_p		= begin_and + LEN_WAND_NODE + begin_and[AND_NODE_NKEYS_POS];
	end_and		= code_p + begin_and[AND_NODE_CODELEN_POS];
	flags	   = begin_and[AND_NODE_FLAGS_POS];

	new_tag = data.tag;
	res=1;

	while ( res>0 && code_p != end_and )
	{
		res = (*((IntFunction)(*code_p)))(this, data);
	} 

	if (data.tag == MODIFY_TAG)
	{
		// Let's try with the old Object
 
		MainItem->set_state(OLD_ST, data.st, data.pos);

		code_p = begin_and + LEN_WAND_NODE + begin_and[AND_NODE_NKEYS_POS];
 
		res2=1;
		while ( res2>0 && code_p != end_and )
		{
			res2 = (*((IntFunction)(*code_p)))(this, data);
		}

		new_tag = ((res2<<1) | res); 	// 1,1 -> 0x3 -> Modify
										// 1,0 -> 0x2 -> Retract
										// 0,1 -> 0x1 -> Insert
 
		if (new_tag != RETRACT_TAG)
			MainItem->set_state(NEW_ST, data.st, data.pos);
		else
			change_to_NEW = TRUE;

	} else res2=0;

	if (res>0 || res2>0)
	{
		MetaObj * NewLeftItem;	
		int n_objs_left =	(int)((begin_and[AND_NODE_N_ITEMS_POS]) >> 16);
		long t1, t2;
	
		data.left->comp_window(data.right, t1, t2, (flags & (IS_TIMED<<16)), (flags & IS_TIMED));
		NewLeftItem = new Compound(data.left, data.right, t1, t2);

		if (NewLeftItem == NULL)
			engine_fatal_err("new Compound: %s\n", strerror(errno));

		ExecData new_data((*data.st), NewLeftItem, NULL, new_tag, LEFT_MEM, data.pos);

		if (new_data.side == RIGHT_MEM && new_data.pos>=0)
			new_data.pos += n_objs_left;
 
		if (trace >= 2)
		{
			fprintf(trace_file,  "WAND : PROPAGATION SIDE = %d TAG = %d\nOBJ = ", new_data.side, new_data.tag);
			NewLeftItem->print(stdout, print_objkey);
			fprintf(trace_file,  "\n");
		}

		propagate(new_data, end_and);

		new_data.left->unlink();
	}
	code_p = begin_and;

	if (change_to_NEW)
		MainItem->set_state(NEW_ST, data.st, data.pos);
}

/**
 * @brief Execution of code NWAND (Timed, Negated AND A & !B).
 * 		NWAND Nodes are to search those L that there are not any R such as L and R are inside the rule ime window and verify some conditions
 * 		By left side if there is no matching with R it progress in the same tag INSERT or RETRACT (Modify is a mix of both)
 * 		By right side if in INSERT and find any L, propagate L in RETRACT
 * 		if in by right with RETRACT and before it was verifying with some L it can continue with INSERT if there is no more matching that L 
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 0 execution does not continue any more (the propagation continued internally)
 */
int Node::nwand_call(Node *node, ExecData &data)
{

	if (checkingScope)
		return NODE_REACHED;

	if (data.side == LEFT_MEM)
		node->nwand_call_by_left(data);
	else
		node->nwand_call_by_right(data);

	return(0);
}

/**
 * @brief Execution of code NWAND by left side.
 * 		The object that arrived is firstable stored in the memory of its side, then the rule window is verified and 
 * 		the conditions are tried between it and the items to the other side.
 * 		If there is no couples that verify the node's code, then the left Object progress toward its children nodes		
 * 
 * @param data Execution data
 */
void Node::nwand_call_by_left(ExecData &data)
{
	BTree *tree	= (BTree *)code_p[WAND_NODE_MEM_START_POS + 1];
	long window = (long)code_p[WAND_NODE_WTIME_POS];
	ULong this_flags  = code_p[AND_NODE_FLAGS_POS] >> 16;		// Left flags!
	ULong other_flags = code_p[AND_NODE_FLAGS_POS] & 0xFFFF;	// Right_flags

	int tag_mask = 0;
	MatchCount *counter;
 
	int old_tag = data.tag;

	if (store_in_Wasym_node_by_LEFT(data, counter))
	{
		// In INSERT or MODIFY the counter must be calculated 
		// In RETRACT the counter is taken with no extra tests
		if (data.tag == INSERT_TAG || data.tag == MODIFY_TAG)
		{
			MetaObj *item;
			BTState state;
			KeyManager keyman(RIGHT_MEM, LEFT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_WAND_NODE, true);
			tree->setKeyManager(&keyman);

			// If the timing is active between the objects we find objects with t2< t1 + window
			// else we find simply by key
			if ((this_flags & IS_TIMED) && (other_flags & IS_TIMED))
				state = tree->FindBiggerThan(data.left, MetaObj::compare_tw, data.left->t1() + window + 1);
			else
				state = tree->FindByKeys(data.left);

			while (item= (MetaObj *)BTree::Walk(state))
			{
				long t1, t2;
				data.left->comp_window(item, t1, t2, (this_flags & IS_TIMED), (other_flags & IS_TIMED));
				if (t2 - t1 <= window)
					check_nwand_cond(counter, item, data, &tag_mask);
				else if ((other_flags & IS_TEMPORAL) && item->class_type() == SINGLE)
					push_to_remove_old_item(item->single(), RIGHT_MEM);
			}
		}
		// data.tag == RETRACT_TAG
		else tag_mask = 0;

		// check_nwand_cond returns 0 if there is no objets at right side that satisfy the conds, or the current tag if any verify them,
		// except in MODIFY_TAG that can also return INSERT_TAG when condition are verified only in NEW_STate or RETRACT_TAG when they are 
		// verified only in OLD_STate, with some object of the right side. 
		// 
		// So, this is the table of what tag must be applied in each case:
		//
		//	Initial tag				Returned Tag (mask)			Tag to apply
		// -------------			----------------------		--------------
		//	INSERT_TAG						0	(no match)		    INSERT_TAG		Maintain tag
		//	RETRACT_TAG						0						RETRACT_TAG		Maintain tag
		//	MODIFY_TAG						0						MODIFY_TAG	    Maintain tag
		//	INSERT_TAG					INSERT_TAG						0			No propagation
		//	RETRACT_TAG					RETRACT_TAG						0			No propagation
		//	MODIFY_TAG					INSERT_TAG					RETRACT_TAG		Propagate RETRACT
		//	MODIFY_TAG					RETRACT_TAG					INSERT_TAG		Propagate ISERT
		//	MODIFY_TAG					MODIFY_TAG						0 			No propagation
 
		data.tag ^= tag_mask;	// The tag to appy is the XOR of the returned mask applied to the original tag !!
 
		if (counter->count == 0 && data.tag != 0)
		{
 
			if (trace >= 2)
			{
					fprintf(trace_file,  "NWAND : PROPAGATION SIDE = %d, TAG = %d\nOBJ = ", LEFT_MEM, data.tag);
					data.left->print(stdout, print_objkey);
					fprintf(trace_file,  "\n");
			}
 
			ULong *end_and	= code_p + LEN_WAND_NODE + code_p[AND_NODE_NKEYS_POS] + code_p[AND_NODE_CODELEN_POS];
 
			// In NWAND nodes only is propagated the left side
			propagate(data, end_and);
		}

	}

	if (data.tag == RETRACT_TAG || (this_flags & IS_TRIGGER))
		delete counter;
 
	data.tag=old_tag;
}

/**
 * @brief Execution of code NWAND by right side.
 * 		The object that arrived is firstable stored in the memory of its side, then the rule time window is verified
 * 		and the conditions are tried between it and the items to the other side.
 * 		If there is no couples that verify the node's code, nothing is done
 * 		If in INSERT when the confitions are verified with some node of the left that was propagate, then a RETRACT is forwarded
 * 		When in RETRACT and it was the unique that verified the conditions with some node of the left, then an INSERT is forwarded
 * 
 * 		To know exactly how many matches makes any object by right, the BTree stores counters that makes the effort extremely fast
 * 
 * @param data Execution data
 */
void Node::nwand_call_by_right(ExecData &data)
{

	BTree *tree = (BTree *)code_p[WAND_NODE_MEM_START_POS];
	long window = (long)code_p[WAND_NODE_WTIME_POS];
	ULong this_flags	= code_p[AND_NODE_FLAGS_POS] & 0xFFFF;
	ULong other_flags	= code_p[AND_NODE_FLAGS_POS] >> 16;

	int old_tag = data.tag;
	data.right = data.left;

	if (store_in_wand_node_by_RIGHT(data))
	{
		BTState state;
		MatchCount *counter;

		KeyManager keyman(LEFT_MEM, RIGHT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_WAND_NODE, true);
		tree->setKeyManager(&keyman);

		// If the timing is active between the objects we find objects with t2< t1 + window
		// else we find simply by key
		if ((this_flags & IS_TIMED) && (other_flags & IS_TIMED))
			state = tree->FindBiggerThan(data.right, Node::cmp_count_with_object_tw, data.right->t1() + window + 1);
		else
			state = tree->FindByKeys(data.right);

		while (counter= (MatchCount *)BTree::Walk(state))
		{
			long t1, t2;
			data.right->comp_window(counter->item, t1, t2, (this_flags & IS_TIMED), (other_flags & IS_TIMED));
			if (t2 - t1 <= window)
				check_nwand_cond(counter, data.right, data, NULL);
			else if ((other_flags & IS_TEMPORAL) && counter->item->class_type() == SINGLE)
				push_to_remove_old_item(counter->item->single(), LEFT_MEM);
		}
	}

	data.left = data.right;
	data.tag=old_tag;
}

/**
 * @brief Checks if the conditions of the node are satisfied between an object at left (MatchCounter) and an object by right (MetaObj),
 * 		including that they are inside the rule time window if the objects are timed
 * 		A MatchCount is formed by a counter of matching couples and a link to the MetaOBj
 * 
 * @param left MatchCount by right
 * @param right Object by right
 * @param data Execution data
 * @param result_tag the resulting tag that must be applied
 */
void Node::check_nwand_cond(MatchCount *counter, MetaObj *item, ExecData &data, int *result_tag)
{

	int new_tag, old_tag;
	MetaObj *MainItem;
	int res, res2;
	ULong *begin_and, *end_and;
	int change_to_NEW = FALSE;
	ULong flags;

	if (data.side == LEFT_MEM)
	{
		MainItem = data.left;
		data.right = item;
	}
	else
	{
		data.left = counter->item;
		MainItem = data.right;
	}

	begin_and	= code_p;
	code_p		= begin_and + LEN_WAND_NODE + begin_and[AND_NODE_NKEYS_POS];
	end_and		= code_p + begin_and[AND_NODE_CODELEN_POS];
	flags		= begin_and[AND_NODE_FLAGS_POS];
	old_tag		= data.tag;
	
	res=1;
	while ( res>0 && code_p != end_and )
	{
		res = (*((IntFunction)(*code_p)))(this, data);
	} 

	new_tag = old_tag = data.tag;
	if (new_tag == MODIFY_TAG)
	{
		// Let's try with the old Object
 
		MainItem->set_state(OLD_ST, data.st, data.pos);
 
		code_p = begin_and + LEN_WAND_NODE + begin_and[AND_NODE_NKEYS_POS];
 
		res2=1;
		while ( res2>0 && code_p != end_and )
		{
			res2 = (*((IntFunction)(*code_p)))(this, data);
		}
 
		new_tag = ((res2<<1) | res); 	// 1,1 -> 0x3 -> Modify
										// 1,0 -> 0x2 -> Retract
										// 0,1 -> 0x1 -> Insert
		if (new_tag != RETRACT_TAG)
			MainItem->set_state(NEW_ST, data.st, data.pos);
		else
			change_to_NEW = TRUE;

	} else res2=res;

	if ((new_tag & INSERT_TAG) == INSERT_TAG && res == 1)
		counter->count++;
 
	if ((res>0 || res2>0) && data.side == RIGHT_MEM && counter->count==1)
	{
		//	Initial Tag				current Tag				Tag to apply
		// -------------			-------------------		--------------
		//	INSERT_TAG				INSERT_TAG				RETRACT_TAG
		//	RETRACT_TAG				RETRACT_TAG				INSERT_TAG
		//	MODIFY_TAG				INSERT_TAG				RETRACT_TAG
		//	MODIFY_TAG				RETRACT_TAG				INSERT_TAG
		//	MODIFY_TAG				MODIFY_TAG				0 nothing to do, no propagation
 
		// If the object arrived by the right side, position is -1

		new_tag ^= 0x3;		// RETACT=10b, INSERT=01b, MODIFY=11b
							// The final tag is the XOR (invert) of the last two bits
 
		if (new_tag != 0)
		{
			// In NWAND only must be propagated the left side
			ExecData new_data((*data.st), data.left, NULL, new_tag, LEFT_MEM, -1);

			if (trace >= 2)
			{
				fprintf(trace_file,  "NWAND : PROPAGATION SIDE = %d TAG = %d\nOBJ = ", new_data.side, new_data.tag);
				new_data.left->print(stdout, print_objkey);
				fprintf(trace_file,  "\n");
			}

			propagate(new_data, end_and);
		}

	}

	if ((old_tag & RETRACT_TAG) == RETRACT_TAG && res2 == 1)
		counter->count--;

	// Only is set if a matching was done
	if (result_tag && (res>0 || res2>0))
		*result_tag = new_tag;
	
	code_p = begin_and;

	if (change_to_NEW)
		MainItem->set_state(NEW_ST, data.st, data.pos);
}

/**
 * @brief Execution of code OWAND (Optional, Timed AND).
 * 		OWAND Nodes work very similar to WAND nodes, the unique difference is that on an arrival of an object by left side
 * 		the propagations continues even if no matching are found (with a couple with null value to its right ride)
 * 		This nodes are very useful when the final context of rules are important and you want to get all the related information that
 * 		provide evidence to your inference. In this case some evidence may not be present in all cases but, if they are, you want to
 * 		get it, to consider.
 * 		Imagine you are getting alarms from meny interconnected systems and you want to identify the real cause/fault. You are interested in 
 * 		the alarms you are considering as evidence because you are going to present only the final faults and all the alarms that each fault
 * 		is producing in the different systems, it is very possible that some alarms may be optional, but you want them in case they triggered !! 
 * 
 * @param node Current Node
 * @param data Execution data
 * @return int = 0 execution does not continue any more (the propagation continued internally)
 */
int Node::owand_call(Node *node, ExecData &data)
{

	if (checkingScope)
		return NODE_REACHED;

	if (data.side == LEFT_MEM)
		node->owand_call_by_left(data);
	else
		node->owand_call_by_right(data);

	return(0);
}

/**
 * @brief Execution of code OWAND by left side.
 * 		The object that arrived is firstable stored in the memory of its side, then the rule's
 * 		time window is verified and the conditions are tried between it and the items to the other side.
 * 		The couples that verify the node's code progress toward its children nodes		
 * 		In case no matching is found, a couple with NULL as right side is propagated
 * 
 * @param data Execution data
 */
void Node::owand_call_by_left(ExecData &data)
{
	MatchCount *counter;
	int how_many;
	BTree *tree	= (BTree *)code_p[WAND_NODE_MEM_START_POS + 1];
	long window = (long)code_p[WAND_NODE_WTIME_POS];
	ULong this_flags	= code_p[AND_NODE_FLAGS_POS] >> 16;		// Left_flags!
	ULong other_flags = code_p[AND_NODE_FLAGS_POS] & 0xFFFF;	// Right_flags

	int old_tag = data.tag;

	if (store_in_Wasym_node_by_LEFT(data, counter))
	{
		// In RETRACTION we rely in the counter to propagate the Null Single 
		// by right in case the count is 0
		// Positive tags (INSERT or MODIFY) oblige to calculate this counter
		
		if (data.tag == RETRACT_TAG && counter->count == 0)
		{
			check_owand_cond(counter, Single::null_single(), data, NULL);
		}
		else
		{
			BTState state;
			MetaObj *item;
			how_many=0;

			KeyManager keyman(RIGHT_MEM, LEFT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_WAND_NODE);
			tree->setKeyManager(&keyman);

			// If the timing is active between the objects we find objects with t2< t1 + window
		  	// else we find simply by key
			if ((this_flags & IS_TIMED) && (other_flags & IS_TIMED))
				state = tree->FindBiggerThan(data.left, MetaObj::compare_tw, data.left->t1() + window + 1);
			else
				state = tree->FindByKeys(data.left);

			while (item= (MetaObj *)BTree::Walk(state))
			{
				long t1, t2;
				data.left->comp_window(item, t1, t2, (this_flags & IS_TIMED), (other_flags & IS_TIMED));
				if (t2 - t1 <= window)
					check_owand_cond(counter, item, data, &how_many);
				else if ((other_flags & IS_TEMPORAL) && item->class_type() == SINGLE)
					push_to_remove_old_item(item->single(), RIGHT_MEM);
			}

			if (how_many == 0)
			{
				check_owand_cond(counter, Single::null_single(), data, NULL);
			}
		}

		if (data.tag == RETRACT_TAG || (this_flags & IS_TRIGGER))
			delete counter;
	}

	data.tag=old_tag;
}

/**
 * @brief Execution of code OWAND by right side.
 * 		The object that arrived is firstable stored in the memory of its side, then the rule's time window
 * 		is verified and the conditions are tried between it and the items to the other side.
 * 		The couples that verify the node's code progress toward its children nodes		
 *
 * 		Essentially is quite similar to what is done in a WAND node by right
 * 		There is a counter (MatchCount) for every object in the memory by left. It is updated with the matchings found
 * 
 * @param data Execution data
 */
void Node::owand_call_by_right(ExecData &data)
{
	MatchCount *counter;
	BTree *tree = (BTree *)code_p[WAND_NODE_MEM_START_POS];
	long window = (long)code_p[WAND_NODE_WTIME_POS];
	ULong this_flags	= code_p[AND_NODE_FLAGS_POS] & 0xFFFF;
	ULong other_flags   = code_p[AND_NODE_FLAGS_POS] >> 16;

	int old_tag = data.tag;
	data.right = data.left;

	if (store_in_wand_node_by_RIGHT(data))
	{
		MetaObj *item;
		BTState state;

		KeyManager keyman(LEFT_MEM, RIGHT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_WAND_NODE, true);
		tree->setKeyManager(&keyman);

		// If the timing is active between the objects we find objects with t2< t1 + window
		// else we find simply by key
		if ((this_flags & IS_TIMED) && (other_flags & IS_TIMED))
			state = tree->FindBiggerThan(data.right, Node::cmp_count_with_object_tw, data.right->t1() + window + 1);
		else
			state = tree->FindByKeys(data.right);

		while (counter= (MatchCount *)BTree::Walk(state))
		{
			long t1, t2;
			data.right->comp_window(counter->item, t1, t2, (this_flags & IS_TIMED), (other_flags & IS_TIMED));
			if (t2 - t1 <= window)
				check_owand_cond(counter, data.right, data, NULL);
			else if ((other_flags & IS_TEMPORAL) && counter->item->class_type() == SINGLE)
			push_to_remove_old_item(counter->item->single(), LEFT_MEM);
		}
	}
	data.left = data.right;
	data.tag=old_tag;
}

				
/**
 * @brief Execution of the conditions that every couple made in the OWAND node must satisfy.
 * 		Be aware that in case of modification the conditions are cheched with the OLD state of the object and with NEW state.
 * 		If in both cases the conditions have been satisfied the propagation continues with MODIFY_TAG. If only was in one state
 * 		the propagations will continue as RETRACT_TAG (old), or INSERT_TAG (new) 
 * 
 * @param counter The MatchCount by left
 * @param item The MetaObj by right
 * @param data Execution data
 * @param how_many Number of matching found
 */
void Node::check_owand_cond(MatchCount *counter, MetaObj *item, ExecData &data, int *how_many)
{

	// MEMORIA IZQUIERDA = (void**)(code_p + WAND_NODE_MEM_START_POS) 
	// MEMORIA DERECHA	= (void**)(code_p + WAND_NODE_MEM_START_POS+1)

	int new_tag;
	MetaObj *MainItem;
	int res, res2;
	ULong *begin_and, *end_and;
	int change_to_NEW = FALSE;
	ULong flags;

	if (data.side == LEFT_MEM)
	{
		data.right = item;
		MainItem	= data.left;
	}
	else
	{
		data.left = counter->item;
		MainItem	= data.right;
	}

	begin_and	= code_p;
	code_p		= begin_and + LEN_WAND_NODE + begin_and[AND_NODE_NKEYS_POS];
	end_and		= code_p + begin_and[AND_NODE_CODELEN_POS];
	flags		= begin_and[AND_NODE_FLAGS_POS];
	
	new_tag = data.tag;

	res=1;
	if (data.right != Single::null_single())
	{
		while ( res>0 && code_p != end_and )
		{
			res = (*((IntFunction)(*code_p)))(this, data);
		} 

		if (new_tag == MODIFY_TAG)
		{
			// Let's try with the old object
	
			MainItem->set_state(OLD_ST, data.st, data.pos);
	
			code_p = begin_and + LEN_WAND_NODE + begin_and[AND_NODE_NKEYS_POS];
	
			res2=1;
			while ( res2>0 && code_p != end_and )
			{
				res2 = (*((IntFunction)(*code_p)))(this, data);
			}

			new_tag = ((res2<<1) | res); 	// 1,1 -> 0x3 -> Modify
											// 1,0 -> 0x2 -> Retract
											// 0,1 -> 0x1 -> Insert
 
			if (new_tag != RETRACT_TAG)
				MainItem->set_state(NEW_ST, data.st, data.pos);
			else
				change_to_NEW = TRUE;

		}
		else res2=0;
	}
	else res2=0;

	if (res>0 || res2>0)
	{
		int new_pos = data.pos;
		int n_objs_left =	(int)((begin_and[AND_NODE_N_ITEMS_POS]) >> 16);
		MetaObj * NewLeftItem, *RightItem;
		Status new_st(*(data.st));
		long t1, t2;

		RightItem = data.right;

		if (data.right != Single::null_single())
		{
			int prev_tag = new_tag;
 
			if (prev_tag == INSERT_TAG)
				counter->count ++;

			// Only in the transition from count=0 to 1 the propagation is as modification
			// in the rest of cases the tag is maintained

			if (data.side == RIGHT_MEM && counter->count == 1)
			{

				new_tag = MODIFY_TAG;
 
				if (prev_tag==INSERT_TAG)
				{
					new_st._old_single = Single::null_single();
					new_st._single = data.right->single();
				}

				if (prev_tag == RETRACT_TAG)
				{
					new_st._old_single = data.right->single();
					new_st._single = Single::null_single();
					RightItem = Single::null_single();
				}
			}
 
			if (prev_tag == RETRACT_TAG)
				counter->count --;

		}

		data.left->comp_window(RightItem, t1, t2, (flags & (IS_TIMED<<16)), (flags & IS_TIMED));
		NewLeftItem = new Compound(data.left, RightItem, t1, t2);

		if (NewLeftItem == NULL)
			engine_fatal_err("new Compound: %s\n", strerror(errno));

		ExecData new_data(new_st, NewLeftItem, NULL, new_tag, LEFT_MEM, data.pos);

		if (data.side == RIGHT_MEM)
			new_data.pos += n_objs_left;

		if (trace >= 2)
		{
				fprintf(trace_file,  "OWAND : PROPAGATION SIDE = %d TAG = %d\nOBJ = ", new_data.side, new_data.tag);
				NewLeftItem->print(stdout, print_objkey);
				fprintf(trace_file,  "\n");
		}

		propagate(new_data, end_and);

		new_data.left->unlink();

		if (how_many) (*how_many)++;

	}

	code_p = begin_and;

	if (change_to_NEW)
		MainItem->set_state(NEW_ST, data.st, data.pos);
}

/**
 * @brief Store Control in the memory (BTree) an WAND node by LEFT
 * 		if stored the MetaObj is linked (increments the #links by 1), if removed it is unlinked (decrements #links by 1)
 * 
 * @param data Execution data
 * @return int If the inference must continue
 */
int Node::store_in_wand_node_by_LEFT(ExecData &data)
{
	// LEFT MEMORY = (void**)(code_p + AND_NODE_MEM_START_POS)
	// RIGHT MEMORY	= (void**)(code_p + AND_NODE_MEM_START_POS+1)

	MetaObj * LeftItemInMem;
	int continue_inference;
	ULong flags;
	BTree *tree = (BTree *)code_p[WAND_NODE_MEM_START_POS];
	KeyManager keyman(LEFT_MEM, LEFT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_WAND_NODE);
	tree->setKeyManager(&keyman);

	flags = code_p[AND_NODE_FLAGS_POS]>>16;			//Left side Flags

	switch (data.tag)
	{
		case INSERT_TAG:
			
			// IS_TRIGGER flag imply that no storage is done
			// IS_TEMPORAL flag imply the objects must be retracted when its time os out of window

			if (!(flags & IS_TRIGGER))
			{
				LeftItemInMem = *(MetaObj **)tree->Insert(data.left, MetaObj::compare_t);

				// The object is linked if it was stored (was not found)
				// also the inference continues due it is something new
				if (continue_inference = !BTree::WasFound()){
					data.left->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_wand_node_by_LEFT\n");

				}
			}
			else continue_inference = TRUE;
			break;
		case MODIFY_TAG:
			if (!(flags & IS_TRIGGER))
			{
				bool keys_mod;
				keys_mod = keyman.keysModified(data.pos, data.st->_obj, data.st->_old_obj);

				data.left->set_state(OLD_ST, data.st, data.pos);

				if (keys_mod)
					LeftItemInMem = (MetaObj *)tree->Delete(data.left, MetaObj::compare_t);
				else
					LeftItemInMem = (MetaObj *)tree->Find(data.left, MetaObj::compare_t);

				if (LeftItemInMem)
				{
					if (LeftItemInMem != data.left)
					{
						data.left->unlink();
						data.left = LeftItemInMem;
						data.left->link();			// this way al least the object will have one link more
													// that will decrease at do_loop
						if (trace >= 2) fprintf(trace_file, "LINK+ store_in_wand_node_by_LEFT\n");
					}
				}
				else
				{
					data.tag = INSERT_TAG;
					data.left->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_LEFT\n");
				}

				data.left->set_state(NEW_ST, data.st, data.pos);
				if (!LeftItemInMem || keys_mod)
					tree->Insert(data.left, MetaObj::compare_t);

				continue_inference = TRUE;
			}
			else continue_inference = FALSE;		// The triggers are onlypropagated on INSERT
			break;
		case RETRACT_TAG: 
			if (!(flags & IS_TRIGGER))
			{
				LeftItemInMem = (MetaObj *)tree->Delete(data.left, MetaObj::compare_t);

				continue_inference = (LeftItemInMem != NULL);

				// We delete the arrived compound and replace it by the old copy in the memory
				// We have to work with it from now on
				
				if (continue_inference && LeftItemInMem != data.left)
				{
					data.left->unlink();
					data.left = LeftItemInMem;		// Let's continue with original
					data.left->set_state(OLD_ST, data.st, data.pos);
					data.left->link();				// this way al least the object will have one link more
													// that will decrease at do_loop
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_wand_node_by_LEFT\n");
				}

				if (continue_inference)
					data.left->unlink();

			} else continue_inference = FALSE;		// The triggers are only propagated on INSERT

			break;
	}
	return continue_inference;

}

/**
 * @brief Store Control in the memory (BTree) an WAND node by RIGHT
 * 		if stored the MetaObj is linked (increments the #links by 1), if removed it is unlinked (decrements #links by 1)
 * 
 * @param data Execution data
 * @return int If the inference must continue
 */			
int Node::store_in_wand_node_by_RIGHT(ExecData &data)
{
	// LEFT MEMORY = (void**)(code_p + AND_NODE_MEM_START_POS)
	// RIGHT MEMORY	= (void**)(code_p + AND_NODE_MEM_START_POS+1)

	MetaObj * RightItemInMem;
	int continue_inference;
	ULong flags;
	BTree *tree = (BTree *)code_p[WAND_NODE_MEM_START_POS + 1];
	KeyManager keyman(RIGHT_MEM, RIGHT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_WAND_NODE);
	tree->setKeyManager(&keyman);

	flags = code_p[AND_NODE_FLAGS_POS] & 0xFFFF;	// Right side Flags

	switch (data.tag)
	{
		case INSERT_TAG:
			
			// IS_TRIGGER flag imply that no storage is done
			// IS_TEMPORAL flag imply the objects must be retracted when its time os out of window
			if (!(flags & IS_TRIGGER))
			{
				RightItemInMem = *(MetaObj **)tree->Insert(data.right, MetaObj::compare_t);

				// The object is linked if it was stored (was not found)
				// also the inference continues due it is something new
				if (continue_inference = !BTree::WasFound()){
					data.right->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_wand_node_by_RIGHT\n");

				}
			}
			else continue_inference = TRUE;
			break;
		case MODIFY_TAG:
			if (!(flags & IS_TRIGGER))
			{
				int keys_mod;
				keys_mod = keyman.keysModified(data.pos, data.st->_obj, data.st->_old_obj);
				data.right->set_state(OLD_ST, data.st, data.pos);
				if (keys_mod)
					RightItemInMem = (MetaObj *)tree->Delete(data.right, MetaObj::compare_t);
				else
					RightItemInMem = (MetaObj *)tree->Find(data.right, MetaObj::compare_t);

				if (RightItemInMem)
				{
					if (RightItemInMem != data.right)
					{
						data.right->unlink();
						data.right = RightItemInMem;
						data.right->link();			// this way al least the object will have one link more
													// that will decrease at do_loop
						if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_RIGHT\n");
					}
				}
				else
				{
					data.tag = INSERT_TAG;
					data.right->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_and_node_by_RIGHT\n");
				}
				data.right->set_state(NEW_ST, data.st, data.pos);
				if (!RightItemInMem || keys_mod)
					tree->Insert(data.right, MetaObj::compare_t);

				continue_inference = TRUE;
			}
			else continue_inference = FALSE;		// The triggers are onlypropagated on INSERT
			break;
		case RETRACT_TAG: 
			if (!(flags & IS_TRIGGER))
			{
				RightItemInMem = (MetaObj *)tree->Delete(data.right, MetaObj::compare_t);

				continue_inference = (RightItemInMem != NULL);

				// We delete the arrived compound and replace it by the old copy in the memory
				// We have to work with it from now on

				if (continue_inference && RightItemInMem != data.right)
				{
					data.right->unlink();
					data.right = RightItemInMem;	// Let's continue with original
					data.right->set_state(OLD_ST, data.st, data.pos);
					data.right->link();				// this way al least the object will have one link more
													// that will decrease at do_loop
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_wand_node_by_RIGHT\n");

				}

				if (continue_inference)
					data.right->unlink();

			} else continue_inference = FALSE;		// The triggers are onlypropagated on INSERT
	}
	return continue_inference;

}

/**
 * @brief Store Control in the memory (BTree) an NWAND od OWAND nodes by LEFT (asymmetric nodes)
 * 		In this nodes a MatchCount is maintained with the number of matchings with the right side
 * 		if stored the MetaObj is linked (increments the #links by 1), if removed it is unlinked (decrements #links by 1)
 * 
 * @param data Execution data
 * @param count The counter of matchings
 * @return int If the inference must continue
 */
int Node::store_in_Wasym_node_by_LEFT(ExecData &data, MatchCount *&count)
{
	// LEFT MEMORY = (void**)(code_p + AND_NODE_MEM_START_POS)
	// RIGHT MEMORY	= (void**)(code_p + AND_NODE_MEM_START_POS+1)

	int continue_inference;
	MatchCount * LeftItemInMem;
	MetaObj **LeftItemInMem_p;
	ULong flags;
	BTree *tree = (BTree *)code_p[WAND_NODE_MEM_START_POS];
	KeyManager keyman(LEFT_MEM, LEFT_MEM, code_p[AND_NODE_NKEYS_POS], code_p + LEN_WAND_NODE, true);
	tree->setKeyManager(&keyman);

	flags = code_p[AND_NODE_FLAGS_POS]>>16;			// Right side Flags

	switch (data.tag)
	{
		case INSERT_TAG:
			
			// IS_TRIGGER flag imply that no storage is done
			// IS_TEMPORAL flag imply the objects must be retracted when its time os out of window
			if (!(flags & IS_TRIGGER))
			{
				LeftItemInMem_p = (MetaObj **)tree->Insert(data.left, cmp_count_with_object_t);

				// The object is linked if it was stored (was not found)
				// also the inference continues due it is something new
				if (continue_inference = (LeftItemInMem_p!= NULL && !BTree::WasFound()))
				{
					LeftItemInMem = *((MatchCount **)LeftItemInMem_p) = new MatchCount(data.left);
					data.left->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_Wasym_node_by_LEFT\n");
				}
			}
			else
			{
				continue_inference = TRUE;
				LeftItemInMem = new MatchCount(data.left);
			}
			break;
		case MODIFY_TAG:
			if (!(flags & IS_TRIGGER))
			{
				int keys_mod;
				keys_mod = keyman.keysModified(data.pos, data.st->_obj, data.st->_old_obj);
				data.left->set_state(OLD_ST, data.st, data.pos);
				if (keys_mod)
				{
					LeftItemInMem = (MatchCount *)tree->Delete(data.left, cmp_count_with_object_t);
				}
				else
				{
					LeftItemInMem = (MatchCount *)tree->Find(data.left, cmp_count_with_object_t);
				}

				if (LeftItemInMem)
				{
					if (LeftItemInMem->item != data.left)
					{
						data.left->unlink();
						data.left = LeftItemInMem->item;
						data.left->link();			// this way al least the object will have one link more
													// that will decrease at do_loop
						if (trace >= 2) fprintf(trace_file, "LINK+ store_in_Wasym_node_by_LEFT\n");
					}
				}
				else
				{
					data.tag = INSERT_TAG;
					data.left->link();
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_Wasym_node_by_LEFT\n");
				}
				data.left->set_state(NEW_ST, data.st, data.pos);
				if (!LeftItemInMem || keys_mod)
				{
					LeftItemInMem_p = (MetaObj **)tree->Insert(data.left, cmp_count_with_object_t);
					if (!LeftItemInMem)
						LeftItemInMem = *((MatchCount **)LeftItemInMem_p) = new MatchCount(data.left);
					else 
						*((MatchCount **)LeftItemInMem_p) = LeftItemInMem;
				}

				continue_inference = TRUE;
			}
			else continue_inference = FALSE;	// The triggers are onlypropagated on INSERT
			break;
		case RETRACT_TAG: 
			if (!(flags & IS_TRIGGER))
			{
				LeftItemInMem = (MatchCount *)tree->Delete(data.left, cmp_count_with_object_t);

				continue_inference = (LeftItemInMem != NULL);

				// We delete the arrived compound and replace it by the old copy in the memory
				// We have to work with it from now on
				if (continue_inference && LeftItemInMem->item != data.left)
				{
					data.left->unlink();
					data.left = LeftItemInMem->item;	// Let's continue with original
					data.left->set_state(OLD_ST, data.st, data.pos);
					data.left->link();				// this way al least the object will have one link more
													// that will decrease at do_loop
					if (trace >= 2) fprintf(trace_file, "LINK+ store_in_Wasym_node_by_LEFT\n");
				}

				if (continue_inference)
					data.left->unlink();

			}
			else continue_inference = FALSE;	// The triggers are onlypropagated on INSERT
			break;
	}
	count = LeftItemInMem;
	return continue_inference;

}

/**
 * @brief Push (add to the queue) a new RETRACT action to be propagated
 * 
 * @param item The Single to be pushed
 * @param side The side where propagate
 */
void
Node::push_to_remove_old_item(Single *item, int side)
{
	item->link(); // To be done the inference
	if (trace >= 2) fprintf(trace_file, "LINK+ remove_old_items\n");
	Action *act = new Action(RETRACT_TAG, item, NULL, item->obj(), FALSE, this, NULL, side);
	act->push();
}

/**
 * @brief Execution of MAKESET code
 * 		SET code make a set with all the objects that verify certain conditions
 * 
 * @param node The node where the code is
 * @param data Execution data
 * @return int = 0, stop propagation due it is done internally
 */
int Node::set_call(Node *node, ExecData &data)
{

	if (checkingScope)
		return NODE_REACHED;

	switch (data.tag)
	{
		case INSERT_TAG :
				node->set_call_INSERT(data);
				break;
		case MODIFY_TAG :
				node->set_call_MODIFY(data);
				break;
		case RETRACT_TAG:
				node->set_call_RETRACT(data);
				break;
	}
	return 0;
}

/**
 * @brief Store an item in the tree,. The object always come by left and nothing at right (is an INTRA node)
 * 
 * @param data Execution data
 * @param LeftItemInMem_p = NULL if the insert must be done in the tree of the Set (the object is taken from data.left), 
 * 		  or the address of the item if already in the tree
 */
void Node::set_call_INSERT(ExecData &data, MetaObj **LeftItemInMem_p /*=NULL*/)
{
	MetaObj **PosSet, *LeftItemInMem;
	MetaObj *Item;
	int continue_inference;
	BTree *tree = (BTree *)code_p[SET_NODE_MEM_POS];

	int first_pos = (int)(code_p[SET_NODE_FIRST_ITEM_POS] >> 8);
	int n_objs = (int)(code_p[SET_NODE_N_ITEMS_POS]);
	ULong * begin_code = code_p + LEN_SET_NODE;
	ULong * end_code = begin_code + code_p[SET_NODE_CODELEN_POS];
	ULong * masks = code_p + SET_NODE_PATT_MASK;
	bool  masked = false;

	if (LeftItemInMem_p == NULL)
		LeftItemInMem_p =(MetaObj **)tree->Insert(data.left, MetaObj::metacmp_objs, &masked, masks, begin_code, end_code);

	continue_inference = (LeftItemInMem_p != NULL);

	if (continue_inference)
	{

		MetaObj	*&init_LeftItem = data.left;

		int new_set;

		// The object has been stored if what stored is the object
		new_set = (data.left == (*LeftItemInMem_p));

		Item = *(data.left->meta_dir(&data.left, first_pos, n_objs));

		// In first insertion the MetaObj struct is duplicated and the item is substituted by the set
		if (new_set)
			(*LeftItemInMem_p) = data.left->duplicate_struct(FALSE);

		PosSet = (*LeftItemInMem_p)->meta_dir(LeftItemInMem_p, first_pos, n_objs);

		if (new_set)
		{
			*PosSet = new Set();
		}

		// Due it is posible the co-existence of several MAKESET in the same rule, the masks can produce
		// the insert with a metacmp_ob() return a tuple with this object as a SET but, in other position that
		// is going to be made SET also, a previous object. This is why we duplicate the structure en put the SET there
		else if (masked)
		{
			MetaObj **item_where_set_p;
			LeftItemInMem_p =(MetaObj **)tree->Insert(data.left->duplicate_struct(FALSE), MetaObj::metacmp_objs, NULL, NULL, begin_code, end_code);
			item_where_set_p = (*LeftItemInMem_p)->meta_dir(LeftItemInMem_p, first_pos, n_objs);
			//(*item_where_set_p)->unlink();
			(*item_where_set_p) = *PosSet;
			(*PosSet)->link();
		}


		LeftItemInMem = *LeftItemInMem_p; // We do it now if the set substitutes to LeftItemInMem

		(*PosSet)->set()->setOp(INSERT_TAG, Item);
		(*PosSet)->set()->add_elem(Item);

		if (new_set || masked)
		{

			if (trace >= 2)
			{
				fprintf(trace_file,  "SET INSERT: PROPAGATION TAG = %d\nOBJ = ", INSERT_TAG);
				LeftItemInMem->print(stdout, print_objkey);
				fprintf(trace_file,  "\n");
			}

			ExecData new_data((*data.st), LeftItemInMem, NULL, INSERT_TAG, data.side, data.pos);
			propagate(new_data, end_code);
		}
			
		else
		{
			if (trace >= 2)
			{
				fprintf(trace_file,  "SET INSERT: PROPAGATION TAG = %d\nOBJ = ", MODIFY_TAG);
				LeftItemInMem->print(stdout, print_objkey);
				fprintf(trace_file,  "\n");
			}
	
			ExecData new_data((*data.st), LeftItemInMem, NULL, MODIFY_TAG, data.side, data.pos);
			propagate_modify(new_data, end_code);
		}
 
		data.left = init_LeftItem; 

		(*PosSet)->set()->setOp(0, Item); // To set no more ops in the SET
	}

}

/**
 * @brief Retract an item in the tree. The object always come by left and nothing at right (is an INTRA node)
 * 
 * @param data Execution data
 * @param LeftItemInMem_p = NULL if the retract must be done in the tree of the Set (the object is taken from data.left), 
 * 		  or the address of the item if already in the tree
 */
void Node::set_call_RETRACT(ExecData &data, MetaObj *LeftItemInMem/*=NULL*/)
{
	MetaObj **PosSet;
	MetaObj *Item;
	int continue_inference;
	BTree *tree = (BTree *)code_p[SET_NODE_MEM_POS];
 
	int first_pos = (int)(code_p[SET_NODE_FIRST_ITEM_POS] >> 8);
	int n_objs = (int)(code_p[SET_NODE_N_ITEMS_POS]);
	ULong * begin_code = code_p + LEN_SET_NODE;
	ULong * end_code = begin_code + code_p[SET_NODE_CODELEN_POS];
	ULong * masks = code_p + SET_NODE_PATT_MASK;
 
	if (LeftItemInMem == NULL)
		LeftItemInMem = (MetaObj *)tree->Find(data.left, MetaObj::metacmp_objs, NULL, NULL, begin_code, end_code);
															
	// Continue the inference if it was in the tree
	continue_inference = (LeftItemInMem != NULL);

	if (continue_inference)
	{ 

		MetaObj *Item_in_tree;
		int set_deleted;

		Item = *(data.left->meta_dir(&data.left, first_pos, n_objs));
		PosSet = LeftItemInMem->meta_dir(&LeftItemInMem, first_pos, n_objs);

		Item_in_tree =	(*PosSet)->set()->find_elem(Item);

		continue_inference = (Item_in_tree != NULL);
 
		if (continue_inference)
		{

			set_deleted = (*PosSet)->set()->will_be_null();

			if (set_deleted)
				tree->Delete(data.left, MetaObj::metacmp_objs, NULL, NULL, begin_code, end_code);

			// if was stored the passed item and difers from the one in the tree, make the change	
			// Only the top level MetaObje¡ should be replaced
			if (data.left == Item && Item != Item_in_tree)
			{
				data.left->unlink();
				data.left = Item_in_tree;	// Seguimos con el original
				data.left->set_state(OLD_ST, data.st, data.pos);
				data.left->link();
				if (trace >= 2) fprintf(trace_file, "LINK+ set_call_RETRACT\n");

				
			}
			Item = Item_in_tree;

			(*PosSet)->set()->setOp(RETRACT_TAG, Item);

			if (set_deleted)
			{

				if (trace >= 2)
				{
					fprintf(trace_file,  "SET RETRACT: PROPAGATION TAG = %d\nOBJ = ", RETRACT_TAG);
					LeftItemInMem->print(stdout, print_objkey);
					fprintf(trace_file,  "\n");
				}

				ExecData new_data((*data.st), LeftItemInMem, NULL, RETRACT_TAG, data.side, data.pos);
				propagate(new_data, end_code );
			
			}

			else 
			{
				if (trace >= 2)
				{
					fprintf(trace_file,  "SET RETRACT: PROPAGATION TAG = %d\nOBJ = ", MODIFY_TAG);
					LeftItemInMem->print(stdout, print_objkey);
					fprintf(trace_file,  "\n");
				}

				ExecData new_data((*data.st), LeftItemInMem, NULL, INSERT_TAG, data.side, data.pos);
				propagate_modify(new_data, end_code);
			}
 
		    //(*PosSet)->set()->delete_elem(Item); 
			(*PosSet)->set()->setOp(0, Item); // Nothing else must ve done in the Set

			if (set_deleted)
				LeftItemInMem -> delete_struct(FALSE, FALSE);	// Delete the set

		}
	}
}

/**
 * @brief MODIFY an item in the tree. The object always come by left and nothing at right (is an INTRA node)
 * 
 * @param data Execution data
 */
void Node::set_call_MODIFY(ExecData &data)
{
	MetaObj **PosSet, **LeftItemInMem_old, **LeftItemInMem_new;
	MetaObj *Item;
	int continue_inference;
	Status new_st(*data.st);
	BTree *tree = (BTree *)code_p[SET_NODE_MEM_POS];

	int first_pos = (int)(code_p[SET_NODE_FIRST_ITEM_POS] >> 8);
	int n_objs = (int)(code_p[SET_NODE_N_ITEMS_POS]);
	ULong * begin_code = code_p + LEN_SET_NODE;
	ULong * end_code = begin_code + code_p[SET_NODE_CODELEN_POS];
	ULong * masks = code_p + SET_NODE_PATT_MASK;
 
	Item = *(data.left->meta_dir(&data.left, first_pos, n_objs));

	// Let's locate the Set and find the item in it
	data.left->set_state(OLD_ST, data.st, data.pos);
	LeftItemInMem_old = (MetaObj **)tree->FindDir(data.left, MetaObj::metacmp_objs, NULL, NULL, begin_code, end_code);
	data.left->set_state(NEW_ST, data.st, data.pos);

	// If the item is what have change (now is a set at that position)
	// Let's see it it does not belong now to another set (due to internal private code)

	// If I remove this condition we are allowing different matching of an optional pattern against SET
	// By example for the tuple <(null),{ b1, b2, b3}> if now comes 'a' where null (this is MODIFY_TAG), will come <a , b1> 
	// "a" must do matching with all the items in the set so, aftar this, <a,b2> y <a,b3> will come after,...
	// This must work so ever, always null->a cannot suppose a change in the set. This is done by compilation
	// and we dont need to propagate a RETRACTION and a INSERTION.

	if (first_pos == data.pos && end_code > begin_code)
		LeftItemInMem_new = (MetaObj **)tree->FindDir(data.left, MetaObj::metacmp_objs, NULL, NULL, begin_code, end_code);
	else
		LeftItemInMem_new = LeftItemInMem_old;


	// The inference continue if it was in the tree
	continue_inference = (LeftItemInMem_old != NULL);
 
	if (continue_inference)
	{

		MetaObj	*Item_in_tree;
	
		PosSet = (*LeftItemInMem_old)->meta_dir(LeftItemInMem_old, first_pos, n_objs);
	
		if (LeftItemInMem_old != LeftItemInMem_new)
		{

			ULong *code_p_init;
	
			code_p_init = code_p;
			data.left->set_state(OLD_ST, data.st, data.pos);
			set_call_RETRACT(data, (*LeftItemInMem_old));
	
			code_p = code_p_init;
			data.left->set_state(NEW_ST, data.st, data.pos);
			set_call_INSERT(data, LeftItemInMem_new);
			return;
		}

		Item_in_tree =	(*PosSet)->set()->find_elem(Item);

		// if was stored the passed item and differs from the one in the tree, make the change	
		// Only the top level MetaObje¡ should be replaced
		if (data.left == Item && Item_in_tree && Item != Item_in_tree )
		{
			data.left->unlink();
			data.left = Item_in_tree;	// Let's continue with original
			data.left->link();
			if (trace >= 2) fprintf(trace_file, "LINK+ set_call_MODIFY\n");
			Item = Item_in_tree;
		}

		if (first_pos == data.pos)
		{

			if (Item == Single::null_single())		// Retraction of the null single in SET
			{
				(*PosSet)->set()->setOp(RETRACT_TAG, new_st._old_single);
				new_st._single = (*PosSet)->single();
				new_st._old_single = (*PosSet)->single();

			}
			else if ((*PosSet)->set()->is_null())	// Insertion in an optional set element
			{
				(*PosSet)->set()->setOp(INSERT_TAG, Item);
				(*PosSet)->set()->add_elem(Item);
				new_st._single = (*PosSet)->single();
				new_st._old_single = (*PosSet)->single();
			}
			else 
			{
				// MODIFY of the element in the SET
				(*PosSet)->set()->add_elem(Item);
				(*PosSet)->set()->setOp(BTree::WasFound() ? MODIFY_TAG : INSERT_TAG, Item);
			}
		}
	
		if (trace >= 2)
		{
			fprintf(trace_file,  "SET MODIFY: PROPAGATION TAG = %d\nOBJ = ", MODIFY_TAG);
			(*LeftItemInMem_old)->print(stdout, print_objkey);
			fprintf(trace_file,  "\n");
		}
	
		ExecData new_data(new_st, *LeftItemInMem_old, NULL, MODIFY_TAG, data.side, data.pos);
		propagate_modify(new_data, end_code);
	
		(*PosSet)->set()->setOp(0, Item); // Para que no se haga nada mas en el SET
				
	}
}

/**
 * @brief Execution of TIMER code
 * 		  It stores all the elements that arrive before going down to other elemets with no time control (e.g. a SET),
 * 		  in those timed rules where there are no other mechanism to control the retraction of timed out elements
 * 		  Control the timing of the elements 
 * 
 * @param node Node
 * @param data Execution data
 * @return int = 0 execution must not continue due the propagation is done internally 
 */
int Node::timer_call(Node *node, ExecData &data)
{
	ULong window = code_p[TIMER_NODE_WINDOW_POS];
	BTree *tree = (BTree *)code_p[TIMER_NODE_MEM_POS];
	MetaObj *LeftItemInMem;
	int continue_inference;

	if (checkingScope)
		return NODE_REACHED;

	switch (data.tag)
	{
		int tmr;
		case INSERT_TAG:
			// If, as a consecuence of refresh some actions have been created, we will concatenate the insertion after them
			// Thinking on the possible counts of elements, the old element must to get out the set before enter the new
			
			if ((tmr = node->timer_refresh(code_p, data.left->t1(), false)) > 0)
			{
				data.left->link();
				if (trace >= 2) fprintf(trace_file, "LINK+ timer_call\n");

				Action *act = new Action(INSERT_TAG, data.left->single(), NULL, data.left->single()->obj(), 
																	FALSE, node, code_p);
				act->push();
				continue_inference = 0;
			}
			else if (tmr == 0)
			{
				LeftItemInMem = *(MetaObj **)tree->Insert(data.left, MetaObj::compare_t);

				// The object is stored if what is stored is the object
				if (continue_inference = (LeftItemInMem != NULL && data.left == LeftItemInMem))
					data.left->link();

				if (trace >= 2) fprintf(trace_file, "LINK+ timer_call\n");
			}
			else
			{
				continue_inference = 0;
			}
			break;
		case MODIFY_TAG:
			data.left->set_state(OLD_ST, data.st, data.pos);
	
			LeftItemInMem = (MetaObj *)tree->Find(data.left, MetaObj::compare_t);

			continue_inference = (LeftItemInMem != NULL);


			// The new object is deleted and the function that is managing it
			// now is managing the same old copy found.

			// In MODIFY we have to work with the old ojects that are what is in the BTrees

			if (continue_inference)
			{
				LeftItemInMem->set_state(NEW_ST, data.st, data.pos);

				if (LeftItemInMem != data.left)
				{
					data.left->unlink();
					data.left = LeftItemInMem;
					data.left->link();			// When replacing by the old we have to make a additional link
												// corresponding with the unlink at do_loop
					if (trace >= 2) fprintf(trace_file, "LINK+ timer_call\n");
				} 
			} 
			else data.left->set_state(NEW_ST, data.st, data.pos);

			break;
		case RETRACT_TAG: 
			LeftItemInMem = (MetaObj *)tree->Delete(data.left, MetaObj::compare_t);

			continue_inference = (LeftItemInMem != NULL);

			// The new object is deleted and the function that is managing it
			// now is managing the same old copy found.

			// In RETRACT we have to work with the old ojects that are what is in the BTrees

			if (continue_inference && LeftItemInMem != data.left)
			{
				data.left->unlink();
				data.left = LeftItemInMem;
				data.left->set_state(OLD_ST, data.st, data.pos);
				data.left->link();				// When replacing by the old we have to make a additional link
												// corresponding with the unlink at do_loop
				if (trace >= 2) fprintf(trace_file, "LINK+ timer_call\n");
			}

			if (continue_inference)
				data.left->unlink();

			break;
	}

	code_p += LEN_TIMER_NODE;


	if (continue_inference)
	{
		if (data.tag == MODIFY_TAG)
				node->propagate_modify(data, code_p);
		else
				node->propagate(data, code_p);
	}
	return 0;
}

/**
 * @brief Retract all the elements older than the window in timed nodes (TIMER or ANDW an its variations)
 * 
 * @param codep Where the timed op code starts
 * @param timestamp Real time unix timestamp
 * @param extern_refresh If done externally
 * @return int Number of object flushed
 */
int Node::timer_refresh(ULong *codep, long timestamp, bool extern_refresh)
{
	int number_of_objects_flushed = 0;

	code_p = codep;

	long window = (long) code_p[TIMER_NODE_WINDOW_POS];

	Single *ItemInMem;

	BTree *mem = (BTree *)code_p[TIMER_NODE_MEM_POS];
	BTState state;

	state = mem->FindBiggerThan(NULL, MetaObj::compare_tw, (timestamp - window));

	while (ItemInMem = (Single *)mem->Walk(state))
	{
		ItemInMem->link(); // To execute the inference
		if (trace >= 2) fprintf(trace_file, "LINK+ timer_refresh\n");

		number_of_objects_flushed++;

		Action *act = new Action(RETRACT_TAG, ItemInMem, NULL, ItemInMem->obj(),
														FALSE, this, codep);
		act->push();

		if (extern_refresh)
			do_loop(Action::main_list(), FALSE);

	}
	return number_of_objects_flushed;
}

/**
 * @brief Execution of PROD code. Arriving to a Production node. The rule is inserted (or retracted) from a 
 * 		  conflict set. When all the propagations will we done, the first rule in order at the conflict_set 
 * 		  will be executed
 * 
 * @param node Node where we are
 * @param data Execution data
 * @return int = 0 The propagation cannot go further
 */
int Node::prod_call(Node *node, ExecData &data)
{

	int cat, n_objs_impl, *objs_impl;
	void **mem;
	ULong flags;
	Compound *ProdCompound;

	if (checkingScope)
		return NODE_REACHED;

	char *rulename 	= (char *)code_p[PROD_NODE_RULENAME_POS];
	char *ruleset	= (char *)code_p[PROD_NODE_RULESETNM_POS];
	cat				= (int)code_p[PROD_NODE_RULECAT_POS];
	n_objs_impl		= (int)code_p[PROD_NODE_NOBJIMP_POS];
	flags			= code_p[PROD_NODE_FLAGS_POS];
	objs_impl		= (int*)(code_p + PROD_START_OBJ_POS);
	BTree *tree		= (BTree *)code_p[PROD_NODE_MEM_POS];

	BTree &tree_cat = conflict_set_mem[cat];

	if (trace >= 2)
	{
		fprintf(trace_file,  "PROD : %s/%s CAT = %d TAG = %d\nOBJ = ", rulename, ruleset, cat, data.tag);
		data.left->print(stdout, print_objkey);
		fprintf(trace_file,  "\n");
	}


	switch(data.tag)
	{
		// Insertion and Modification try to insert the rule in the CS and link the data.left
		// If this item (data.left) would be there, it is replaced by thhe old version
		// A previous insertion in the CS has always with higher priority than current
		
		case INSERT_TAG :
		{
			ConflictSet *cs, *cs_inserted;

			ProdCompound = new Compound(data.left);
			if (!(flags & EXEC_TRIGGER))
			{
				Compound *inserted;
				inserted = *(Compound **)tree->Insert(ProdCompound, (BTCompareFunc)Compound::compare_left_with_left);
				if (inserted != ProdCompound)
				{
					// This never should happen
					if (trace >= 2) fprintf(trace_file, "PROD INSERT_TAG: RULE ALREADY IN TRIGGER PROCESS?\n");
					ProdCompound->unlink();			// It was already there, so free the Prod Compound
					return 0;
				}
			}

			cs = new ConflictSet(data.tag, node, ProdCompound);
			cs_inserted = *(ConflictSet **)tree_cat.Insert(cs, (BTCompareFunc)ConflictSet::compare_cset);
			if (cs_inserted !=	cs)		// If it was already there
			{
				delete cs;				// The current cs is freed
				ProdCompound->unlink(); // and the Pro compound too
				return 0;
			}
			else cs->insert(cat);		// Else, it is inserted according to the rule category
			data.left->link();
			if (trace >= 2) fprintf(trace_file, "LINK+ prod_call\n");
		} break;
		case MODIFY_TAG :
		{
			ConflictSet *cs, *cs_inserted;

			data.left->set_state(OLD_ST, data.st, data.pos);

			if (!(flags & EXEC_TRIGGER))
			{
				ProdCompound=(Compound *)tree->Find(data.left, (BTCompareFunc)Compound::compare_with_left);

				if (ProdCompound == NULL)
				{
					// This never should happen
					if (trace >= 2) fprintf(trace_file, "PROD MODIFY_TAG: RULE ALREADY IN TRIGGER PROCESS?\n");
					return 0;
				}
			}
			else ProdCompound = new Compound(data.left);

			ProdCompound->left()->set_state(NEW_ST, data.st, data.pos);

			cs = new ConflictSet(data.tag, node, ProdCompound);
			cs_inserted = *(ConflictSet **)tree_cat.Insert(cs, (BTCompareFunc)ConflictSet::compare_cset);

			if (cs_inserted !=	cs)		// If it was already there
			{
				delete cs;				// The current cs is freed
				if (flags & EXEC_TRIGGER) ProdCompound->unlink(); // and the Pro compound too
				return 0;
			}
			else cs->insert(cat);		// Else, it is inserted according to the rule category
			if (flags & EXEC_TRIGGER) ProdCompound->left()->link();
			if (trace >= 2) fprintf(trace_file, "LINK+ prod_call\n");
		} break;

		// Retraciones borrar la regla del CS y se ejecuta inmediatamente
		case RETRACT_TAG :
		{
			ConflictSet *old;
			ProdCompound = NULL;
			bool exec_FLAG = true;
			Compound prod_com_aux(data.left);

			if (!(flags & EXEC_TRIGGER))
			{
				ProdCompound = (Compound *)tree->Delete(data.left, (BTCompareFunc)Compound::compare_with_left);
				if (ProdCompound == NULL)
				{
					// This never should happen
					if (trace >= 2) fprintf(trace_file, "PROD RETRACT_TAG: RULE ALREADY IN TRIGGER PROCESS?\n");
					return 0;
				}
			}
			
			ConflictSet cs_muestra(data.tag, node, &prod_com_aux);
			old = (ConflictSet *)tree_cat.Delete(&cs_muestra, (BTCompareFunc)ConflictSet::compare_cset);
 
			if (old != NULL)				// The rule was already at the CS (May be in MODIFY or RETRACT)
			{
				if (trace >= 2)
					fprintf(trace_file, "RULE WAS ALREADY AT CS WITH TAG %d\n", old->tag());
 
				ProdCompound = old->prod_compound();
 
				// Remove it
				if (old->tag() == INSERT_TAG){
					exec_FLAG = false;
					ProdCompound->left()->unlink();	// Unlink the link done at INSERT
					ProdCompound->unlink();
				}
				old->remove(cat);
			}

			if (!ProdCompound) exec_FLAG = false;

			if (exec_FLAG)
			{
				ProdCompound->left()->set_state(NEW_ST, data.st, data.pos);
				// we link to unify. This way all the executions have the left linked one more time
				if (flags & EXEC_TRIGGER) ProdCompound->left()->link();
				node->perform_execution(RETRACT_TAG, ProdCompound);
			}
		}
	}
	return(0);	// Do not continue
}

/**
 * @brief Run the Conflict Set. Execute the higher priorized rule
 * 
 * @return int TRUE did execute, FALSE didn't
 */
int Node::run_cs()
{
	int cat;
	ConflictSet *cs;

	cs = ConflictSet::best_cset(&cat);
	BTree &tree_cat = conflict_set_mem[cat];

	if (cs != NULL)
	{
		(void)tree_cat.Delete(cs, (BTCompareFunc)ConflictSet::compare_cset);
		cs->execute(&Node::perform_execution);
		cs->remove(cat);
		return TRUE;

	}
	else
	{
		return FALSE;
	}
}

/**
 * @brief Execute the right part of a Rule
 * 
 * @param tag Execution tag
 * @param ProdCompound Compound created at production node
 */
void Node::perform_execution(int tag, Compound *ProdCompound)
{
	char *nombre;
	char *ruleset;
	int	cat;
	int	n_objs;
	void	**mem;
	ULong flags;
	int exec;

	nombre	= (char*)(_code[PROD_NODE_RULENAME_POS]);
	ruleset = (char*)(_code[PROD_NODE_RULESETNM_POS]);
	cat		= (int)(_code[PROD_NODE_RULECAT_POS]);
	n_objs	= (int)(_code[PROD_NODE_NOBJIMP_POS]);	// # of implied objects
	flags	= _code[PROD_NODE_FLAGS_POS];	

	code_p = _code + LEN_BASIC_PROD_NODE;

	exec = 0;
	switch (tag)
	{
		case INSERT_TAG :
			exec = (flags & EXEC_INSERT);
			break;
		case MODIFY_TAG :
			exec = (flags & EXEC_MODIFY);
			break;
		case RETRACT_TAG :
			exec = (flags & EXEC_RETRACT);
			break;
	}

	if (trace >= 2)
	{
		fprintf(trace_file, "EXEC : %s/%s CAT = %d, TAG = %d\nOBJ = ",
										nombre, ruleset, cat, tag);
		ProdCompound->left()->print(stdout, print_objkey);
		fprintf(trace_file, "\n");
	}

	if (exec)
	{
		if (n_objs != 0)
		{
			mem = (void **)(_code + PROD_NODE_MEM_POS);
			code_p+= n_objs;	// The indexes of the implied objects
		}

		n_inf++;

		if (trace)
		{
			fprintf(trace_file, "----------------------------------------\n");
			fprintf(trace_file, "RULE EXECUTION IN %s\n%s/%s Cat %d\n",
				(tag == INSERT_TAG) ? "INSERTION" : (tag == MODIFY_TAG) ? "MODIFICATION":"RETRACTION",
				nombre,ruleset,cat);
			ProdCompound->left()->print(trace_file, print_obj); fprintf(trace_file, "\n");
			check_trace_file_size();
		}

		Status st_new;
		ExecData exec_data(st_new, ProdCompound->left(), ProdCompound->right(), tag, LEFT_MEM, -1);
		

		int res=1;
		while ( res>0 )
		{
			res = (*((IntFunction)(*code_p)))(this, exec_data);
		}

		ProdCompound->set_right(exec_data.right);

		if ((flags & EXEC_TRIGGER) || tag != MODIFY_TAG)
		{
			if (ProdCompound->right() != NULL){
				ProdCompound->right() = ProdCompound->right()->delete_struct(FALSE, tag == INSERT_TAG);
			}
		}
	}
	else if (trace >= 2) fprintf(trace_file, "EXEC : IGNORED\n");

	if ((flags & EXEC_TRIGGER) || tag == RETRACT_TAG){
		ProdCompound->left()->unlink();			// Was linked in the insertion in the CS
		ProdCompound->unlink();					// It means destructions of the ProdCompound
	}
}
 
/**
 * @brief Execution of the code OBJNEW
 * 		It creates a new object that is appended to the current MetaObj structure by left
 * 
 * @param node MetaObj ProdNode
 * @param data Execution data
 * @return int = 1, execution must continue
 */
int Node::new_call(Node *node, ExecData &data)
{

	ObjectType	*nuevo;
	Single			*nuevo_single;
	Action			*act;
	int exec = 0;

	code_p++;
	int pos			= (int)(((*code_p++) >> 8) & 0x7F);
	int n_attrs	= (int)(*code_p++);
	ULong on_tags = (*code_p++);
	int len_code = (int)(*code_p++);
 
	switch (data.tag)
	{
		case INSERT_TAG :
			exec = (on_tags & EXEC_INSERT);
			break;
		case MODIFY_TAG :
			exec = (on_tags & EXEC_MODIFY);
			break;
		case RETRACT_TAG :
			exec = (on_tags & EXEC_RETRACT);
			break;
	}

	if (!exec)
	{
		code_p += len_code;
		return 1;
	}

	nuevo = new_object(n_attrs, data.left->t2());
	nuevo_single = new Single(nuevo);
	data.right = ((Compound *)data.right) -> join_by_right_untimed(nuevo_single);

	act = new Action(INSERT_TAG, nuevo_single, data.left, nuevo);
	act->push();

	if (trace >= 2)
	{
		fprintf(trace_file,  "CREATE : %s\n", clave(nuevo_single->obj()));
		data.left->print(stdout, print_objkey);
		fprintf(trace_file,  "\n");
	}

	// The object is not linked. An unlink will be done after propagation

	return 1;
}

/**
 * @brief Execution of the code OBJMOD
 * 		It modifies an object positioned at the left side of rule.
 * 
 * @param node MetaObj ProdNode
 * @param data Execution data
 * @return int = 1, execution must continue
 */
int Node::mod_call(Node *node, ExecData &data)
{
	Action *act;
	int exec = 0;

	code_p++;
	int pos			= (int)(((*code_p++) >> 8) & 0x7F);
	int n_attrs	= (int)(*code_p++);
	ULong on_tags = (*code_p++);
	int len_code = (int)(*code_p++);

	ObjectType *obj_cpy;
	Single *act_single;
	
	switch (data.tag)
	{
		case INSERT_TAG :
			exec = (on_tags & EXEC_INSERT);
			break;
		case MODIFY_TAG :
			exec = (on_tags & EXEC_MODIFY);
			break;
		case RETRACT_TAG :
			exec = (on_tags & EXEC_RETRACT);
			break;
	}

	if (!exec)
	{
		code_p += len_code;
		return 1;
	}

	act_single = (*data.left)[pos]->single();

	obj_cpy		= new_object(n_attrs, act_single->obj()->time);
	objcpy(obj_cpy, act_single->obj(), n_attrs);

	act = new Action(MODIFY_TAG, act_single, data.left, obj_cpy);
	act->push();

	if (trace >= 2)
	{
		fprintf(trace_file,  "MODIFY : %s\n", clave(act_single->obj()));
		data.left->print(stdout, print_objkey);
		fprintf(trace_file,  "\n");
	}
 
	// We link it due it has to survide to the modification inference
	act_single->link();
	if (trace >= 2) fprintf(trace_file, "LINK+ mod_call\n");

	return(1);
}

/**
 * @brief Execution of the code OBJCHG
 * 		It modifies an object positioned at the left side of rule WITHOUT propagation.
 * 
 * @param node MetaObj ProdNode
 * @param data Execution data
 * @return int = 1, execution must continue
 */
int Node::mod_wp_call(Node *node, ExecData &data)
{

	Action *act;
	Single *act_single;
	int exec = 0;

	code_p++;
	int pos		= (int)(((*code_p++) >> 8) & 0x7F);
	code_p++;
	ULong on_tags	= (*code_p++);
	int len_code	= (int)(*code_p++);

	switch (data.tag)
	{
		case INSERT_TAG :
			exec = (on_tags & EXEC_INSERT);
			break;
		case MODIFY_TAG :
			exec = (on_tags & EXEC_MODIFY);
			break;
		case RETRACT_TAG :
			exec = (on_tags & EXEC_RETRACT);
			break;
	}

	if (!exec)
	{
		code_p += len_code;
		return 1;
	}

	act_single = (*data.left)[pos]->single();

	act = new Action(CHANGE_TAG, act_single, NULL, NULL);	// Dummy action inserted
	act->push();

	if (trace >= 2)
	{
		fprintf(trace_file,  "CHANGE : %s\n", clave(act_single->obj()));
		data.left->print(stdout, print_objkey);
		fprintf(trace_file,  "\n");
	}

	act_single->link();
	if (trace >= 2) fprintf(trace_file, "LINK+ mod_wp_call\n");
 
	return(1);
}

/**
 * @brief Execution of the code OBJDEL
 * 		It deletes an object positioned at the left side of rule.
 * 
 * @param node MetaObj ProdNode
 * @param data Execution data
 * @return int = 1, execution must continue
 */
int Node::del_call(Node *node, ExecData &data)
{
	Action *act;
	Single *act_single;
	int exec = 0;

	code_p++;
	int pos		= (int)(((*code_p++) >> 8) & 0x7F);
	ULong on_tags	= (*code_p++);
 
	switch (data.tag)
	{
		case INSERT_TAG :
			exec = (on_tags & EXEC_INSERT);
			break;
		case MODIFY_TAG :
			exec = (on_tags & EXEC_MODIFY);
			break;
		case RETRACT_TAG :
			exec = (on_tags & EXEC_RETRACT);
			break;
	}

	if (!exec)
		return 1;

	act_single = (*data.left)[pos]->single();
	act = new Action(RETRACT_TAG, act_single, NULL, act_single->obj());
	act->push();

	if (trace >= 2)
	{
		fprintf(trace_file,  "DELETE : %s\n", clave(act_single->obj()));
		data.left->print(stdout, print_objkey);
		fprintf(trace_file,  "\n");
	}

	// We link it due it has to survide to the modification inference
	(*data.left)[pos]->link();
	if (trace >= 2) fprintf(trace_file, "LINK+ del_call\n");
	
	return 1;
}

/**
 * @brief Execution of the code OBJIMP
 * 		It creates an object that will join the proCompound by right and the execution tag will be maintained over the object
 * 		This object represents an abstraction of the conditions expressed in the LHS of the rule
 * 
 * @param node MetaObj ProdNode
 * @param data Execution data
 * @return int = 1, execution must continue
 */
int Node::objimp_call(Node *node, ExecData &data)
{
	Action *act;

	code_p++;
	int pos		= (int)(((*code_p++) >> 8) & 0x7F);
	int n_attrs = (int)(*code_p++);
	code_p++;									
	int len_code	= (int)(*code_p++);
 
	switch (data.tag)
	{
		case INSERT_TAG: 
		{
			ObjectType	*nuevo = new_object(n_attrs, data.left->t2());
			Single		*nuevo_single = new Single(nuevo);
			data.right = ((Compound *)data.right) -> join_by_right_untimed(nuevo_single);

			act = new Action(INSERT_TAG, nuevo_single, data.left, nuevo);
			act->push();

			// we link it due it must survive until the cease of any of teh conditions that trigger the rule
			// This initial link is valid to the subsequent inference
			
			nuevo_single->link();	// For PROPAGATION + stay alive
			if (trace >= 2) fprintf(trace_file, "LINK+ objimp_call\n");

			if (trace >= 2)
			{
				fprintf(trace_file, "OBJIMPL/CREATE : %s\n", clave(nuevo_single->obj()));
				data.left->print(stdout, print_objkey);
				fprintf(trace_file, "\n");
			}

		} break;
		case MODIFY_TAG: 
		{
			Single *act_single = (*data.right)[pos]->single();
			ObjectType *obj_cpy		= new_object(n_attrs, act_single->obj()->time);

			objcpy(obj_cpy, act_single->obj(), n_attrs);

			act = new Action(MODIFY_TAG, act_single, data.left, obj_cpy);
			act->push();

			// The new single will be replaced by the current
			act_single->link();	// For PROPAGATION + stay alive
			if (trace >= 2) fprintf(trace_file, "LINK+ objimp_call\n");

			if (trace >= 2)
			{
				fprintf(trace_file, "OBJIMPL/MODIFY : %s\n", clave(act_single->obj()));
				data.left->print(stdout, print_objkey);
				fprintf(trace_file, "\n");
			}

		} break;
		case RETRACT_TAG: 
		{
			Single *act_single = (*data.right)[pos]->single();
			act = new Action(RETRACT_TAG, act_single, NULL, act_single->obj());
			act->push();

			// The link is not done due we want to remove the the object
			if (trace >= 2)
			{
				fprintf(trace_file, "OBJIMPL/DELETE : %s\n", clave(act_single->obj()));
				data.left->print(stdout, print_objkey);
				fprintf(trace_file, "\n");
			}
			// Let's ignore the rest of internal code (settings od attributes)
			// only usefull on creation or modification
			code_p += len_code;
		} break;
	}
	return 1;
}

/**
 * @brief Ensd of a rule
 * 
 * @param node -
 * @param data - 
 * @return int = 0, the execution cannot continue beyong this.
 */
int Node::endrule_call(Node *node, ExecData &data)
{
	return 0;
}

/**
 * @brief Execution of code POPSA
 * 		  Store in an object attribute a string
 * 
 * @param node Current Node
 * @param data Execution data
 * @return * int = 1, the execution continues
 */
int Node::popsa_call(Node *node, ExecData &data)
{
	code_p++;

	ULong mem = ((*code_p) >> 15) & 0x1;
	int pos	= (int)(((*code_p) >> 8) & 0x7F);
	int attr = (int)((*code_p++) & 0xFF);
	char *str;
	Value *value;
	Single *sing;

	if (mem == LEFT_MEM)
	{
		sing = (*data.left)[pos]->single();
		value = &(sing->obj()->attr[attr]);
	}
	else
	{
		sing = (*data.right)[pos]->single();
		value = &(sing->obj()->attr[attr]);
	}

	dstack_p--;

	if (dstack_p->str.str_p == NULL || value->str.str_p == NULL ||
			strcmp(dstack_p->str.str_p, value->str.str_p) != 0)
	{
		// if the data at the stack is DINAMIC it is used directly
		// otherwise a copy of it is done (an attribute of an object or something static hast the flag to 0)

		if (dstack_p->str.dynamic_flags == DYNAMIC || dstack_p->str.str_p == NULL)
			str = dstack_p->str.str_p;
		else
			str = strdup(dstack_p->str.str_p);

		// OLD VALUE
		// If the old value is DYNAMIC it is freed (unless in MODIFY that is stored to be able to recreate the old status)
		if (value->str.dynamic_flags == DYNAMIC) 	
		{
			if (Action::last()->_tag == MODIFY_TAG)
				Action::last()->store_mod_attr(attr);
			else
				free(value->str.str_p);
		}

		// SET of the new value
		value->str.str_p = str;

		if (str != NULL)
			value->str.dynamic_flags = DYNAMIC;
	}
	else if (dstack_p->str.str_p != NULL && dstack_p->str.dynamic_flags == DYNAMIC)
	{
		free(dstack_p->str.str_p);
	}

	return(1);
}

/**
 * @brief Execution of the codes POPSN or POPSF
 * 		  Almacena un numero entero o en punto flotante en un atributo de un objeto
 * 
 * @param node 
 * @param data 
 * @return int 
 */
int Node::pops_call(Node *node, ExecData &data)
{
	code_p++;

	ULong mem = ((*code_p) >> 15) & 0x1;
	int pos	= (int)(((*code_p) >> 8) & 0x7F);
	int attr = (int)((*code_p++) & 0xFF);

	if (Action::last()->_tag == MODIFY_TAG)
		Action::last()->store_mod_attr(attr);

	dstack_p--;

	if (mem == LEFT_MEM)
		(*data.left)[pos]->single()->obj()->attr[attr] = (*dstack_p);
	else
		(*data.right)[pos]->single()->obj()->attr[attr] = (*dstack_p);
	return(1);


}

/**
 * @brief Order function based on code execution
 * 
 * @param code Start of code
 * @param end_of_code End of code
 * @param left MetaObj at the left side
 * @param right MetaObje at the right side
 * @return int <0, 0 > 1
 */
int Node::execute_cmp(ULong *code, ULong *end_of_code, MetaObj *left, MetaObj *right)
{
	ULong *code_p_old;
	int res;
 
	code_p_old = code_p;
	code_p = code;
 
	Status st_new;
	ExecData new_data(st_new, left, right, INSERT_TAG, LEFT_MEM, -1);

	res = 1;
	while (res>0 && code_p!=end_of_code)
	{
		res = (*((IntFunction)(*code_p)))(NULL, new_data);
	}
	code_p = code_p_old;
	return cmp_result;
}
 
/**
 * @brief Compare function to compare a MatchCount with a MetaObj 
 * 
 * @param c_obj1 MetaObj
 * @param c_obj2 MatchCounter
 * @param list List of variable arguments taht will be passe dto the compare function
 * @return int <1, 0 , >1
 */
int Node::cmp_count_with_object(const void *c_obj1, const void *c_obj2, va_list list)
{
	return ((MatchCount *)c_obj2)->item->compare((MetaObj *)c_obj1, list);
}

/**
 * @brief Compare the time of a MatchCount and a MetaObj
 * 
 * @param c_obj1 MetaObj
 * @param c_obj2 MatchCount
 * @param list List of variable arguments taht will be passe dto the compare_t function
 * @return int int <1, 0 , >1
 */
int Node::cmp_count_with_object_t(const void *c_obj1, const void *c_obj2, va_list list)
{
	return MetaObj::compare_t(c_obj1, ((MatchCount *)c_obj2)->item, list);
}

/**
 * @brief Compare the time window of a MatchCount and a MetaObj
 * 
 * @param c_obj1 MetaObj
 * @param c_obj2 MatchCount
 * @param list List of variable arguments taht will be passe dto the compare_t function
 * @return int int <1, 0 , >1
 */
int Node::cmp_count_with_object_tw(const void *c_obj1, const void *c_obj2, va_list list)
{
	return MetaObj::compare_tw(c_obj1, ((MatchCount *)c_obj2)->item, list);
}


