if vim.g.ylc_disable_default_keymaps then
	return
end

local opts = { buffer = true, silent = true }

vim.keymap.set("n", "<C-CR>", function()
	require("ylc").send_selection_or_current_chunk()
end, opts)

vim.keymap.set("n", "<leader>yo", function()
	require("ylc").reload_or_open()
end, vim.tbl_extend("force", opts, { desc = "YLC reload/open" }))

vim.keymap.set("x", "<C-CR>", function()
	require("ylc").send_selection_or_current_chunk()
end, opts)
