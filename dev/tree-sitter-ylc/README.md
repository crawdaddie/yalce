# tree-sitter-ylc

Initial Tree-sitter grammar for YLC, derived from the existing lexer in [lang/lex.l](/home/adam/projects/yalce/lang/lex.l) and parser in [lang/parser.y](/home/adam/projects/yalce/lang/parser.y).

## Goal

This grammar is aimed first at editor structure:

- locating enclosing expressions for send-to-REPL actions
- supporting syntax-aware selections
- enabling highlighting and future textobjects in Neovim

It is not yet intended as a complete replacement for the Bison parser.

## Covered in this first pass

- semicolon-separated expression sequences
- `let`, `import`, `open`, `type`
- `fn` and `module` lambdas
- application, `|>`, `@@`, arithmetic, comparisons, `::`, `..`, `:=`
- tuples, lists, arrays, field access, indexing, slicing
- `match ... with` and `if ... then ... else`
- basic type expressions and function signatures
- backtick and triple-backtick format strings with `{...}` interpolation

## Known gaps

- lexer parity is incomplete, especially around some string edge cases
- operator precedence should still be validated against the Bison parser
- custom operator declarations and some destructuring/type edge cases need more refinement
- no external scanner yet; format strings are handled with pure grammar rules for now
- blockwise semantic distinctions used by the compiler AST are not represented here

## Suggested next steps

1. Run `tree-sitter generate` in this directory and fix any conflicts the generator reports.
2. Add a corpus under `test/corpus/` using representative YLC snippets from the repo.
3. In Neovim, use the enclosing `expression` node for `<C-c><C-c>` send behavior.
4. Add queries for textobjects once the grammar stabilizes.

## Neovim use

For your send-to-REPL case, the useful node classes will likely be:

- `expression`
- `statement_sequence`
- `application_expression`
- `let_binding`
- `match_expression`
- `if_expression`

The usual shape is:

```lua
local node = vim.treesitter.get_node()
while node and node:type() ~= "expression" do
  node = node:parent()
end
```

In practice you may want to climb to a larger enclosing named node such as `let_binding` or `statement_sequence` depending on what should count as a runnable chunk.
