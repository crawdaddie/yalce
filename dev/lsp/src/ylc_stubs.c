#include "common.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Stub implementations for YLC functions that we don't need for basic parsing

// Only declare external variables that are already defined in YLC object files
extern Ast *ast_root;
extern char *_cur_script;
extern const char *_cur_script_content;
extern char *__import_current_dir;
extern custom_binops_t *__custom_binops;
extern const char *__base_dir;

// Lexer variables are already defined in lex.yy.o and y.tab.o
extern int yylineno;
extern int yycolumn;
extern int yyprevcolumn;
extern long long int yyabsoluteoffset;
extern long long int yyprevoffset;
extern char *yytext;

// Location tracking for parser
typedef struct YYLTYPE {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
extern YYLTYPE yylloc;

// Config stub
typedef struct {
  int dummy;
} Config;

Config config = {0};

// Path utilities stubs
char *get_dirname(const char *path) {
  if (!path)
    return strdup(".");

  char *dir = strdup(path);
  char *last_slash = strrchr(dir, '/');
  if (last_slash) {
    *last_slash = '\0';
  } else {
    strcpy(dir, ".");
  }
  return dir;
}

char *normalize_path(const char *path) { return strdup(path ? path : "."); }

char *get_mod_name_from_path_identifier(const char *path) {
  if (!path)
    return strdup("");

  // Extract filename without extension
  const char *filename = strrchr(path, '/');
  filename = filename ? filename + 1 : path;

  char *mod_name = strdup(filename);
  char *dot = strrchr(mod_name, '.');
  if (dot)
    *dot = '\0';

  return mod_name;
}

char *read_script(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (!fp)
    return NULL;

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *content = malloc(size + 1);
  if (!content) {
    fclose(fp);
    return NULL;
  }

  fread(content, 1, size, fp);
  content[size] = '\0';
  fclose(fp);

  return content;
}

// yyerror is already defined in y.tab.o
extern void yyerror(const char *s);

// LSP-specific initialization function
void ylc_lsp_init() {
  // Initialize global variables that the parser needs
  ast_root = NULL;
  _cur_script = NULL;
  _cur_script_content = NULL;
  __import_current_dir = NULL;
  __custom_binops = NULL;

  // Reset lexer state
  yylineno = 1;
  // yycolumn = 1;
  // yyprevcolumn = 1;
  yyabsoluteoffset = 0;
  yyprevoffset = 0;
  yytext = NULL;

  // Initialize location tracking
  yylloc.first_line = 1;
  yylloc.first_column = 1;
  yylloc.last_line = 1;
  yylloc.last_column = 1;
}

// LSP-specific initialization function with directory
void ylc_lsp_init_with_dir(const char *base_dir) {
  ylc_lsp_init();
  set_base_dir(base_dir);
}
