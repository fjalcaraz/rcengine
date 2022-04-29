/**
 * @file classes.cpp
 * @author Francisco Alcaraz
 * @brief Object Class management. It maintais a list of all defined classes their hierarchy and their inner attributes
 *    It manages classes that are extensions of others (superclasses, ussing the IS A syntax)
 *    It also manages classes that are a restriction of other. These are those hat have some attributes with special, fixed, values
 * 
 *    This file also contains the functions called during compilation time related to classes that will add the classname/restricted attributes check nodes
 * 
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "codes.h"
#include "patterns.hpp"
#include "vars.hpp"
#include "lex.hpp"
#include "strlow.hpp"
#include "classes.hpp"

PRIVATE ObjClass *first_class = NULL;
PRIVATE ObjClass *curr_class = NULL;
PRIVATE int curr_attr;

PRIVATE Node root(INTRA, 0, 0); // Root of the final, generated, tree

/**
 * @brief Construct a new Obj Class:: Obj Class object
 *
 * @param name Name of the class
 * @param abstract If it is abstract (does not allow instances)
 * @param temp_flags Temporal Flags
 */
ObjClass::ObjClass(char *name, int abstract, ULong temp_flags)
{
  _attr[0].type = TYPE_STR;
  strcpy(_attr[0].name, "classname");
  _num_of_attrs = 1;
  _num_of_attrs_inherited = 0;
  strncpy(_name, name, MAXNAME);
  _next_class = NULL;
  _subclass = NULL;
  _superclass = NULL;
  _next_subcl = NULL;
  _is_abstract = abstract;
  _temp_flags = temp_flags;
  _last_node_of_class_def = NULL;
  _is_a_restriction = FALSE;
  // class_apttern if filled when is known it is a superclass or a restriction
  _class_pattern = NULL;
}

/**
 * @brief Destroy the Obj Class:: Obj Class object
 *
 */
ObjClass::~ObjClass()
{
  ObjClass **class_p, *subclass, *next_sub;

  class_p = ObjClass::get_class(_name);
  (*class_p) = (*class_p)->_next_class;

  if (_class_pattern != NULL)
    delete _class_pattern;

  subclass = _subclass;

  while (subclass != NULL)
  {
    next_sub = subclass->_next_subcl;
    delete subclass;
    subclass = next_sub;
  }

  free_nodes_of_class();
}

/**
 * @brief grep an ObjectClass by its name
 *
 * @param classname Name of the class
 * @return ObjClass**
 */
ObjClass **
ObjClass::get_class(char *classname)
{
  ObjClass **class_p;

  for (class_p = &first_class;
       (*class_p) != NULL &&
       strncmp((*class_p)->_name, classname, MAXNAME) != 0;
       class_p = &((*class_p)->_next_class))
    ;

  return class_p;
}

/**
 * @brief set the last node in the nodes tree that checks the bellonging of an object to this class
 *
 */
void ObjClass::set_last_node_from_pattern()
{
  if (_class_pattern != NULL)
    _last_node_of_class_def = _class_pattern->last_intra();
  _class_pattern = NULL;
}

/**
 * @brief set up that a this class inherit from other superclass
 *
 * @param supercl_p the superclass of this
 */
void ObjClass::inherit_class(ObjClass *supercl_p)
{
  int i;

  for (i = 0; i < supercl_p->_num_of_attrs; i++)
  {
    _attr[i].type = supercl_p->_attr[i].type;
    strncpy(_attr[i].name, supercl_p->_attr[i].name, MAXNAME);
  }
  _num_of_attrs = supercl_p->_num_of_attrs;
  _num_of_attrs_inherited = supercl_p->_num_of_attrs;

  // Super -subclass-> B -next_subcl-> C
  // Super -subclass-> A(this) -next_subcl-> B -next_subcl-> C
  _next_subcl = supercl_p->_subclass;
  supercl_p->_subclass = this;

  // Also each class is pointing to its superclass
  _superclass = supercl_p;
}

/**
 * @brief Definition of a new attribute for this class
 *
 * @param name Name of the Attribute
 * @param type Type of the Attribute
 */
void ObjClass::new_attr(char *name, int type)
{
  if ((curr_attr = attr_index(name)) == -1)
  {
    if (type == ANY_TYPE)
      comp_err("Must be defined the type for new attributes\n");

    if (_num_of_attrs == MAXATTRS)
      comp_err("Too many attributes for class %s (MAX=%d)\n", name, MAXATTRS);

    if (_is_a_restriction)
      comp_err("Cannot be defined new attributes for restricted classes\n");

    _attr[_num_of_attrs].type = type;
    strncpy(_attr[_num_of_attrs].name, name, MAXNAME);
    curr_attr = _num_of_attrs;
    _num_of_attrs++;
  }
  else
  {
    if (!_is_a_restriction)
    {
      comp_err("Attribute \"%s\" duplicated\n", name);
    }
    else if (type != ANY_TYPE && _attr[curr_attr].type != type)
    {
      comp_err("Restricted classes cannot redefine attribute types\n");
    }
  }
}

//
// FUNCTIONS RELATED TO CLASSES COMPILING
//


/**
 * @brief Try to define a new class. 
 *    Set curr_class that will be used in the following functions
 * 
 * @param name Name of the class
 * @param abstract If abstract
 * @param event Temporal Flags
 */
PUBLIC void
def_class(char *name, int abstract, int event)
{
  ObjClass **class_p;

  class_p = ObjClass::get_class(name);

  if ((*class_p) == NULL)
  {
    curr_class = (*class_p) = new ObjClass(name, abstract, event);
  }
  else
  {
    curr_class = (*class_p);
    comp_err("Class %s already defined\n", name);
  }
  free(name);
}

/**
 * @brief Try to set curr_class as a extension of a superclass
 * 
 * @param name Name of the superclass
 */
PUBLIC void
join_class(char *name)
{
  ObjClass **supercl_p;

  supercl_p = ObjClass::get_class(name);

  if ((*supercl_p) == NULL)
    comp_err("Class %s unknown in superclass specification\n", name);

  else
  {
    if ((*supercl_p) == curr_class)
      comp_err("The class %s cannot be superclass of itself\n", name);

    if ((*supercl_p)->is_a_restriction())
      comp_err("The class %s is restricted so cannot act as a superclass\n",
               name);

    if (curr_class->is_a_restriction() && (*supercl_p)->is_abstract())
      comp_err("Restricted classes can only be defined from non-abstract classes\n");

    curr_class->inherit_class(*supercl_p);

    curr_class->add_class_check();
  }
  free(name);
}

/**
 * @brief Try to set curr_class as a restriction of another 
 * 
 * @param name Name of the superclass
 */
PUBLIC void
restrict_class(char *name)
{
  if (curr_class->is_abstract())
    comp_err("Restricted classes cannot be abstract\n");
  curr_class->set_restricted();
  join_class(name);
}

/**
 * @brief Base classes should have its class check nodes
 * 
 */
PUBLIC void
is_a_base_class()
{
  curr_class->add_class_check();
}

/**
 * @brief Declare an Attribute
 * 
 * @param name Name of the attribute
 * @param type type of it
 */
PUBLIC void
def_attr(char *name, int type)
{
  curr_class->new_attr(name, type);
  free(name);
}

/**
 * @brief Class compilations has ended, Last node is stored due rule code related to this class will hang from there
 * 
 */
PUBLIC void
end_def_class()
{
  curr_class->set_last_node_from_pattern();
  Pattern::delete_patterns();
  Var::delete_vars();
}

//
// ACCESS TO CLASSES INFORMATION WHILE COMPILING RULES
//

/**
 * @brief Get the index of a class attribute. An attribute is managed as an index inside the array of values of an object
 * 
 * @param name Name of the attribute
 * @return int index of it or -1 in case of not found
 */
int ObjClass::attr_index(char *name)
{
  int ind;

  for (ind = 0; ind < _num_of_attrs &&
        strncmp(_attr[ind].name, name, MAXNAME) != 0;
       ind++);

  if (ind == _num_of_attrs)
    return -1;
  else
    return ind;
}

/**
 * @brief Get the root of the nodes net
 * 
 * @return Node* the root
 */
// static
Node *ObjClass::get_real_root()
{
  return &root;
}

/**
 * @brief Set the class of the clause at the left hand of the rule that is being compiled
 * 
 * @param name Name of the class
 * @param status The status of the matching (NORMAL, NEGATED or OPTIONAL)
 */
// static
void ObjClass::set_curr_class_LHR(char *name, StValues status)
{

  curr_class = *(get_class(name));

  if (curr_class == NULL)
  {
    comp_err("Class %s not defined\n", name);
  }

  if (curr_class->is_an_abstract_hierarchy())
  {
    comp_err("Class %s has only abstract subclasses\n", name);
  }

  (void)new Pattern(curr_class,
                    status,
                    curr_class->_last_node_of_class_def, TRUE);
}

/**
 * @brief Set the class that is compiled at the right hand of the rule 
 * 
 * @param name Name of tha class
 */
// static
void ObjClass::set_curr_class_RHR(char *name)
{
  curr_class = *(get_class(name));

  if (curr_class == NULL)
    comp_err("Class %s not defined\n", name);

  if (!curr_class->is_a_normal_class())
    comp_err("Class only allowed in pattern matching\n");

  (void)new Pattern(curr_class);
}

/**
 * @brief Set curr_class with this
 * 
 */
void ObjClass::set_curr_class()
{
  curr_class = this;
}

/**
 * @brief check if this class is normal (is not abstract or a restriction)
 * 
 * @return int 
 */
int ObjClass::is_a_normal_class()
{
  return (!_is_a_restriction && !_is_abstract);
}

/**
 * @brief Set curr_attr with the index ot the attribute of curr_class and identified by name
 * 
 * @param name Name of the attribute
 */
// static
void ObjClass::set_curr_attr(char *name)
{
  curr_attr = curr_class->attr_index(name);

  if (curr_attr == -1)
  {
    comp_err("Attribute %s not defined for class %s\n",
             name, curr_class->_name);
  }
}

/**
 * @brief Return the effective classname. 
 *    In case of classes that are restrictions of another, the classname of the objects will be one of the superclass. 
 *    By example "person" -> male(person with sex='M') and female(person with sex='F')
 *      All the objects must come with "person" as classname but male and female may be used in the rules
 * 
 * @return char* classname effective
 */
char *
ObjClass::effective_classname()
{

  if (curr_class->_is_a_restriction)
    return curr_class->_superclass->_name;
  else
    return curr_class->_name;
}

/**
 * @brief Return the type of the curr_attr
 * 
 * @param attr Where to write the curr_attr (index)
 * @param type Where to write the curr_attr type
 */
// static
void ObjClass::curr_attr_type(int *attr, int *type)
{
  if (attr != (int *)NULL)
    (*attr) = curr_attr;
  if (type != (int *)NULL)
    (*type) = curr_class->_attr[curr_attr].type;
}

/**
 * @brief Return the number of attributes that curr_class has
 * 
 * @return int 
 */
// static
int ObjClass::n_attrs_of_curr()
{
  return curr_class->_num_of_attrs;
}

/**
 * @brief Add the nodes, hunging from root, that will filter the object od this class (objects that have as classname = attr[0] the value of _name)
 * 
 */
void ObjClass::add_class_check()
{

  if (!_is_abstract)
  {
    if (!_is_a_restriction)
    {
      // In normal classes is inserted a check of its own classname from root
      // and also after its superclass (if any)

      _class_pattern = new Pattern(this, ST_NORMAL, &root, TRUE);
      _class_pattern->add_class_check(_name);
      for (ObjClass *super = _superclass; super != NULL; super = super->_superclass)
        root.insert_class_check(&(super->_last_node_of_class_def),
                                _name, FALSE);
    }
    else
    {
      // if case of restrictions the pattern is started from the classname check node of superclass that restricts
      _class_pattern = new Pattern(this, ST_NORMAL,
                                   _superclass->_last_node_of_class_def->parent_node(LEFT_MEM), TRUE);
    }
  }
  // else
  // Abstract class do not have pattern to check the classname. No objects will come with this classname
  
}

/**
 * @brief check that it and all the subclasses are abstract. False means that some "real" object of this class may exist
 * 
 * @return int 
 */
int ObjClass::is_an_abstract_hierarchy()
{
  ObjClass *subclass_p;

  if (!_is_abstract)
  {
    return (FALSE);
  }
  else
  {
    for (subclass_p = _subclass;
         subclass_p != NULL;
         subclass_p = subclass_p->_next_subcl)
    {
      if (!subclass_p->is_an_abstract_hierarchy())
        return (FALSE);
    }
    return (TRUE);
  }
}

/**
 * @brief check if two different classes may share the shame objects
 * 
 * @param cl1 Class 1
 * @param cl2 Class 2
 * @return int 
 */
int ObjClass::could_be_equal_obj(ObjClass *cl1, ObjClass *cl2)
{

  if (cl1 == cl2)
  {
    return TRUE;
  }
  else
  {
    return (cl1->is_subclass(cl2) || cl2->is_subclass(cl1) ||
            (cl1->_is_a_restriction && cl2->_is_a_restriction && cl1->_superclass == cl2->_superclass));
  }
}

/**
 * @brief Check if this class is a subclass of another Superclass 
 * 
 * @param cl2 SuperClass
 * @return int 
 */
int ObjClass::is_subclass(ObjClass *cl2)
{

  ObjClass *clx;

  if (this == cl2)
    return (TRUE);

  for (clx = _superclass;
       clx != NULL;
       clx = clx->_superclass)
  {
    if (clx == cl2)
      return (TRUE);
  }
  return (FALSE);
}

//
// CLASS RELATED NODES DELETION
//

/**
 * @brief Free the nodes that filter the objects of an specific class
 * 
 */
void ObjClass::free_nodes_of_class()
{
  NodeIter iter;
  int side;
  Node *node, *next;

  if (_last_node_of_class_def == NULL)
    return;

  if (_last_node_of_class_def->first_child(side, iter) != NULL)
    comp_err("_last_node_of_class_def not NULL\n");

  if (!_is_abstract)
  {
    // If it is not an Abstract class, it has own nodes that have to be deleted
    // The abstract classes only have links to nodes that belongs to the subclasses

    // We have to delete until a node with code length 0. Usually we will find only one
    // node with the TCLASS, unless in the restricted class
  
    node = _last_node_of_class_def;

    do
    {

      next = node->parent_node(LEFT_MEM);
      delete node;
      node = next;

    } while (node != &root && (!_is_a_restriction || node->first_child(side, iter) == NULL));
  }
  else
  {
    delete _last_node_of_class_def;
  }
  _last_node_of_class_def = NULL;
}

void ObjClass::delete_all()
{
  while (first_class != NULL)
  {
    delete first_class;
  }
}

//
// PUBLIC FUNCTIONS TO ACCESS TO CLASSES AND THE NET
//

/**
 * @brief Check if a class of name "name1" is a subclass of a superclass of name "name2"
 * 
 * @param name1 Name of subclass
 * @param name2 Name of superclass
 * @return int 
 */
int class_is_subclass_of(char *name1, char *name2)
{

  ObjClass **class1;
  ObjClass **class2;

  class1 = ObjClass::get_class(name1);
  class2 = ObjClass::get_class(name2);

  if (*class1 == NULL || *class2 == NULL)
    return (FALSE);

  return ((*class1)->is_subclass(*class2));
}

/**
 * @brief Get the class object identified by a name and get its number of attributes
 * 
 * @param name Name of the class
 * @param n_attrs where to write the number of attributes
 * @return void* Returns the ObjClass object as (void *)
 */
void *
get_class(char *name, int *n_attrs)
{
  ObjClass **the_class;
  char namelower[MAXNAME];

  strlowerncpy(namelower, name, MAXNAME);
  the_class = ObjClass::get_class(namelower);

  if (*the_class != NULL)
    *n_attrs = (*the_class)->n_attrs();

  return (void *)(*the_class);
}

/**
 * @brief Gets the attribute name by its number
 * 
 * @param the_class ObjClass object
 * @param n_attr Number of the attribute searched
 * @return char* The name of the attribute
 */
char *
attr_name(void *the_class, int n_attr)
{
  return ((ObjClass *)the_class)->attr_name(n_attr);
}

/**
 * @brief Gets the attribute type by its number
 * 
 * @param the_class ObjClass object
 * @param n_attr Number of the attribute searched
 * @return char* The type of the attribute
 */
int attr_type(void *the_class, int n_attr)
{
  return ((ObjClass *)the_class)->attr_type(n_attr);
}

/**
 * @brief Gets the attribute index by its name
 * 
 * @param the_class ObjClass object
 * @param name Name of the attribute searched
 * @return char* The index of the attribute
 */
int attr_index(void *the_class, char *name)
{
  char namelower[MAXNAME];

  strlowerncpy(namelower, name, MAXNAME);

  return ((ObjClass *)the_class)->attr_index(namelower);
}

/**
 * @brief print the net
 * 
 */
PUBLIC
void print_net()
{
  root.print("");
}

/**
 * @brief Print all the classes
 * 
 */
// static
void ObjClass::print_all()
{
  ObjClass *class_p;

  for (class_p = first_class; class_p != NULL; class_p = class_p->_next_class)
    class_p->print();
}

/**
 * @brief Print this ObjClass information
 * 
 */
void ObjClass::print()
{
  int n;

  fprintf(trace_file, "CLASS %s (", _name);
  if (!_is_a_restriction && _superclass != NULL)
    fprintf(trace_file, "IS A %s ", _superclass->_name);
  if (_is_abstract)
    fprintf(trace_file, "ABSTRACT ");
  if (_is_a_restriction)
    fprintf(trace_file, "RESTRICTION OF %s ", _superclass->_name);
  fprintf(trace_file, ")\n");

  for (n = 0; n < _num_of_attrs; n++)
    fprintf(trace_file, "\t%s\t(%s)\n", _attr[n].name,
           _attr[n].type == TYPE_CHAR ? "CHAR" : _attr[n].type == TYPE_NUM ? "INT"
                                             : _attr[n].type == TYPE_STR   ? "STR"
                                             : _attr[n].type == TYPE_FLO   ? "FLO"
                                             : _attr[n].type == TYPE_BOOL  ? "BOOL"
                                                                           : "???");
}
