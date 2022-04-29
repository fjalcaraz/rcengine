
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <engine.h>

#include <tester.h>

PUBLIC
void callbackfunc(int tag, ObjectType *obj, ObjectType **ctx, int n_objs)
{
   int n;

   switch (tag)
   {
     case WHEN_INSERTED:
        insert_new_obj(obj);
        if (ctx!=(ObjectType**)NULL)
          free(ctx);
        break;

     case WHEN_MODIFIED:
        if (ctx!=(ObjectType**)NULL)
          free(ctx);
        break;

     case WHEN_RETRACTED:
        if (ctx!=(ObjectType**)NULL)
          free(ctx);
        delete_new_obj(obj);
        break;

     case WHEN_NOT_USED:
        delete_new_obj(obj);
        break;
   }
}


extern void insert_new_obj(ObjectType *obj);

void test_func(Value *stack, int)
/* Funcion externa: FUNCTION test_func(INTEGER) = INTEGER */
{
	printf("Extern function call: param = %ld\n",stack[0].num);
	/* Devolvemos un valor clave, ej 62 */
	stack[0].num = 62;
	return;
}

void test_func2(Value *stack, int)
/* Procedure externo: PROCEDURE test_func2(STRING) */
{
   if (stack[0].str.str_p != NULL)
   {
      printf("Extern procedure called: param = %s\n",stack[0].str.str_p);
      if (stack[0].str.dynamic_flags == DYNAMIC)
        free(stack[0].str.str_p);
   }
   else printf("Extern procedure called with NULL param\n");

   return;
}


void bd_extraer(Value *stk, int)
{
printf("ARG1 _%s_  ARG2 _%s_  ARG3 _%s_\n", stk[0].str.str_p, stk[1].str.str_p, stk[2].str.str_p);
  if (stk[0].str.dynamic_flags == DYNAMIC)
     free(stk[0].str.str_p);
  if (stk[1].str.dynamic_flags == DYNAMIC)
     free(stk[1].str.str_p);
  if (stk[2].str.dynamic_flags == DYNAMIC)
     free(stk[2].str.str_p);
stk[0].str.dynamic_flags = DYNAMIC;
stk[0].str.str_p=strdup("PEPITO");
}

void bd_damesecuencia(Value *stk, int)
{
  static int sec_alarma=1;
  static int sec_llamada=1;

  if(stk[0].num == 0){
    if(strcmp(stk[1].str.str_p,"SEC_ALARMA") == 0)
      stk[0].num = sec_alarma++;
    else
      stk[0].num =  sec_llamada++;
  }

  if (stk[1].str.dynamic_flags == DYNAMIC)
     free(stk[1].str.str_p);
}
 
void genera_llamadas(Value *stk, int)
{
  char *cadena;
  char *newcadena;
  void *set1;
  int n;
  static int my_id=0;
  int t;
  int i;
  char *chp;
  int div;
  int secuencia;
  ObjectType *obj;

  printf("AQUI ESTOY\n");

  for(i=0; i<stk[0].num; i++)
  {
    obj= new_object(6, stk[1].num +  lrand48()); 
    obj->attr[0].str.str_p = (char *)"llamada";
    obj->attr[0].str.dynamic_flags = 0;
    obj->attr[1].str.str_p = (char *)"LEON";
    obj->attr[1].str.dynamic_flags = 0;
    obj->attr[2].num = 0;
    obj->attr[3].num = 0;

    chp=(char *)malloc(3);
    sprintf(chp,"S%ld", lrand48() & 1);
    obj->attr[4].str.str_p = chp;
    obj->attr[4].str.dynamic_flags = DYNAMIC;

    chp=(char *)malloc(3);
    sprintf(chp,"F%ld", lrand48() & 1);
    obj->attr[5].str.str_p = chp;
    obj->attr[5].str.dynamic_flags = DYNAMIC;

    obj->attr[6].num = 0;

    insert_new_obj(obj);

    engine_loop(INSERT_TAG, obj);
  }
}


