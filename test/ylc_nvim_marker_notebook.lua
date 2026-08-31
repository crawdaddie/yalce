local root = assert(os.getenv("YLC_REPO_ROOT"))

package.path = root .. "/dev/nvim/ylc.nvim/lua/?.lua;" ..
               root .. "/dev/nvim/ylc.nvim/lua/?/init.lua;" .. package.path

local sent = {}
local cmd_seen = nil

vim.notify = function() end
vim.fn.termopen = function(cmd)
	cmd_seen = cmd
	return 42
end
vim.fn.jobwait = function()
	return { -1 }
end
vim.fn.chansend = function(_, text)
	sent[#sent + 1] = text
	return 1
end

vim.cmd("edit /tmp/ylc_nvim_marker.ylc")
vim.api.nvim_buf_set_lines(0, 0, -1, false, {
	"let boot = 1;",
	"",
	"#%%",
	"let stop = 2;",
})

local ylc = require("ylc")
ylc.setup({
	cmd = { "ylc-test" },
	lsp = { enabled = false },
})
ylc.open()

vim.wait(1000, function()
	return #sent > 0
end, 10)

assert(vim.deep_equal(cmd_seen, { "ylc-test", "-i" }), vim.inspect(cmd_seen))
assert(sent[1] == "let boot = 1;\n\n", vim.inspect(sent))
assert(not sent[1]:find("#%%", 1, false), vim.inspect(sent))
assert(not sent[1]:find("let stop", 1, false), vim.inspect(sent))

vim.api.nvim_win_set_cursor(0, { 4, 0 })
ylc.send_selection_or_current_chunk()

assert(sent[2] == "let stop = 2;\n\n", vim.inspect(sent))
