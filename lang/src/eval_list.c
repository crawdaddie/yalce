#include "eval_list.h"
#include <stdlib.h>

Value eval(Ast *ast, ht *stack, int stack_ptr, val_bind_fn_t val_bind);

Value eval_list(Ast *ast, ht *stack, int stack_ptr, val_bind_fn_t val_bind) {
  Value val;
  int len = ast->data.AST_LIST.len;
  if (len == 0) {
    return (Value){VALUE_LIST,
                   .value = {.vlist = &(IntList){.items = NULL, .len = 0}}};
  }
  val.type = VALUE_LIST;
  Ast first_el_ast = ast->data.AST_LIST.items[0];
  Value first_el_value = eval(&first_el_ast, stack, stack_ptr, val_bind);

  switch (first_el_value.type) {
  case VALUE_INT: {
    val.value.vlist = malloc(sizeof(IntList));
    ((IntList *)val.value.vlist)->type = VALUE_INT;
    ((IntList *)val.value.vlist)->items = malloc(sizeof(int) * len);
    ((IntList *)val.value.vlist)->items[0] = first_el_value.value.vint;
    ((IntList *)val.value.vlist)->len = len;
    for (int i = 1; i < len; i++) {
      ((IntList *)val.value.vlist)->items[i] =
          eval(ast->data.AST_LIST.items + i, stack, stack_ptr, val_bind)
              .value.vint;
    }
    break;
  }

  case VALUE_BOOL: {
    val.value.vlist = malloc(sizeof(IntList));
    ((IntList *)val.value.vlist)->type = VALUE_BOOL;
    ((IntList *)val.value.vlist)->items = malloc(sizeof(bool) * len);
    ((IntList *)val.value.vlist)->items[0] = first_el_value.value.vbool;
    ((IntList *)val.value.vlist)->len = len;
    for (int i = 1; i < len; i++) {
      ((IntList *)val.value.vlist)->items[i] =
          eval(ast->data.AST_LIST.items + i, stack, stack_ptr, val_bind)
              .value.vbool;
    }

    break;
  }
  case VALUE_NUMBER: {
    val.value.vlist = malloc(sizeof(NumberList));
    ((NumberList *)val.value.vlist)->type = VALUE_NUMBER;
    ((NumberList *)val.value.vlist)->items = malloc(sizeof(double) * len);
    ((NumberList *)val.value.vlist)->items[0] = first_el_value.value.vnum;
    ((NumberList *)val.value.vlist)->len = len;
    for (int i = 1; i < len; i++) {
      ((NumberList *)val.value.vlist)->items[i] =
          eval(ast->data.AST_LIST.items + i, stack, stack_ptr, val_bind)
              .value.vnum;
    }

    break;
  }
  case VALUE_STRING: {
    val.value.vlist = malloc(sizeof(StringList));
    ((StringList *)val.value.vlist)->type = VALUE_STRING;
    ((StringList *)val.value.vlist)->items = malloc(sizeof(char *) * len);
    ((StringList *)val.value.vlist)->lens = malloc(sizeof(int) * len);
    ((StringList *)val.value.vlist)->hashes = malloc(sizeof(uint64_t) * len);
    ((StringList *)val.value.vlist)->items[0] = first_el_value.value.vstr.chars;

    ((StringList *)val.value.vlist)->lens[0] = first_el_value.value.vstr.length;

    ((StringList *)val.value.vlist)->hashes[0] = first_el_value.value.vstr.hash;

    ((StringList *)val.value.vlist)->len = len;
    for (int i = 1; i < len; i++) {
      Value str_val =
          eval(ast->data.AST_LIST.items + i, stack, stack_ptr, val_bind);
      ((StringList *)val.value.vlist)->items[i] = str_val.value.vstr.chars;
      ((StringList *)val.value.vlist)->lens[i] = str_val.value.vstr.length;
      ((StringList *)val.value.vlist)->hashes[i] = str_val.value.vstr.hash;
    }

    break;
  }
  case VALUE_LIST: {
    val.value.vlist = malloc(sizeof(ObjList));
    ((ObjList *)val.value.vlist)->type = VALUE_LIST;
    ((ObjList *)val.value.vlist)->items = malloc(sizeof(void *) * len);
    ((ObjList *)val.value.vlist)->items[0] = first_el_value.value.vlist;
    ((ObjList *)val.value.vlist)->len = len;
    for (int i = 1; i < len; i++) {
      ((ObjList *)val.value.vlist)->items[i] =
          eval(ast->data.AST_LIST.items + i, stack, stack_ptr, val_bind)
              .value.vlist;
    }

    break;
  }
  case VALUE_OBJ: {
    val.value.vlist = malloc(sizeof(ObjList));

    ((ObjList *)val.value.vlist)->type = VALUE_LIST;
    ((ObjList *)val.value.vlist)->items = malloc(sizeof(void *) * len);
    ((ObjList *)val.value.vlist)->items[0] = first_el_value.value.vobj;
    ((ObjList *)val.value.vlist)->len = len;
    for (int i = 1; i < len; i++) {
      ((ObjList *)val.value.vlist)->items[i] =
          eval(ast->data.AST_LIST.items + i, stack, stack_ptr, val_bind)
              .value.vobj;
    }

    break;
  }
  }
  return val;
}
