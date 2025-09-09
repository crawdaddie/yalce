#ifndef _LANG_TYPE_BUILTINS_H
#define _LANG_TYPE_BUILTINS_H
#include "ht.h"
#include "type.h"
#include "types/inference.h"
extern ht builtin_types;
void initialize_builtin_types();
void add_builtin(char *name, Type *t);

void print_builtin_types();

Type *lookup_builtin_type(const char *name);

extern Scheme void_scheme;
extern Scheme array_at_scheme;

extern Scheme arithmetic_scheme;
extern Scheme eq_scheme;
extern Scheme ord_scheme;

extern Scheme opt_scheme;

extern Scheme list_prepend_scheme;
extern Scheme list_of_scheme;
extern Scheme array_of_scheme;
extern Scheme array_set_scheme;
extern Scheme array_size_scheme;
extern Type t_builtin_print;

extern Scheme array_of_scheme;
extern Scheme array_set_scheme;
extern Scheme array_size_scheme;
extern Scheme list_concat_scheme;
extern Scheme cor_map_scheme;
extern Scheme iter_of_list_scheme;
extern Scheme id_scheme;

Scheme *lookup_builtin_scheme(const char *name);
#endif
