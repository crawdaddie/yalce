if vim.g.ylc_disable_default_keymaps then
  return
end

local opts = { buffer = true, silent = true }

vim.keymap.set("n", "<C-c><C-c>", function()
  require("ylc").select_and_send_current_chunk()
end, opts)

vim.keymap.set("n", "<leader>yo", function()
  require("ylc").reload_or_open()
end, vim.tbl_extend("force", opts, { desc = "YLC reload/open" }))

vim.keymap.set("x", "<C-c><C-c>", function()
  require("ylc").send_visual_selection()
end, opts)
