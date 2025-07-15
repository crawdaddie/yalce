#include <clang-c/Index.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// scans a list of header files and prints corresponding ylc extern declarations
// to stdout
//
// struct lookup_t {
//   char *c_name;
//   char *ylc_name;
// };
//
// static struct lookup_t LOOKUPS[] = {
//     {"int", "Int"},         {"double", "Double"}, {"void", "()"},
//     {"uint64_t", "Uint64"}, {"uint32_t", "Int"},  {"bool", "Bool"},
//     {"char", "Char"},       {"char *", "Ptr"},    {"const char *", "Ptr"},
//     {"void *", "Ptr"},      {"double *", "Ptr"},
// };

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

void print_function_type(CXType type, name_lookup *lookups) {
  CXType result_type = clang_getResultType(type);

  int num_args = clang_getNumArgTypes(type);
  if (result_type.kind == CXType_Void && num_args == 0) {
    printf("() -> ()");
    return;
  } else {
    int num_args = clang_getNumArgTypes(type);
    for (int i = 0; i < num_args; ++i) {
      CXType arg_type = clang_getArgType(type, i);
      CXString arg_type_str = clang_getTypeSpelling(arg_type);
      printf("%s -> ", yalce_name(lookups, clang_getCString(arg_type_str)));
      clang_disposeString(arg_type_str);
    }
  }

  result_type = clang_getResultType(type);
  CXString return_type = clang_getTypeSpelling(result_type);
  printf("%s", yalce_name(lookups, clang_getCString(return_type)));
  clang_disposeString(return_type);
}

void print_typedef_decl(CXCursor cursor, name_lookup *lookups) {
  CXType underlying_type = clang_getTypedefDeclUnderlyingType(cursor);
  CXString type_name = clang_getCursorSpelling(cursor);

  const char *cursor_name = clang_getCString(type_name);

  // Special case for CCCallback
  if (strcmp(cursor_name, "CCCallback") == 0) {
    printf("type CCCallback = (Double -> ());\n");
    clang_disposeString(type_name);
    return;
  }

  // fprintf(stderr, "name %s -> %s\n", cursor_name,
  //         yalce_name(lookups, cursor_name));
  printf("type %s = ", yalce_name(lookups, cursor_name));

  if (underlying_type.kind == CXType_Pointer) {
    CXType pointee_type = clang_getPointeeType(underlying_type);
    if (pointee_type.kind == CXType_FunctionProto) {
      print_function_type(pointee_type, lookups);
    } else {
      CXString type_spelling = clang_getTypeSpelling(underlying_type);
      printf("%s", yalce_name(lookups, clang_getCString(type_spelling)));
      clang_disposeString(type_spelling);
    }
  } else if (underlying_type.kind == CXType_FunctionProto) {
    print_function_type(underlying_type, lookups);
  } else {
    CXString type_spelling = clang_getTypeSpelling(underlying_type);
    printf("%s", yalce_name(lookups, clang_getCString(type_spelling)));
    clang_disposeString(type_spelling);
  }

  printf(";\n");
  clang_disposeString(type_name);
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

  // printf("%s RETURN TYPE %s\n", clang_getCString(func_name),
  //        clang_getCString(return_type));
  printf("%s", yalce_name(lookups, clang_getCString(return_type)));

  printf(";\n");
  // printf("# return type %s\n", clang_getCString(return_type));

  clang_disposeString(func_name);
  clang_disposeString(return_type);
}

void print_struct_decl(CXCursor cursor, name_lookup *lookups) {
  CXString struct_name = clang_getCursorSpelling(cursor);
  CXType struct_type = clang_getCursorType(cursor);
  // printf("STRUCT: %s\n", clang_getCString(struct_name));
}

void print_enum_decl(CXCursor cursor, name_lookup *lookups) {
  CXString struct_name = clang_getCursorSpelling(cursor);
  // CXType struct_type = clang_getCursorType(cursor);
  // printf("ENUM: %s\n", clang_getCString(struct_name));
}

enum CXChildVisitResult visitor(CXCursor cursor, CXCursor parent,
                                CXClientData client_data) {

  CXType underlying_type = clang_getTypedefDeclUnderlyingType(cursor);
  CXString type_name = clang_getCursorSpelling(cursor);
  const char *cursor_name = clang_getCString(type_name);
  name_lookup *lookups = client_data;

  // fprintf(stderr, "TYPE DECL: %s\n", cursor_name);
  if (clang_Location_isFromMainFile(clang_getCursorLocation(cursor)) == 0) {
    return CXChildVisit_Continue;
  }
  switch (clang_getCursorKind(cursor)) {
  case CXCursor_FunctionDecl: {

    print_function_decl(cursor, lookups);
    break;
  }
  case CXCursor_StructDecl: {

    print_struct_decl(cursor, lookups);
    break;
  }
  case CXCursor_EnumDecl: {

    print_enum_decl(cursor, lookups);
    break;
  }
  case CXCursor_TypedefDecl: {

    print_typedef_decl(cursor, lookups);
    break;
  }
  default: {
  }
  }

  return CXChildVisit_Recurse;
}
// Helper function to determine if a file is likely C++
int is_cpp_file(const char *filename) {
  const char *ext = strrchr(filename, '.');
  if (!ext)
    return 0;

  return (strcmp(ext, ".hpp") == 0 || strcmp(ext, ".hxx") == 0 ||
          strcmp(ext, ".h++") == 0 || strcmp(ext, ".cpp") == 0 ||
          strcmp(ext, ".cxx") == 0 || strcmp(ext, ".cc") == 0);
}

// Option 1: Accept lookups from a file
int main(int argc, char *argv[]) {
  name_lookup *lookups = NULL;

  // Default lookups
  lookups = lookups_extend(lookups, "double", "Double");
  lookups = lookups_extend(lookups, "void", "()");
  lookups = lookups_extend(lookups, "uint64_t", "Uint64");
  lookups = lookups_extend(lookups, "uint32_t", "Int");
  lookups = lookups_extend(lookups, "uint8_t", "Char");
  lookups = lookups_extend(lookups, "int", "Int");
  lookups = lookups_extend(lookups, "bool", "Bool");
  lookups = lookups_extend(lookups, "char", "Char");
  lookups = lookups_extend(lookups, "char *", "Ptr");
  lookups = lookups_extend(lookups, "const char *", "Ptr");
  lookups = lookups_extend(lookups, "void *", "Ptr");
  lookups = lookups_extend(lookups, "double *", "Ptr");

  // engine lib -specific lookups

  // Parse command line arguments
  int header_start = 1;

  // Check for -l flag for additional lookups file
  if (argc > 2 && strcmp(argv[1], "-l") == 0) {
    if (argc < 4) {
      fprintf(stderr, "Usage: %s [-l lookup_file] <header_files>\n", argv[0]);
      return 1;
    }

    // Load additional lookups from file
    FILE *lookup_file = fopen(argv[2], "r");
    if (lookup_file == NULL) {
      fprintf(stderr, "Error: Unable to open lookup file %s\n", argv[2]);
      return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), lookup_file)) {
      // Remove newline
      line[strcspn(line, "\n")] = 0;

      // Skip empty lines and comments
      if (line[0] == '\0' || line[0] == '#')
        continue;

      // Parse "from_type -> to_type" format
      char *arrow = strstr(line, " -> ");
      if (arrow) {
        *arrow = '\0';
        char *from_name = line;
        char *to_name = arrow + 4;

        // Trim whitespace
        while (*from_name == ' ')
          from_name++;
        while (*to_name == ' ')
          to_name++;
        // fprintf(stderr, "from name [%s] to name [%s]\n", from_name, to_name);

        lookups = lookups_extend(lookups, strdup(from_name), strdup(to_name));
      }
    }
    fclose(lookup_file);
    header_start = 3;
    // name_lookup *l = lookups;
    // fprintf(stderr, "LOOKUPS\n");
    //   while (l) {
    //     fprintf(stderr, "%s -> %s\n", l->key, l->val);
    //     l = l->next;
    //   }
  }

  // Process header files
  for (int i = header_start; i < argc; i++) {
    char *input_header = argv[i];
    int is_cpp = is_cpp_file(input_header);

    printf("# %s\n", input_header);
    CXIndex index = clang_createIndex(0, 0);

    CXTranslationUnit unit;
    if (is_cpp) {
      const char *clang_args[] = {"-xc++", "-std=c++11"};
      unit = clang_parseTranslationUnit(index, input_header, clang_args, 2,
                                        NULL, 0, CXTranslationUnit_None);
    } else {

      unit = clang_parseTranslationUnit(index, input_header, NULL, 0, NULL, 0,
                                        CXTranslationUnit_None);
    }

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
