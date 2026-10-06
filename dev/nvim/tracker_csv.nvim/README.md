# tracker_csv.nvim

Minimal semicolon-delimited tracker formatting for Neovim.

Add the directory to `runtimepath`:

```lua
vim.opt.rtp:append('/path/to/tracker_csv.nvim')
```

`*.csv` buffers use the `tracker_csv` filetype and align on entry and before
writing. Empty and trailing fields remain present. Lines beginning with `#`
(optionally after whitespace) are preserved as comments. Fields use the
`field ;field` style, with one space before each delimiter. Semicolons inside
parentheses remain part of the cell and are formatted as `item ;item` without
column padding. Columns have alternating background stripes; comments are not
striped. Tracker tokens have separate syntax groups for instruments, MIDI
notes, command IDs, and command values.

Commands:

```vim
:TrackerAlign
:TrackerUnalign
:TrackerToggle
:TrackerMoveCellUp
:TrackerMoveCellDown
```

Use `:TrackerAlign!` to force re-alignment.

`Alt-k` and `Alt-j` move the cell under the cursor up and down without moving
the other columns.
