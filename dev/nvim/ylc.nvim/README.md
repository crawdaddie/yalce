# ylc.nvim

Neovim plugin for running `ylc <current-file> -i` in a side terminal and sending code snippets to that process over stdin.

## Features

- `:YlcOpen` starts a YLC REPL for the current file in a vertical split
- `:YlcOpen` starts notebook mode automatically when the current buffer ends in `.ylcnb`
- `:YlcOpenDebug` starts `ylc` under `lldb` and runs it automatically
- `:YlcReload` restarts the plugin-managed YLC job if one is open, otherwise starts one
- `<C-c><C-c>` in normal mode expands to the attached LSP `selectionRange` and sends it, with no fallback path
- `<C-c><C-c>` in visual mode sends the current selection
- `<leader>yo` runs `:YlcReload`
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

## Notes

- The plugin assumes `ylc` is on your `$PATH`
- The plugin starts `ylc_lsp_server` automatically for `ylc` buffers and prefers the local binary at `build/tools/ylc_lsp_server`
- `.ylc` and `.ylcnb` buffers are both detected as `ylc` filetype
- Set `vim.g.ylc_disable_default_keymaps = 1` before loading the plugin if you want to define your own mappings
- The plugin does not ship a syntax file; it assumes your existing YLC syntax/filetype setup
- Configure a custom command or process environment with `require("ylc").setup({ cmd = {...}, env = {...} })`
- Configure debugger launch with `require("ylc").setup({ debugger_cmd = { "lldb", "-o", "run", "--" } })`
- Set `close_term_on_successful_exit = false` if you never want the terminal to close automatically
