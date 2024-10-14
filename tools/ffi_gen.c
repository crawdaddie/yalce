#include <clang-c/Index.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// scans a list of header files and prints corresponding ylc extern declarations to stdout 

typedef struct name_lookup {
  const char *key;
  const char *val;
  struct name_lookup *next;
} name_lookup;

name_lookup *lookups_extend(name_lookup *lookups, const char *key,
                            const char *value) {
  name_lookup *l = malloc(sizeof(name_lookup));
  l->key = key;
  l->val = value;
  l->next = lookups;
  return l;
}

const char *lookup(name_lookup *lookups, const char *key) {
  while (lookups) {
    if (strcmp(lookups->key, key) == 0) {
      return lookups->val;
    }
    lookups = lookups->next;
  }
  return NULL;
}

const char *yalce_name(name_lookup *lookups, const char *c_name) {
  const char *name = lookup(lookups, c_name);
  if (name) {
    return name;
  }
  return c_name;
}

void print_function_decl(CXCursor cursor, name_lookup *lookups) {

  CXString raw_comment = clang_Cursor_getRawCommentText(cursor);
  const char *comment_text = clang_getCString(raw_comment);
  if (comment_text) {
    printf("#%s\n", comment_text + 3);
  }

  CXString func_name = clang_getCursorSpelling(cursor);
  CXType func_type = clang_getCursorType(cursor);
  CXString return_type = clang_getTypeSpelling(clang_getResultType(func_type));

  printf("let %s = extern fn ", clang_getCString(func_name));

  int num_args = clang_Cursor_getNumArguments(cursor);
  if (num_args == 0) {
    printf("() -> ");
  } else {
    for (int i = 0; i < num_args; ++i) {
      CXCursor arg = clang_Cursor_getArgument(cursor, i);
      CXType arg_type = clang_getCursorType(arg);
      CXString arg_type_str = clang_getTypeSpelling(arg_type);
      CXString arg_name = clang_getCursorSpelling(arg);

      printf("%s", yalce_name(lookups, clang_getCString(arg_type_str)));
      printf(" -> ");

      clang_disposeString(arg_type_str);
      clang_disposeString(arg_name);
    }
  }

  printf("%s", yalce_name(lookups, clang_getCString(return_type)));

  printf(";\n");

  clang_disposeString(func_name);
  clang_disposeString(return_type);
}

void print_struct_decl(CXCursor cursor, name_lookup *lookups) {
  CXString struct_name = clang_getCursorSpelling(cursor);
  CXType struct_type = clang_getCursorType(cursor);
  printf("STRUCT: %s\n", clang_getCString(struct_name));
}

void print_enum_decl(CXCursor cursor, name_lookup *lookups) {
  CXString struct_name = clang_getCursorSpelling(cursor);
  // CXType struct_type = clang_getCursorType(cursor);
  printf("ENUM: %s\n", clang_getCString(struct_name));
}

enum CXChildVisitResult visitor(CXCursor cursor, CXCursor parent,
                                CXClientData client_data) {
  if (clang_Location_isFromMainFile(clang_getCursorLocation(cursor)) == 0) {
    return CXChildVisit_Continue;
  }

  if (clang_getCursorKind(cursor) == CXCursor_FunctionDecl) {

    name_lookup *lookups = client_data;
    print_function_decl(cursor, lookups);
  }

  if (clang_getCursorKind(cursor) == CXCursor_StructDecl) {

    name_lookup *lookups = client_data;
    print_struct_decl(cursor, lookups);
  }


  if (clang_getCursorKind(cursor) == CXCursor_EnumDecl) {

    name_lookup *lookups = client_data;
    print_enum_decl(cursor, lookups);
  }
  return CXChildVisit_Recurse;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <header_files>\n", argv[0]);
    return 1;
  }

  name_lookup *lookups = NULL;
  lookups = lookups_extend(lookups, "int", "Int");
  lookups = lookups_extend(lookups, "double", "Double");
  lookups = lookups_extend(lookups, "void", "()");
  lookups = lookups_extend(lookups, "uint64_t", "Uint64");
  lookups = lookups_extend(lookups, "uint32_t", "Int");
  lookups = lookups_extend(lookups, "bool", "Bool");
  lookups = lookups_extend(lookups, "char", "Char");
  lookups = lookups_extend(lookups, "char *", "Ptr");
  lookups = lookups_extend(lookups, "const char *", "Ptr");
  lookups = lookups_extend(lookups, "void *", "Ptr");

  // engine lib -specific lookups
  lookups = lookups_extend(lookups, "SignalRef", "Signal");
  lookups = lookups_extend(lookups, "NodeRef", "Synth");
  lookups = lookups_extend(lookups, "SchedulerCallback", "Ptr");

  for (int i = 1; i < argc; i++) {

    char *input_header = argv[i];

    printf("# %s\n", input_header);
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index, input_header, NULL, 0, NULL, 0, CXTranslationUnit_None);

    if (unit == NULL) {
      fprintf(stderr, "Unable to parse translation unit. Quitting.\n");
      return 1;
    }

    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(cursor, visitor, lookups);

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
    printf("\n");
  }

  return 0;
}
