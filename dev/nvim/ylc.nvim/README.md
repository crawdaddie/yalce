# ylc.nvim

Neovim plugin for running `ylc <current-file> -i` in a side terminal and sending code snippets to that process over stdin.

## Features

- `:YlcOpen` starts a YLC REPL for the current file in a vertical split
- `:YlcOpen` starts notebook mode automatically when the current buffer ends in `.ylcnb`
- `:YlcOpenKitty` starts YLC in an external Kitty window and sends snippets via Kitty remote control
- `:YlcOpenDebug` starts `ylc` under `lldb`, runs it automatically, and uses the same input send path as normal mode
- `:YlcReload` restarts the plugin-managed YLC job if one is open, otherwise starts one
- `<C-CR>` / `<D-CR>` in normal mode sends the current notebook cell, or expands to the attached LSP `selectionRange`
- `<C-CR>` / `<D-CR>` in visual mode sends the current selection
- `gd` runs deduped YLC go-to-definition
- `<leader>yo` runs `:YlcReload`
- blank lines are ignored in normal mode
- `:YlcRestart`, `:YlcStop`, `:YlcDefinition`, `:YlcSelectNode`, `:YlcSendNode`, `:YlcSendParagraph`, `:YlcSendLine`, and `:YlcSendBuffer`

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
- Configure debugger launch with `require("ylc").setup({ debugger_cmd = { "lldb" } })`
- Set `close_term_on_successful_exit = false` if you never want the terminal to close automatically

## External Kitty Terminal

For terminal graphics such as `gnuplot`'s `kittycairo` terminal, Neovim's built-in
terminal buffer is not enough because it is a libvterm emulator. Use an external
Kitty window instead:

```vim
:YlcOpenKitty
```

This launches Kitty with remote control enabled for that window and sends code
with `kitty @ send-text`. To make notebook buffers use this backend by default:

```lua
require("ylc").setup({
  notebook_terminal_backend = "kitty",
})
```

To make all YLC sessions use external Kitty:

```lua
require("ylc").setup({
  terminal_backend = "kitty",
})
```

The Kitty backend can be customized:

```lua
require("ylc").setup({
  kitty = {
    cmd = { "kitty" },
    title = "ylc.nvim",
    socket = nil,
    startup_delay_ms = 250,
    extra_args = {},
  },
})
```
