# ylc.nvim

Neovim plugin for running `ylc <current-file> -i` in a side terminal and sending code snippets to that process over stdin.

## Features

- `:YlcOpen` starts a YLC REPL for the current file in a vertical split
- `:YlcOpenDebug` starts `ylc` under `lldb` and runs it automatically
- `<C-c><C-c>` in normal mode expands to the attached LSP `selectionRange` and sends it, with no fallback path
- `<C-c><C-c>` in visual mode sends the current selection
- blank lines are ignored in normal mode
- `:YlcRestart`, `:YlcStop`, `:YlcSelectNode`, `:YlcSendNode`, `:YlcSendParagraph`, `:YlcSendLine`, and `:YlcSendBuffer`

## Install

Example with `lazy.nvim`:

```lua
{
  dir = "/home/adam/projects/yalce/dev/nvim/ylc.nvim",
  config = function()
    require("ylc").setup()
  end,
}
```

That setup call now registers the local YLC Tree-sitter parser with `nvim-treesitter` automatically when available. After that, install it once with:

```vim
:TSInstall ylc
```

If you build a parser library at `dev/tree-sitter-ylc/parser/ylc.so`, the plugin will also load it directly on startup without `nvim-treesitter`.

## Notes

- The plugin assumes `ylc` is on your `$PATH`
- Set `vim.g.ylc_disable_default_keymaps = 1` before loading the plugin if you want to define your own mappings
- The plugin does not ship a syntax file; it assumes your existing YLC syntax/filetype setup
- Configure a custom command or process environment with `require("ylc").setup({ cmd = {...}, env = {...} })`
- Configure debugger launch with `require("ylc").setup({ debugger_cmd = { "lldb", "-o", "run", "--" } })`
- Tree-sitter send behavior can be configured with `require("ylc").setup({ treesitter = { enabled = true, runnable_node_types = {...} } })`
- Tree-sitter parser registration can be configured with `require("ylc").setup({ treesitter = { auto_register = true, parser_dir = "/path/to/tree-sitter-ylc", parser_files = { "src/parser.c" } } })`
- Direct parser loading can be configured with `require("ylc").setup({ treesitter = { parser_library = "/path/to/ylc.so" } })`
- Set `close_term_on_successful_exit = false` if you never want the terminal to close automatically
