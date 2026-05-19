local ylc = require("ylc")

local ylc_augroup = vim.api.nvim_create_augroup("ylc.nvim", { clear = true })

vim.api.nvim_create_autocmd({ "BufRead", "BufNewFile" }, {
  group = ylc_augroup,
  pattern = "*.ylc",
  callback = function(args)
    vim.bo[args.buf].filetype = "ylc"
  end,
})

vim.api.nvim_create_user_command("YlcOpen", function()
  ylc.open()
end, {})

vim.api.nvim_create_user_command("YlcOpenNb", function()
  ylc.open_notebook()
end, {})

vim.api.nvim_create_user_command("YlcOpenDebug", function()
  ylc.open_debug()
end, {})

vim.api.nvim_create_user_command("YlcRestart", function()
  ylc.restart()
end, {})

vim.api.nvim_create_user_command("YlcReload", function()
  ylc.reload_or_open()
end, {})

vim.api.nvim_create_user_command("YlcStop", function()
  ylc.stop()
end, {})

vim.api.nvim_create_user_command("YlcSendParagraph", function()
  ylc.send_current_paragraph()
end, {})

vim.api.nvim_create_user_command("YlcSendLine", function()
	ylc.send_current_line()
end, {})

vim.api.nvim_create_user_command("YlcSendNode", function()
	ylc.send_current_node()
end, {})

vim.api.nvim_create_user_command("YlcSelectNode", function()
	ylc.select_current_node()
end, {})

vim.api.nvim_create_user_command("YlcDebugTreesitter", function()
	ylc.debug_treesitter()
end, {})

vim.api.nvim_create_user_command("YlcSendBuffer", function()
	ylc.send_buffer()
end, {})
