/**
 * @file eng.cpp
 * @author Francisco Alcaraz
 * @brief This module is in charge of the execution of a rule package against an object
 *        that has been created, modified or deleted.
 *        The rules has been compiled previously following the Folgy's RETE algorithm,
 *        generating a net of nodes with some code inside that act as object filters, that is
 *        the objects must satisfy the node's inner code to be allowed to pass through, bound to the children nodes
 *        The op codes has been replaced by the equivalent function pointers to achieve high performance
 *        Each function is responsible to advance de program counter to the next operation
 *
 * @version 1.0
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "engine.h"
#include "nodes.hpp"
#include "error.hpp"
#include "eng_p.hpp"

int n_inf = 0;                 /* number of inferences made */
int trace = 0;                 /* tracing level             */
FILE *trace_file = stdout;     /* traces file               */

/**
 * @brief reset the inferences counter
 *
 */
PUBLIC
void reset_inf_cnt()
{
  n_inf = 0;
}

/**
 * @brief Get the value of the inference counter
 *
 * @return int
 */
PUBLIC
int get_inf_cnt()
{
  return n_inf;
}

/**
 * @brief Set the trace level
 *
 * @param level: 1 basic, 2 advanced
 */
PUBLIC
void set_trace_level(int level)
{
  trace = level;
}

/**
 * @brief Set the trace file where all the traces are ouput
 *    trace_file_size_control is set to false (no control over file size) due with this method the filename is not known
 *    Use set_trace_file_name in case a 
 *
 * @param file FILE structure where to write the traces
 */
PUBLIC
void set_trace_file(FILE *file)
{
  // if trace_file_size_control is set, it is because the previous trace file is an regular file
  if (trace_file_size_control && file != trace_file)
    fclose(trace_file);

  trace_file = file;
  trace_file_size_control = false;
}

/**
 * @brief Set the trace file name
 *    trace_file_size_control is set to TRUE
 * 
 * @param name 
 */
PUBLIC
void set_trace_file_name(const char *name)
{
  FILE *sz_file;
  char dat_file[256];
  char dat_file_old[256];

  // if trace_file_size_control is set, it is because the previous trace file is an regular file
  if (trace_file_size_control)
    fclose(trace_file);

  strcpy(trace_file_name, name);

  if (!strcmp(name, "stdout"))
  {
    trace_file_size_control = FALSE;
    trace_file = stdout;
  }
  else
  {
    trace_file_size_control = TRUE;
    sprintf(dat_file, "%s.dat", name);
    sprintf(dat_file_old, "%s.old", name);
    unlink(dat_file_old);
    (void) !link(dat_file, dat_file_old);
    if ((trace_file = fopen(dat_file, "a")) == NULL)
      engine_fatal_err("Can't open trace file: %s\n", dat_file);
  }
}

/**
 * @brief Set the trace file size 
 * 
 * @param size 
 */
PUBLIC
void set_trace_file_size(unsigned long size)
{
  trace_file_size = size;
}

/**
 * @brief Check the size of the trace file
 * 
 * @return PUBLIC 
 */
PUBLIC void check_trace_file_size()
{
  int fd_of_trace_file;
  struct stat trace_file_stat;
  char dat_name[260];
  char dat_name_old[260];

  if (!trace_file_size_control)
    return;

  fd_of_trace_file = fileno(trace_file);
  if (fstat(fd_of_trace_file, &trace_file_stat) >= 0)
  {
    if (trace_file_stat.st_size >= trace_file_size)
    {
      fclose(trace_file);
      sprintf(dat_name, "%s.dat", trace_file_name);
      sprintf(dat_name_old, "%s.old", trace_file_name);
      unlink(dat_name_old);
      (void) !link(dat_name, dat_name_old);
      unlink(dat_name);
      if ((trace_file = fopen(dat_name, "a")) == NULL)
        engine_fatal_err("Can't open trace file: %s\n", dat_name);
    }
    else
      fflush(trace_file);
  }
}

//
// Object events propagation
//

/**
 * @brief This function comunicate the engine that an objects is going to be externally modified
 *      The object passed should be in its previous state, before being modified
 *      After the modification the object must be propagated calling engine_loop with the tag MODIFY_TAG
 * 
 * @param obj The old version of the object, before being modified
 */
PUBLIC
void engine_modify(ObjectType *obj)
{
  ObjClass *the_class = *ObjClass::get_class(obj->attr[0].str.str_p);

  if (the_class == NULL)
    engine_fatal_err("Unknown class %s in external object modification",
                     obj->attr[0].str.str_p);
  old_obj_modify = new_object(the_class->n_attrs() - 1, obj->time); 
  objcpy(old_obj_modify, obj, the_class->n_attrs() - 1);
}

/**
 * @brief This is the main engine loop. It propagates an event over an object through the nodes net
 *    The event type is called the "tag" and can be INSERT_TAG, MODIFY_TAG, od RETRACT_TAG
 *    In case of MODIFY_TAG, first a call to engine_modify must be done with the object before being modified
 *    In case or RETRACT_TAG, after this call the object can be freed due no additional references will remain in the net's nodes
 * 
 * @param tag INSERT_TAG, MODIFY_TAG, od RETRACT_TAG
 * @param obj The object to be propagated
 */
PUBLIC
void engine_loop(int tag, ObjectType *obj)
{
  Action *act;

  static int in_the_loop = FALSE;

  if (tag == MODIFY_TAG)
  {
    act = new Action(tag, new Single(obj), NULL, old_obj_modify, TRUE);
    act->objswap();
    act->push();
  }
  else
  {
    act = new Action(tag, new Single(obj), NULL, obj, TRUE);
    act->push();
  }


  // To avoid additional propagations due to calls to this functions made by example in external functions 
  // this lock assure that the rest of calls will be queued until this propagation ends
  if (!in_the_loop)
  {
    in_the_loop = TRUE;
    do_loop(Action::main_list(), TRUE);
    in_the_loop = FALSE;
  }
}

/**
 * @brief This is the top function to perform a propagation of an Action in the net 
 *        and all the derived Actions until the conflict set will be empty
 * 
 * @param my_init 
 * @param first_loop 
 * @return PUBLIC 
 */
PUBLIC
void do_loop(Action *&my_init, int first_loop)
{
  int not_empty_cs;
  Action **new_init;
  Action *act;

  if (my_init == NULL)
    return;

  if ((act = Action::pop(my_init)) == NULL)
    return;

  if (act->_tag == MODIFY_TAG)
  {
    if (!act->_single->has_been_deleted())
      act->objswap();
  }

  new_init = Action::tail();

  // If the object linked to the Single has been already deleted, it is freed everything
  // and no propagation is done
  if (act->_single->has_been_deleted())
  {
    if (trace >= 2)
    {
      fprintf(trace_file, "PROPAGATION OF DELETED OBJECT TAG %d, LINKS=%d OBJ = 0x%lx = ",
             act->_tag, act->_single->links(), (unsigned long int)act->_single->obj());
      act->_single->print(stdout, print_objkey);
      fprintf(trace_file, "\n");
    }

    if (act->_tag == RETRACT_TAG && act->_context != NULL)
      free(act->_context);

    delete act;
    return;
  }

  if (trace >= 2)
  {
    fprintf(trace_file, "PROPAGACION TAG %d LINKS=%d OBJ = 0x%lx = ",
           act->_tag, act->_single->links(), (unsigned long int)act->_single->obj());
    act->_single->print(stdout, print_objkey);
    fprintf(trace_file, "\n = ");
    act->_single->print(stdout, print_obj);
    fprintf(trace_file, "\n");
  }

  Status st_aux(act);
  if (act->_tag == MODIFY_TAG)
  {
    ExecData data(st_aux, (MetaObj *&)act->_single, NULL, MODIFY_TAG, act->_side, act->_pos);
    act->_node->propagate_modify(data, act->_codep);
    act->_single = (Single *&)data.left;
  }

  else
  {
    ExecData data(st_aux, (MetaObj *&)act->_single, NULL, act->_tag, act->_side, act->_pos);
    act->_node->propagate(data, act->_codep);
    act->_single = (Single *&)data.left;
  }

  if (!first_loop && act->_tag == RETRACT_TAG && act->_from_the_root)
  {
    act->_single->set_deleted();
  }

  // Go deeper with the event, execute a rule and then repeat it with all the derived events until the Conflict Set will be empty
  do
  {

    not_empty_cs = Node::run_cs();

    // The actions are communicated by calling to object_created. modified or deleted
    // Creation or Modification before propagation, only deletion is done after propagation
    for (Action *inf_act = *new_init; inf_act != NULL; inf_act = inf_act->_next)
    {
      switch (inf_act->_tag)
      {
        case INSERT_TAG:
          if ((inf_act->_node == ObjClass::get_real_root()) && inf_act->_codep == NULL)
          {
            if (trace) // Only if the propagation are from the root, it is communicated
            {
              fprintf(trace_file, "OBJECT CREATED ");
              inf_act->_single->print(trace_file, print_obj);
              fprintf(trace_file, "\n");
              check_trace_file_size();
            }
            object_created(inf_act->_single->obj(), inf_act->_context, inf_act->_n_objs_in_ctx);
          }
          break;
        case MODIFY_TAG:
          if ((inf_act->_node == ObjClass::get_real_root()) && inf_act->_codep == NULL)
          {
            if (trace) // Only if the propagation are from the root, it is communicated
            {
              fprintf(trace_file, "OBJECT MODIFIED ");
              inf_act->_single->print(trace_file, print_obj);
              fprintf(trace_file, "\n");
              check_trace_file_size();
            }
            object_modified(inf_act->_single->obj(), inf_act->_context, inf_act->_n_objs_in_ctx);
            inf_act->objswap();
          }
          break;
        case RETRACT_TAG:
          if ((inf_act->_node == ObjClass::get_real_root()) && inf_act->_codep == NULL)
          {
            if (trace) // Only if the propagation are from the root, it is communicated
            {
              fprintf(trace_file, "OBJECT DELETED ");
              inf_act->_single->print(trace_file, print_obj);
              fprintf(trace_file, "\n");
              check_trace_file_size();
            }
          }

          // RETRACT is communicated after propagation
          break;
      }
    }

    while (*new_init != NULL)
      do_loop(*new_init, FALSE);

  } while (not_empty_cs);

  // Communicate the retraction to the user at the end of the inferece, in reverse order
  // Only the objects not passed initially (!first_loop) that has been propagated
  // from the root (init_code_p == curr_pkg->code) of those that are not going to stay in the
  // system (curr_single->links()==1 are communicated to the user (object deleted event).
  // The single is marked as deleted if the final unlink does not delete it. This way 
  // further inferences with this object are avoided (see control has_been_deleted at the 
  // beginning of the loop)
  
  if (!first_loop && act->_tag == RETRACT_TAG && act->_from_the_root)
  {
    act->_single->set_deleted();
    if (trace)
    {
      fprintf(trace_file, "OBJECT DELETED IS COMMUNICATED ");
      act->_single->print(trace_file, print_obj);
      fprintf(trace_file, "\n");
    }
    object_deleted(act->_single->obj());
  }

  else if ((act->_tag != RETRACT_TAG || !act->_from_the_root) &&
           act->_single->links() == 1 && !act->_single->has_been_deleted())
  {
    if (trace)
    {
      fprintf(trace_file, "OBJECT NO MORE USED ");
      act->_single->print(trace_file, print_obj);
      fprintf(trace_file, "\n");
    }
    object_no_more_used(act->_single->obj());
  }
  delete act;
}

//
// Helpers for tracing
//

/**
 * @brief Print a unique key associated to an object to make it more easy to identify
 * 
 * @param file Where to write it
 * @param obj 
 */
PUBLIC
void print_objkey(FILE *file, const ObjectType *obj)
{
  fprintf(file, "%s ", clave(obj));
}

/**
 * @brief Print an Object
 * 
 * @param file Where to write it
 * @param obj The objet to print
 */
PUBLIC
void print_obj(FILE *file, const ObjectType *obj)
{

  ObjClass *the_class;
  int nat;

  if (obj == NULL)
  {
    fprintf(file, " ** NULL OBJECT ** ");
    return;
  }

  fprintf(file, "%s(", STR(obj->attr[0].str.str_p));

  the_class = *ObjClass::get_class(obj->attr[0].str.str_p);

  if (the_class != NULL)
  {

    for (nat = 1; nat < the_class->n_attrs(); nat++)
    {
      fprintf(file, "%s", the_class->attr_name(nat));
      switch (the_class->attr_type(nat))
      {
      case TYPE_CHAR:
        if (obj->attr[nat].num < ' ')
          fprintf(file, " '\\%ld'", obj->attr[nat].num);
        else
          fprintf(file, " '%c'", (char)obj->attr[nat].num);
        break;
      case TYPE_BOOL:
        fprintf(file, " %ldT",
                obj->attr[nat].num);
        break;
      case TYPE_NUM:
        fprintf(file, " %ld",
                obj->attr[nat].num);
        break;
      case TYPE_PATTERN:
        fprintf(file, " %04lX",
                obj->attr[nat].num);
        break;
      case TYPE_STR:
        fprintf(file, " \"%s\"",
                STR(obj->attr[nat].str.str_p));
        break;
      case TYPE_FLO:
        fprintf(file, " %g",
                obj->attr[nat].flo);
        break;
      }
      if (nat < the_class->n_attrs() - 1)
        fprintf(file, ", ");
    }
    fprintf(file, "), Time=%ld", obj->time);
  }
  fflush(file);
}

/**
 * @brief Generates a different four characters key for every different object passed and store it in a btree
 * 
 * @param vect The object passed
 * @return const char * with the key
 */
PUBLIC
const char *clave(const void *vect)
{
  static char sbuffer[128];
  static char *com_str = sbuffer;
  vector_key vk, **vf, *vn;
  static BTree tree;

  static char key[5];
  static int nvect = 0;

  if (vect == NULL)
    return "(null)";

  com_str += 5;
  if (com_str - sbuffer > 120)
  {
    com_str = sbuffer;
  }

  vk._vector = vect;

  vf = (vector_key **)tree.Insert((void *)&vk, vectors_compare);

  if (*vf == &vk)
  {
    vn = (vector_key *)malloc(sizeof(vector_key));

    vn->_vector = vect;
    strncpy(vn->_key, get_key_of_nvector(nvect), 4);
    *vf = vn;
    if (nvect++ > MAX_NVECT)
      engine_fatal_err("Max number of vectors reached !\n");
  }
  strncpy(com_str, (*vf)->_key, 4);
  com_str[4] = '\0';

  return com_str;
}

/**
 * @brief Get the key for a given vector number 
 * 
 * @param nvector 
 * @return char * the key
 */
PUBLIC
char *get_key_of_nvector(int nvector)
{
  static char key[5];
  char rkey[5];
  int c;
  int r;
  int n = 0;
  int i;

  do
  {
    c = nvector / 26;
    r = nvector - c * 26;
    rkey[n++] = 'A' + r;
    nvector = c - 1;
  } while (c != 0);

  for (i = 0; i < n; i++)
    key[i] = rkey[n - i - 1];
  key[i] = '\0';

  return key;
}

/**
 * @brief Compare function used ito order the vectors in the vectors tree
 * 
 * @param vec1 
 * @param vec2 
 * @param list 
 * @return int 
 */
PUBLIC
int vectors_compare(const void *vec1, const void *vec2, va_list list)
{
  return (long int)((vector_key *)vec1)->_vector - (long int)((vector_key *)vec2)->_vector;
}

/**
 * @brief Get one obj of a Set MetaObj.
 *        This function will walk over the tree and will return all the objects in it
 * 
 * @param tree_state Used to walk the BTRee of the Set to get an Object in it
 * @return the Object found 
 */
PUBLIC
ObjectType *get_obj_of_set(BTState *tree_state)
{

  Single *mysingle;

  mysingle = (Single *)BTree::Walk(*tree_state);

  if (mysingle != NULL)
    return mysingle->obj();
  else
    return (ObjectType *)NULL;
}

/**
 * @brief RETRACT all the elements in a Set
 * 
 * @param tree_state  The state used to walk the Set
 */
PUBLIC
void eng_destroy_set(BTState *tree_state)
{
  MatchCount *counter;

  while ((counter = (MatchCount *)BTree::Walk(*tree_state)) != NULL)
  {
    Single *mysingle = counter->item->single();
    Action *act;

    mysingle->link(); // to keep it alive while the inference is processed
    act = new Action(RETRACT_TAG, mysingle, NULL, mysingle->obj(), FALSE);
    act->push();
  }
  end_obj_of_set(tree_state);
}

/**
 * @brief Get the number of items of a Set from the tree state
 * 
 * @param tree_state 
 * @return int
 */
PUBLIC
int n_obj_of_set(BTState *tree_state)
{
  return (tree_state->nitems());
}

/**
 * @brief Delete the Tree State after delete all the elements in it
 * 
 * @param tree_state 
 */
PUBLIC
void end_obj_of_set(BTState *tree_state)
{
  delete tree_state;
}

/**
 * @brief Perform a Loop taking the first peding Action
 * 
 * @param first_loop: if it is the very first iteraction or is a derived one
 */
PUBLIC
void Do_loop(int first_loop)
{
  do_loop(Action::main_list(), first_loop);
}

/**
 * @brief Print an MetaObj and its time window
 * 
 * @param item 
 * @param ret 
 * @param list 
 */
PRIVATE
void print_item(const void *item, int *ret, va_list list)
{
  MetaObj *RightItem;
  RightItem = (MetaObj *)item;

  fprintf(trace_file, "TIME [%ld %ld] ", RightItem->t1(), RightItem->t2());
  RightItem->print(stdout, print_objkey);
  fprintf(trace_file, "\n");
}

/**
 * @brief Copy an ObjectType to another
 * 
 * @param dest Destination Object
 * @param ori Original Object
 * @param n_attrs Number of Attributes
 */
PUBLIC
void objcpy(ObjectType *dest, ObjectType *ori, int n_attrs)
{
  int n;

  for (n = 0; n <= n_attrs; n++) // Note that it is needed to use <= !!!!
    dest->attr[n] = ori->attr[n];

  dest->user_data = ori->user_data;
  dest->time = ori->time;
}

/**
 * @brief Create a new Object Type
 * 
 * @param n_attrs Numer of Attributes
 * @param time_val Timestamp
 * @return ObjectType* The object created
 */
PUBLIC
ObjectType *new_object(int n_attrs, long time_val)
{
  ObjectType *obj;
  int n;

  obj = (ObjectType *)malloc(sizeof(ObjectType) + sizeof(Value) * (n_attrs));

  if (obj == (ObjectType *)0)
  {
    engine_fatal_err("malloc: %s\n", strerror(errno));
  }

  for (n = 0; n <= n_attrs; n++)
  {
    obj->attr[n].str.str_p = 0; // 0 .num
    obj->attr[n].str.dynamic_flags = 0;
  }
  obj->user_data = NULL;
  obj->time = time_val;
  return obj;
}
