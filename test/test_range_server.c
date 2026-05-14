#include "../lang/parse.h"
#include "modules.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylineno;

static void reset_parser_state(void) {
  yylineno = 1;
  yyrestart(NULL);
  pctx.ast_root = NULL;
}

static bool assert_range(const char *name, const char *src, int line,
                         const char *expected_text, int expected_start_line,
                         int expected_start_col) {
  Ast *root = parse_input_buffer(name, src);
  source_range range;
  bool ok = root && find_top_level_range_at_line(root, src, line, &range);
  bool pass = true;

  if (!ok) {
    printf("FAIL %s line %d: no range found\n", name, line);
    pass = false;
  } else {
    size_t expected_len = strlen(expected_text);
    size_t actual_len = (size_t)(range.end_offset - range.start_offset);
    const char *actual_text = src + range.start_offset;

    if (range.start_line != expected_start_line ||
        range.start_col != expected_start_col) {
      printf("FAIL %s line %d: expected start %d:%d got %d:%d\n", name, line,
             expected_start_line, expected_start_col, range.start_line,
             range.start_col);
      pass = false;
    }

    if (actual_len != expected_len ||
        strncmp(actual_text, expected_text, expected_len) != 0) {
      printf("FAIL %s line %d: unexpected text\n", name, line);
      printf("expected:\n%s\n", expected_text);
      printf("actual:\n%.*s\n", (int)actual_len, actual_text);
      pass = false;
    }
  }

  free(root);
  reset_parser_state();
  if (pass) {
    printf("PASS %s line %d\n", name, line);
  }
  return pass;
}

int main(void) {
  bool status = true;
  init_module_registry();

  const char *simple_src = "let x = 1;\nlet y = 2";
  status &= assert_range("simple-second-let", simple_src, 2, "let y = 2", 2, 1);

  const char *comment_src = "let x = 1; # keep this comment\nlet y = 2";
  status &=
      assert_range("comment-after-stmt", comment_src, 2, "let y = 2", 2, 1);

  const char *app_src = "play_pattern q (\n"
                        "\n"
                        "  let kickp =  \n"
                        "    \"3 3 3 4 3\" |> pat\n"
                        "    |> cor_map ((*) (q * 0.25))\n"
                        "    |> cor_map (fn d -> Kick.trig (); d)\n"
                        "  ;\n"
                        "\n"
                        "  let snp = (\n"
                        "    let snrout = (fn () ->\n"
                        "      yield 3.;\n"
                        "      Snare.trig ();\n"
                        "      yield 1.;\n"
                        "      yield 3.;\n"
                        "      Snare.trig ();\n"
                        "      yield 0.75;\n"
                        "      Snare.trig ();\n"
                        "      yield 0.25;\n"
                        "      yield snrout ()\n"
                        "    ) in\n"
                        "    snrout () |> cor_map ((*) q)\n"
                        "  )\n"
                        "\n"
                        ")";
  status &= assert_range("top-level-application", app_src, 18, app_src, 1, 1);
  status &= assert_range("top-level-application-blank-line", app_src, 2,
                         app_src, 1, 1);

  const char *module_src = "let Kick = module () ->\n"
                           "  let buf = 1;\n"
                           "  let trig = fn () ->\n"
                           "    1\n"
                           "  ;;\n"
                           ";\n"
                           "let y = 2";
  const char *module_expected = "let Kick = module () ->\n"
                                "  let buf = 1;\n"
                                "  let trig = fn () ->\n"
                                "    1\n"
                                "  ;;\n"
                                ";\n";
  status &= assert_range("inline-module", module_src, 4, module_expected, 1, 1);
  status &= assert_range("stmt-after-inline-module", module_src, 7, "let y = 2",
                         7, 1);

  return status ? 0 : 1;
}
