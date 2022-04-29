/**
 * @file classes.hpp
 * @author Francisco Alcaraz
 * @brief Functions, constants and related structs exported by the classes module. Definition of ObjClass class
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "config.hpp"
#include "nodes.hpp"
#include "patterns.hpp"
#include "expr.hpp"

#ifndef CLASSES_HH_INCLUDED
#define CLASSES_HH_INCLUDED

#define NORMAL_CLASS 0
#define SUBCLASS 1
#define RESTRICTION 2

/*
#define IS_NORMAL    0
#define IS_TRIGGER   1
#define IS_TEMPORAL  2
#define IS_UNTIMED   3
*/

#define IS_NORMAL 0
#define IS_TIMED 1
#define IS_UNTIMED 2
#define IS_TRIGGER 4
#define IS_TEMPORAL 8
#define IS_PERMANENT 16

#define TIMED_MASK 3
#define STORE_MASK 28

struct Attr
{
  UChar type;
  char name[MAXNAME];
};

class ObjClass
{
private:
  char _name[MAXNAME];
  int _num_of_attrs;
  int _num_of_attrs_inherited;
  Attr _attr[MAXATTRS];
  ObjClass *_next_class;
  ObjClass *_superclass;
  ObjClass *_subclass;
  ObjClass *_next_subcl;
  // Node *_tclass_node;
  Node *_last_node_of_class_def;
  Pattern *_class_pattern;
  int _is_abstract;
  ULong _temp_flags;
  int _is_a_restriction;

public:
  ObjClass(char *name, int abstract, ULong temp_flags);
  ~ObjClass();
  void set_last_node_from_pattern();
  static ObjClass **get_class(char *classname);
  void inherit_class(ObjClass *supercl_p);
  void new_attr(char *name, int type);
  void set_restricted() { _is_a_restriction = TRUE; };

  static Node *get_real_root();

  int is_a_restriction() { return _is_a_restriction; };
  int is_abstract() { return _is_abstract; };
  int is_a_normal_class();
  int is_subclass(ObjClass *cl2);
  ULong temp_flags() { return _temp_flags; };
  int attr_index(char *name);
  int attr_type(int index) { return _attr[index].type; };
  char *attr_name(int index) { return _attr[index].name; };
  int n_attrs() { return _num_of_attrs; };

  void set_curr_class();
  static void set_curr_class_LHR(char *name, StValues status);
  static void set_curr_class_RHR(char *name);
  static void set_curr_attr(char *name);

  static char *effective_classname();
  static void curr_attr_type(int *attr, int *type);
  static int n_attrs_of_curr();
  int is_an_abstract_hierarchy();
  void add_class_check();
  static int could_be_equal_obj(ObjClass *cl1, ObjClass *cl2);

  void free_nodes_of_class();
  static void delete_all();

  static void print_all();
  void print();

  char *getname() { return _name; }
};

PUBLIC void def_class(char *name, int abstract, int event);
PUBLIC void join_class(char *name);
PUBLIC void restrict_class(char *name);
PUBLIC void is_a_base_class();
PUBLIC void def_attr(char *name, int type);
PUBLIC void end_def_class(void);
PUBLIC int num_of_attrs(void);
PUBLIC void print_net();

#endif
