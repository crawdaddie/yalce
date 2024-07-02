#ifndef _LANG_TYPE_TYPE_H
#define _LANG_TYPE_TYPE_H

#include <stdbool.h>
enum TypeKind {
  /* Type Operator */
  T_INT,
  T_NUM,
  T_STRING,
  T_BOOL,
  T_VOID,
  T_FN,
  T_PAIR,
  T_TUPLE,
  T_LIST,
  T_CONS,
  /* Type Variable  */
  T_VAR,
};

typedef struct Type {
  enum TypeKind kind;
  union {
    // Type Variables (T_VAR):
    // They represent unknown types that can be unified with other types during
    // inference.
    char *T_VAR; // for TYPE_VARIABLE
    struct {
      char *name;
      struct Type **args;
      int num_args;
    } T_CONS; // for TYPE_CONSTRUCTOR
    //
    struct {
      struct Type *from;
      struct Type *to;
    } T_FN; // for TYPE_FUNCTION
  } data;

} Type;

// A TypeScheme represents a polymorphic type in Hindley-Milner type systems.
// It consists of two parts:
// A set of type variables that are universally quantified.
// A type expression that may contain these quantified
// variables.
// Purpose of TypeSchemes
// TypeSchemes allow us to express polymorphic types, which are types that can
// work with multiple more specific types.
// For example, the identity function `id = λx.x` can work with any type,
// so its type scheme would be ∀α. α -> α.
//
// How TypeSchemes Are Used
//
// Generalization: When we define a let-bound variable, we generalize its type
// into a TypeScheme. This process quantifies over any type variables that are
// not already bound in the current type environment. Instantiation: When we use
// a variable with a polymorphic type, we instantiate its TypeScheme. This
// creates a fresh copy of the type, replacing the quantified variables with new
// type variables.
//
// Example
// Consider the identity function id = λx.x. Here's how it would be processed:
//
// Inference:
//
// We create a fresh type variable, say α, for the parameter x.
// The body of the function is just x, so its type is also α.
// The inferred type for id is α -> α.
//
//
// Generalization:
//
// Since α is not bound in the outer environment, we can quantify over it.
// We create a TypeScheme: ∀α. α -> α.
//
//
// Usage and Instantiation:
//
// When we use id, say in id 5 and id "hello", we instantiate the scheme.
// For id 5, we create a fresh type variable, say β, and instantiate to β -> β.
// Then we unify β with Int, resulting in Int -> Int.
// For id "hello", we do the same, but unify with String, getting String ->
// String.
typedef struct TypeScheme {
  Type *variables; // A set of type variables that are universally quantified.
  int num_variables;
  Type *type; // A type expression that may contain these quantified variables
} TypeScheme;

// TypeEnv represents a mapping from variable names to their types
//
// In the case of TypeEnv = Linked List:
//    TypeEnvNode(t1, scheme1) ->
//    TypeEnvNode(t2, scheme2) ->
//    TypeEnvNode(t3, scheme3)
typedef struct TypeEnvNode {
  const char *name;
  TypeScheme *scheme;
  struct TypeEnvNode *next;
} TypeEnvNode;

// TypeEnv represents a mapping from variable names to their types
//
// you can add a mapping to an env with
// `extend_env(env, name, type_scheme);`
// and lookup a type with
// `lookup_env(env, name);`
typedef TypeEnvNode *TypeEnv;

extern Type t_int;
extern Type t_num;
extern Type t_string;
extern Type t_bool;
extern Type t_void;

bool unify(Type *t1, Type *t2);
TypeScheme *generalize(TypeEnv *env, Type *type);
Type *instantiate(TypeScheme *scheme);

#endif
