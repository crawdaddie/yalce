#include <clang-c/Index.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Type mapping structure
typedef struct type_mapping {
  const char *c_type;
  const char *ylc_type;
  struct type_mapping *next;
} type_mapping;

// Global type mappings
static type_mapping *g_mappings = NULL;

// Add a type mapping
void add_mapping(const char *c_type, const char *ylc_type) {
  type_mapping *mapping = malloc(sizeof(type_mapping));
  mapping->c_type = strdup(c_type);
  mapping->ylc_type = strdup(ylc_type);
  mapping->next = g_mappings;
  g_mappings = mapping;
}

// Look up YLC type for C type
const char *get_ylc_type(const char *c_type) {
  for (type_mapping *m = g_mappings; m; m = m->next) {
    if (strcmp(m->c_type, c_type) == 0) {
      return m->ylc_type;
    }
  }
  return c_type; // Return original if no mapping found
}

// Initialize all type mappings
void init_type_mappings(void) {
  // Basic types
  add_mapping("void", "()");
  add_mapping("int", "Int");
  add_mapping("bool", "Bool");
  add_mapping("char", "Char");
  add_mapping("double", "Double");

  // Integer types (literal forms as they appear in source)
  add_mapping("uint64_t", "Uint64");
  add_mapping("unsigned long long", "Uint64");
  add_mapping("long long", "Int64");
  add_mapping("uint32_t", "Int");
  add_mapping("unsigned int", "Int");
  add_mapping("unsigned long", "Int");
  add_mapping("uint8_t", "Char");
  add_mapping("unsigned char", "Char");

  // Pointer types
  add_mapping("int *", "Ptr");
  add_mapping("bool *", "Ptr");
  add_mapping("char *", "Ptr");
  add_mapping("const char *", "Ptr");
  add_mapping("void *", "Ptr");
  add_mapping("void*", "Ptr"); // Without space
  add_mapping("double *", "Ptr");

  // Library-specific types
  add_mapping("AudioGraph *", "Ptr");
  add_mapping("SDL_Renderer *", "Ptr");
  add_mapping("SDL_Color", "(Int, Int, Int, Int)");
  add_mapping("struct __color", "(Int, Int, Int, Int)");
  add_mapping("MIDIEndpointRef", "Int");
  add_mapping("ItemCount", "Int");
  add_mapping("_ArrBool", "Array of Bool");
  add_mapping("_String", "String");

  // Engine-specific types
  add_mapping("SignalRef", "Ptr");
  add_mapping("NodeRef", "Synth");
}

// Print tokens for a given cursor range
void print_tokens_for_cursor(CXCursor cursor, CXTranslationUnit unit) {
  CXSourceRange range = clang_getCursorExtent(cursor);

  CXToken *tokens = NULL;
  unsigned num_tokens = 0;

  // Tokenize the range
  clang_tokenize(unit, range, &tokens, &num_tokens);

  printf("  Tokens (%u): ", num_tokens);
  for (unsigned i = 0; i < num_tokens; i++) {
    CXString token_spelling = clang_getTokenSpelling(unit, tokens[i]);
    const char *token_str = clang_getCString(token_spelling);

    CXTokenKind token_kind = clang_getTokenKind(tokens[i]);
    const char *kind_str = "";
    switch (token_kind) {
    case CXToken_Punctuation:
      kind_str = "PUNCT";
      break;
    case CXToken_Keyword:
      kind_str = "KEYWORD";
      break;
    case CXToken_Identifier:
      kind_str = "ID";
      break;
    case CXToken_Literal:
      kind_str = "LITERAL";
      break;
    case CXToken_Comment:
      kind_str = "COMMENT";
      break;
    }

    printf("[%s:%s] ", kind_str, token_str);
    clang_disposeString(token_spelling);
  }
  printf("\n");

  // Clean up tokens
  clang_disposeTokens(unit, tokens, num_tokens);
}

// Extract parameter information from function declaration
void process_function_parameters(CXCursor func_cursor, CXTranslationUnit unit) {
  printf("  Parameters:\n");

  // Visit function parameters
  CXCursor param_cursor = clang_Cursor_getArgument(func_cursor, 0);
  unsigned num_args = clang_Cursor_getNumArguments(func_cursor);

  for (unsigned i = 0; i < num_args; i++) {
    CXCursor param = clang_Cursor_getArgument(func_cursor, i);
    CXString param_name = clang_getCursorSpelling(param);
    CXType param_type = clang_getCursorType(param);
    CXString param_type_spelling = clang_getTypeSpelling(param_type);

    const char *param_name_str = clang_getCString(param_name);
    const char *param_type_str = clang_getCString(param_type_spelling);

    printf("    [%u] %s %s -> %s\n", i, param_type_str,
           param_name_str ? param_name_str : "(unnamed)",
           get_ylc_type(param_type_str));

    // Print tokens for this parameter
    printf("      ");
    print_tokens_for_cursor(param, unit);

    clang_disposeString(param_name);
    clang_disposeString(param_type_spelling);
  }
}

// Process typedef declaration
void process_typedef(CXCursor cursor, CXTranslationUnit unit) {
  CXString cursor_spelling = clang_getCursorSpelling(cursor);
  const char *typedef_name = clang_getCString(cursor_spelling);

  CXType underlying_type = clang_getTypedefDeclUnderlyingType(cursor);
  CXString type_spelling = clang_getTypeSpelling(underlying_type);
  const char *underlying_type_str = clang_getCString(type_spelling);

  printf("typedef %s %s -> %s\n", underlying_type_str, typedef_name,
         get_ylc_type(underlying_type_str));

  // Print all tokens in the typedef
  print_tokens_for_cursor(cursor, unit);

  clang_disposeString(cursor_spelling);
  clang_disposeString(type_spelling);
}

// Process function declaration
void process_function(CXCursor cursor, CXTranslationUnit unit) {
  CXString cursor_spelling = clang_getCursorSpelling(cursor);
  const char *func_name = clang_getCString(cursor_spelling);

  CXType func_type = clang_getCursorType(cursor);
  CXType result_type = clang_getResultType(func_type);
  CXString result_type_spelling = clang_getTypeSpelling(result_type);
  const char *return_type_str = clang_getCString(result_type_spelling);

  printf("function %s() -> %s\n", func_name, get_ylc_type(return_type_str));

  // Print all tokens in the function declaration
  print_tokens_for_cursor(cursor, unit);

  // Process parameters
  process_function_parameters(cursor, unit);

  clang_disposeString(cursor_spelling);
  clang_disposeString(result_type_spelling);
}

// Advanced token analysis - get tokens with their locations
void analyze_tokens_detailed(CXCursor cursor, CXTranslationUnit unit) {
  CXSourceRange range = clang_getCursorExtent(cursor);

  CXToken *tokens = NULL;
  unsigned num_tokens = 0;

  clang_tokenize(unit, range, &tokens, &num_tokens);

  printf("  Detailed token analysis:\n");
  for (unsigned i = 0; i < num_tokens; i++) {
    CXString token_spelling = clang_getTokenSpelling(unit, tokens[i]);
    const char *token_str = clang_getCString(token_spelling);

    CXTokenKind token_kind = clang_getTokenKind(tokens[i]);
    CXSourceLocation location = clang_getTokenLocation(unit, tokens[i]);

    // Get file, line, column info
    CXFile file;
    unsigned line, column, offset;
    clang_getExpansionLocation(location, &file, &line, &column, &offset);

    printf("    Token[%u]: '%s' (kind:%d) at line %u, col %u\n", i, token_str,
           token_kind, line, column);

    clang_disposeString(token_spelling);
  }

  clang_disposeTokens(unit, tokens, num_tokens);
}

// Visitor function for AST traversal
enum CXChildVisitResult visit_cursor(CXCursor cursor, CXCursor parent,
                                     CXClientData client_data) {
  CXTranslationUnit unit = (CXTranslationUnit)client_data;

  enum CXCursorKind kind = clang_getCursorKind(cursor);
  CXString cursor_spelling = clang_getCursorSpelling(cursor);
  const char *cursor_name = clang_getCString(cursor_spelling);

  switch (kind) {
  case CXCursor_FunctionDecl:
    fprintf(stderr, "Processing function: %s\n", cursor_name);
    process_function(cursor, unit);
    printf("\n");
    break;

  case CXCursor_TypedefDecl:
    fprintf(stderr, "Processing typedef: %s\n", cursor_name);
    process_typedef(cursor, unit);
    printf("\n");
    break;

  case CXCursor_StructDecl:
  case CXCursor_EnumDecl:
    // Skip for now, but you could add token analysis here too
    break;

  default:
    break;
  }

  clang_disposeString(cursor_spelling);
  return CXChildVisit_Recurse;
}

void tokenize_entire_file(const char *filename, CXTranslationUnit unit) {
  CXFile file = clang_getFile(unit, filename);
  if (!file) {
    fprintf(stderr, "Could not get file handle for %s\n", filename);
    return;
  }

  CXSourceLocation start = clang_getLocationForOffset(unit, file, 0);
  CXSourceLocation end = clang_getLocationForOffset(unit, file, UINT_MAX);
  CXSourceRange range = clang_getRange(start, end);

  CXToken *tokens = NULL;
  unsigned num_tokens = 0;

  clang_tokenize(unit, range, &tokens, &num_tokens);

  printf("# All tokens in file (%u total):\n", num_tokens);
  for (unsigned i = 0; i < num_tokens; i++) {
    CXString token_spelling = clang_getTokenSpelling(unit, tokens[i]);
    const char *token_str = clang_getCString(token_spelling);

    CXTokenKind token_kind = clang_getTokenKind(tokens[i]);
    printf("Token[%u]: '%s' (kind:%d)\n", i, token_str, token_kind);

    clang_disposeString(token_spelling);
  }

  clang_disposeTokens(unit, tokens, num_tokens);
}

// Main parsing function
int parse_header(const char *filename) {
  CXIndex index = clang_createIndex(0, 0);

  // Use minimal arguments to avoid include issues
  const char *args[] = {"-std=c99"};

  CXTranslationUnit unit = clang_parseTranslationUnit(
      index, filename, args, 1, NULL, 0,
      CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_SingleFileParse);

  if (!unit) {
    // Try with no arguments
    unit = clang_parseTranslationUnit(index, filename, NULL, 0, NULL, 0,
                                      CXTranslationUnit_SkipFunctionBodies |
                                          CXTranslationUnit_SingleFileParse);
  }

  if (!unit) {
    fprintf(stderr, "Error: Failed to parse %s\n", filename);
    clang_disposeIndex(index);
    return 1;
  }

  // Check for parsing errors but continue anyway
  unsigned num_diagnostics = clang_getNumDiagnostics(unit);
  if (num_diagnostics > 0) {
    fprintf(stderr, "Note: %u diagnostic(s) while parsing %s\n",
            num_diagnostics, filename);
  }

  printf("# Generated bindings for %s\n\n", filename);

  // Optional: tokenize entire file first to see all tokens
  // tokenize_entire_file(filename, unit);
  // printf("\n");

  // Visit all cursors in the translation unit
  CXCursor root_cursor = clang_getTranslationUnitCursor(unit);
  clang_visitChildren(root_cursor, visit_cursor, unit);

  clang_disposeTranslationUnit(unit);
  clang_disposeIndex(index);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <header_file> [header_file...]\n", argv[0]);
    return 1;
  }

  // Initialize type mappings
  init_type_mappings();

  // Process each header file
  for (int i = 1; i < argc; i++) {
    if (parse_header(argv[i]) != 0) {
      return 1;
    }
    if (i < argc - 1) {
      printf("\n");
    }
  }

  return 0;
}
