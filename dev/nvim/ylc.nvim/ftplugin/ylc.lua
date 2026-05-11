if vim.g.ylc_disable_default_keymaps then
  return
end

local opts = { buffer = true, silent = true }

vim.keymap.set("n", "<C-c><C-c>", function()
  require("ylc").send_current_node()
end, opts)

vim.keymap.set("x", "<C-c><C-c>", function()
  require("ylc").send_visual_selection()
end, opts)
