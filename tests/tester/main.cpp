
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
 

#include <engine.h>

#include <user.h>
#include <tester.h>
#include <btree.hpp>

#define FALSE 0
#define TRUE 1


PRIVATE BTree obj_tree;
PRIVATE BTree obj_new_tree;
PRIVATE long curr_time = 0;

PUBLIC int context = 0;
PUBLIC int danger = 0;
PUBLIC int free_p = 0;

time_t time(time_t *tloc)
{
  return (time_t)curr_time;
}

void *class_of(char *name, int &n_attrs)
{
  int n;
  void *cr;
  cr=get_class(name,&n_attrs);

  if (cr==NULL)
  {
     fprintf(stderr, "ERROR: class %s not found\n", name);
     exit(1);
  }
 
  return (cr);
}

void attr_info(void *clase, char *attr, int &num, int &type)
{
  int na;

  na=attr_index(clase,attr);

  if (na == -1)
  {
     fprintf(stderr, "ERROR: attribute %s not found\n",attr);
     exit(1);
  }

  num = na;

  type=attr_type(clase,na);
}

 

ObjectType *
read_obj(FILE *file)
{
 char *classname, *attrname, *val;
 char buffer[200];
 ObjectType *obj;
 long tiempo;
 void *clase;
 int attr, attr_type, n_attrs;
 char *res;
 
 obj = NULL;

 do { 
   res = fgets(buffer, 200, file); 
 } while (res != NULL && (*buffer=='\n' || *buffer==';'));

 if (res != NULL)
 {

   tiempo = atol(strtok(buffer, " "));
   curr_time += tiempo;

   classname = strtok(NULL, " (\n");

   clase = class_of(classname, n_attrs);

   obj= new_object(n_attrs-1, curr_time);
   obj->attr[0].str.str_p = strdup(classname);
   obj->attr[0].str.dynamic_flags = DYNAMIC;

   do 
   {
        attrname=strtok(NULL,"( ,");
        if (attrname!= NULL && *attrname != ')' && *attrname != '\n')
        {
          attr_info(clase, attrname, attr, attr_type);

          if (attr_type == TYPE_STR)
             val =strtok(NULL,",)\n");
          else
             val =strtok(NULL," ,)\n");
          switch(attr_type)
          {
            case TYPE_FLO :
                obj->attr[attr].flo = atof(val);
                break;
            case TYPE_NUM  :
            case TYPE_BOOL :
                obj->attr[attr].num = atoi(val);
                break;
            case TYPE_CHAR :
                obj->attr[attr].num = val[0];
                break;
            case TYPE_STR :
                obj->attr[attr].str.str_p = strdup(val);
                obj->attr[attr].str.dynamic_flags = DYNAMIC;
                break;
          }
        }
   } while ( attrname!= NULL && *attrname != ')' && *attrname != '\n');
 }
 return obj;
}

       
void
free_obj(ObjectType *obj)
{
   int n, na;
   void *cr;


   cr=get_class(obj->attr[0].str.str_p, &na);

   for (n=0; n<na;  n++){
     if(attr_type(cr,n)==TYPE_STR &&
         obj->attr[n].str.dynamic_flags == DYNAMIC)
     free (obj->attr[n].str.str_p);
   }
   free (obj);
}


int compare_obj(const void *obj1, const void *obj2, va_list)
{
  return (long)((ObjectType *)obj1) - (long)((ObjectType *)obj2);
}
    
void
insert_obj(ObjectType *obj)
{
   obj_tree.Insert(obj, compare_obj);
}

void
insert_new_obj(ObjectType *obj)
{
   if (danger)
   {
     printf("New Object\n");
     //sleep(2);
     //return;
   }
   obj_new_tree.Insert(obj, compare_obj);
}

void
delete_new_obj(ObjectType *obj)
{
   ObjectType *obj_found=NULL;

   if (danger)
   {
     printf("Delete new Object\n");
     //sleep(2);
     //return;
   }

   // Para no interferir con la liberacion
   if ((obj_found = (ObjectType *)obj_new_tree.Delete(obj, compare_obj)) == NULL)
   {
      obj_found = (ObjectType *)obj_tree.Delete(obj, compare_obj);
      printf("WAS ORIGINAL:");
   } else printf("WAS GENERATED:");

   if (obj_found != NULL)
   {
     free_obj(obj_found);
     puts(" FOUND");
   }
   else
   {
     puts(" NOT FOUND/ DONE!!!");
   }
}

void
delete_tree_item(void *obj, va_list)
{
   printf("AN OBJECT IS RETRACTED ");
   print_obj(stdout, ((ObjectType *)obj)); printf("\n");
   engine_loop(RETRACT_TAG, ((ObjectType *)obj));
   free_obj((ObjectType *)obj);

}
   
void
delete_all()
{
   printf("FINAL DELETE_ALL:\n");
   danger = 0;
   printf("\tORIGINAL OBJECTS:\n");
   obj_tree.Free(delete_tree_item);
   danger = 1;
   printf("\tGENERATED OBJECT:\n");
   obj_new_tree.Free(delete_tree_item);
   danger = 0;
}

void print_net();


int
main(int argc, char *argv[])
{

   int c, n_objs;
   ObjectType *obj;
   FILE *entrada, *f_obj;
   char *fich_entrada, *fich_objs;
   int len_entrada;
   int retract = FALSE;
   int pnet=0;

   extern int optind;
   extern char *optarg;
   FILE *fi;
   fich_objs = NULL;

   set_comp_warnings(1);
 
   while ((c=getopt(argc,argv,"cfpthi:r")) != -1)
   {
     switch(c)
     {
       case 'p':
          pnet=1;
          break;
       case 'c':
          context=1;
          break;
       case 't':
          set_trace_level(1);
          break;
       case 'i':
          fich_objs = optarg;
          break;
       case 'r':
	      retract = TRUE;
	      break;
       case 'f':
	      free_p = TRUE;
	      break;
       case 'h':
       case '?':
	      printf("Usage : %s [-p][-h][-t][-r][f][-i objfile] rulesfile\n", argv[0]);
          printf(" -p : Print the nodes net\n");
          printf(" -t : Enable traces in /tmp/engine.log\n");
          printf(" -i objfile : Define an input objects file\n");
          printf(" -r : Retract all the objects that remain still alive\n");
          printf(" -f : Free the package at the end\n");
          printf(" -h : Show this help\n");
          printf(" rulesfile: Input rules file (package)\n");
          exit(0);

     }
   }

   if (argc==optind)
   {
      fprintf(stderr, "Usage : %s [-p][-h][-t][-r][f][-i objfile] rulesfile\n", argv[0]);
      fprintf(stderr, "Usage : %s -h for help\n", argv[0]);
      fprintf(stderr, "You must indicate a rules file\n");
      exit(1);   
   }

   if (fich_objs != NULL)
   {
     if ((f_obj = fopen(fich_objs, "r")) == NULL)
     {
       fprintf(stderr,
           "Bad objects file\n");
       exit(1);
     }
   }
   else
    f_obj = stdin;

   fich_entrada = argv[optind];
   len_entrada = strlen(fich_entrada);
   srand48(10L);

   if (strcmp(fich_entrada + len_entrada - 2, ".r") != 0)
   {
        fprintf(stderr,  
           "%s : Bad file extension. Only \".r\" allowed\n", fich_entrada);
        exit(1);   
   }    

   add_callback_func(WHEN_ALL, callbackfunc);
   def_function("genera_llamadas", genera_llamadas);
   def_function("bd_damesecuencia", bd_damesecuencia);
   def_function("bd_extraer", bd_extraer);
   def_function("test_func", test_func);
   def_function("test_func2", test_func2);

   if (load_pkg(fich_entrada) == 0)
     exit(1);

   printf("Code read\n");

   if(pnet) 
     print_net();

   reset_inf_cnt();
   
   n_objs=0;
   // int _mynum=0;
   puts("INSERTION BEGINS ------------------");
   while ((obj = read_obj(f_obj)) != NULL)
   {
     // _mynum++;
      printf("AN OBJECT IS INSERTED (%d) T=%ld ", n_objs++, obj->time); print_obj(stdout, obj); printf("\n");

      insert_obj(obj);
      engine_loop(INSERT_TAG, obj);

      // Just to test some modifications
      /*
      if (_mynum == 2) {
      printf("OBJETO MODIFICADO !!!!\n");
      engine_modify(obj);
      engine_loop(MODIFY_TAG, obj);
      }
      */

//sleep(2);
   }

   if (f_obj != stdin)
     fclose(f_obj);


   if (retract)
   {
      int nat;
      puts("RETRACTION BEGINS ------------------");
      delete_all();

   }

   fprintf(stdout, "\n%d inferences done\n", get_inf_cnt());
//engine_refresh(0);

   del_callback_func(WHEN_ALL, callbackfunc);

   if (free_p) {
      fprintf(stdout, "FREEING THE PACKAGE\n");
      free_pkg();
   }

//trace   Twalk();
   
//   printf("Pulsa una tecla para finalizar ..."); fflush(stdout); getchar();
   return 0;

}
