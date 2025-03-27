#include "./common.h"
#include "serde.h"
void _print_location(Ast *ast, FILE *fstream) {
  loc_info *loc = ast->loc_info;

  if (!loc || !loc->src || !loc->src_content) {
    print_ast_err(ast);
    return;
  }

  fprintf(fstream, " %s %d:%d\n", loc->src, loc->line, loc->col);

  const char *start = loc->src_content;
  const char *offset = start + loc->absolute_offset;

  while (offset > start && *offset != '\n') {
    offset--;
  }

  if (offset > start) {
    offset++;
  }

  while (*offset && *offset != '\n') {
    fputc(*offset, fstream);
    offset++;
  }

  fprintf(fstream, "\n");
  fprintf(fstream, "%*c", loc->col - 1, ' ');
  fprintf(fstream, "^");
  fprintf(fstream, "\n");
}
