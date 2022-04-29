
#include <engine.h>

extern int context;
extern int danger;
void *class_of(char *name, int &n_attrs);
void insert_new_obj(ObjectType *obj);
void delete_new_obj(ObjectType *obj);
void callbackfunc(int tag, ObjectType *obj, ObjectType **ctx, int n_objs);
