local M = {}

local config = {
	cmd = { "ylc" },
	interactive_flag = "-i",
	terminal_backend = "nvim",
	notebook_terminal_backend = nil,
	open_cmd = "botright vsplit",
	close_term_on_successful_exit = true,
	env = {},
	kitty = {
		cmd = { "kitty" },
		title = "ylc.nvim",
		socket = nil,
		startup_delay_ms = 250,
		extra_args = {},
	},
	debugger_cmd = { "lldb" },
	lsp = {
		enabled = true,
		name = "ylc_lsp",
		cmd = nil,
		root_markers = { ".git" },
	},
}

local state = {
	backend = nil,
	job_id = nil,
	term_buf = nil,
	term_win = nil,
	script_path = nil,
	debug_active = false,
	kitty_socket = nil,
	kitty_title = nil,
	kitty_window_id = nil,
}

local autocmd_group = vim.api.nvim_create_augroup("ylc.nvim", { clear = true })

local function tbl_copy(value)
	return vim.deepcopy(value)
end

local function module_dir()
	local source = debug.getinfo(1, "S").source
	if vim.startswith(source, "@") then
		source = source:sub(2)
	end
	return vim.fs.dirname(source)
end

local function repo_root_dir()
	return vim.fs.normalize(vim.fs.joinpath(module_dir(), "..", "..", "..", "..", ".."))
end

local function is_blank(line)
	return line == nil or line:match("^%s*$") ~= nil
end

local function is_cell_marker(line)
	if not line then
		return false
	end

	local trimmed = vim.trim(line)
	return trimmed == "#%%" or trimmed == "# %%"
end

local function notify(msg, level)
	vim.notify(msg, level or vim.log.levels.INFO, { title = "ylc.nvim" })
end

local function escape_text(text)
	local lines = vim.split(text, "\n", { plain = true })

	if #lines <= 1 then
		return text .. "\n\n"
	end

	local escaped = {}
	for i = 1, #lines - 1 do
		local line = lines[i]
		if line:match("%S") then
			escaped[#escaped + 1] = line .. " \\"
		end
	end

	local last = lines[#lines]
	if last and last:match("%S") then
		escaped[#escaped + 1] = last
	end

	return table.concat(escaped, "\n") .. "\n\n"
end

local function reset_state()
	state.backend = nil
	state.job_id = nil
	state.term_buf = nil
	state.term_win = nil
	state.script_path = nil
	state.debug_active = false
	state.kitty_socket = nil
	state.kitty_title = nil
	state.kitty_window_id = nil
end

local function kitty_cmd()
	return tbl_copy(config.kitty.cmd or { "kitty" })
end

local function kitty_executable()
	local cmd = config.kitty.cmd or { "kitty" }
	return cmd[1] or "kitty"
end

local function kitty_socket_path()
	if config.kitty.socket and config.kitty.socket ~= "" then
		return config.kitty.socket
	end
	return string.format("/tmp/ylc-nvim-%d.sock", vim.fn.getpid())
end

local function kitty_title()
	return string.format("%s-%d", config.kitty.title or "ylc.nvim", vim.fn.getpid())
end

local function kitty_target()
	return "unix:" .. state.kitty_socket
end

local function kitty_remote(args, stdin)
	if not state.kitty_socket then
		return false, "Kitty socket is not initialized"
	end

	local cmd = kitty_cmd()
	cmd[#cmd + 1] = "@"
	cmd[#cmd + 1] = "--to"
	cmd[#cmd + 1] = kitty_target()
	for _, arg in ipairs(args) do
		cmd[#cmd + 1] = arg
	end

	local output = vim.fn.system(cmd, stdin or "")
	return vim.v.shell_error == 0, output
end

local function decode_json(text)
	if vim.json and vim.json.decode then
		local ok, decoded = pcall(vim.json.decode, text)
		if ok then
			return decoded
		end
	end

	local ok, decoded = pcall(vim.fn.json_decode, text)
	if ok then
		return decoded
	end

	return nil
end

local function first_kitty_window_id(tree)
	if type(tree) ~= "table" then
		return nil
	end

	for _, os_window in ipairs(tree) do
		for _, tab in ipairs(os_window.tabs or {}) do
			for _, window in ipairs(tab.windows or {}) do
				if window.id ~= nil then
					return tostring(window.id)
				end
			end
		end
	end

	return nil
end

local function record_kitty_window_id(output)
	local id = first_kitty_window_id(decode_json(output))
	if not id then
		return false
	end

	state.kitty_window_id = id
	return true
end

local function is_kitty_running()
	if state.backend ~= "kitty" or not state.kitty_socket then
		return false
	end
	local ok, output = kitty_remote({ "ls" })
	if ok and not record_kitty_window_id(output) then
		state.kitty_window_id = nil
	end
	return ok
end

local function is_job_running()
	if state.backend == "kitty" then
		return is_kitty_running()
	end
	return state.job_id and vim.fn.jobwait({ state.job_id }, 0)[1] == -1
end

local function current_file_path()
	local path = vim.fn.expand("%:p")
	if path == nil or path == "" then
		return nil
	end
	return path
end

local function is_notebook_path(path)
	return type(path) == "string" and path:sub(-6) == ".ylcnb"
end

local function current_buffer_is_notebook()
	return is_notebook_path(current_file_path())
end

local function current_env()
	if vim.tbl_isempty(config.env) then
		return nil
	end

	local env = vim.empty_dict()
	for key, value in pairs(config.env) do
		env[key] = value
	end
	return env
end

local function default_lsp_cmd()
	local local_server = vim.fs.normalize(vim.fs.joinpath(repo_root_dir(), "build", "tools", "ylc_lsp_server"))
	local stat = (vim.uv or vim.loop).fs_stat(local_server)
	if stat then
		return { local_server }
	end

	return { "ylc_lsp_server" }
end

local function lsp_root_dir(bufnr)
	local path = vim.api.nvim_buf_get_name(bufnr)
	if not path or path == "" then
		return vim.loop.cwd()
	end

	local root = vim.fs.root(path, config.lsp.root_markers or {})
	if root then
		return root
	end

	return vim.fs.dirname(path)
end

local function lsp_cmd_executable(cmd)
	if type(cmd) == "string" then
		return cmd
	end
	if type(cmd) == "table" then
		return cmd[1]
	end
	return nil
end

local function path_basename(path)
	if not path or path == "" then
		return nil
	end
	if vim.fs.basename then
		return vim.fs.basename(path)
	end
	return path:match("([^/\\]+)$") or path
end

local function normalize_lsp_root(root)
	if type(root) ~= "string" or root == "" then
		return nil
	end
	return vim.fs.normalize(root)
end

local function lsp_roots_match(a, b)
	local left = normalize_lsp_root(a)
	local right = normalize_lsp_root(b)
	return left ~= nil and right ~= nil and left == right
end

local function lsp_cmds_match(a, b)
	local left = lsp_cmd_executable(a)
	local right = lsp_cmd_executable(b)
	if not left or not right then
		return false
	end

	if vim.fn.exepath(left) ~= "" then
		left = vim.fn.exepath(left)
	end
	if vim.fn.exepath(right) ~= "" then
		right = vim.fn.exepath(right)
	end

	left = vim.fs.normalize(left)
	right = vim.fs.normalize(right)
	return left == right or path_basename(left) == path_basename(right)
end

local function is_ylc_lsp_client(client, conf)
	if not client then
		return false
	end

	if client.name == conf.name then
		return true
	end

	local client_cmd = client.config and client.config.cmd
	return lsp_cmds_match(client_cmd, conf.cmd) or path_basename(lsp_cmd_executable(client_cmd)) == "ylc_lsp_server"
end

local function ylc_lsp_client_reusable(client, conf)
	if not is_ylc_lsp_client(client, conf) then
		return false
	end

	local client_root = client.config and client.config.root_dir
	return lsp_roots_match(client_root, conf.root_dir)
end

local function ylc_lsp_clients_for_root(conf)
	local clients = {}
	for _, client in ipairs(vim.lsp.get_clients()) do
		if ylc_lsp_client_reusable(client, conf) then
			clients[#clients + 1] = client
		end
	end

	table.sort(clients, function(a, b)
		return (a.id or 0) < (b.id or 0)
	end)
	return clients
end

local function attach_lsp_client(bufnr, client)
	if not (client and client.id and vim.api.nvim_buf_is_valid(bufnr)) then
		return
	end
	if not vim.lsp.buf_is_attached(bufnr, client.id) then
		pcall(vim.lsp.buf_attach_client, bufnr, client.id)
	end
end

local function detach_lsp_client(bufnr, client)
	if not (client and client.id and vim.api.nvim_buf_is_valid(bufnr)) then
		return
	end
	if vim.lsp.buf_is_attached(bufnr, client.id) then
		pcall(vim.lsp.buf_detach_client, bufnr, client.id)
	end
end

local function lsp_client_buffers(client)
	if not (client and client.id) then
		return {}
	end

	local buffers = {}
	for _, bufnr in ipairs(vim.api.nvim_list_bufs()) do
		if vim.api.nvim_buf_is_valid(bufnr) and vim.lsp.buf_is_attached(bufnr, client.id) then
			buffers[#buffers + 1] = bufnr
		end
	end
	return buffers
end

local function stop_lsp_client(client)
	if client and client.stop then
		pcall(function()
			client:stop()
		end)
	end
end

local function consolidate_ylc_lsp_clients(bufnr, conf)
	local clients = ylc_lsp_clients_for_root(conf)
	local primary = clients[1]

	if not primary then
		return nil
	end

	attach_lsp_client(bufnr, primary)

	for i = 2, #clients do
		local duplicate = clients[i]
		for _, duplicate_bufnr in ipairs(lsp_client_buffers(duplicate)) do
			attach_lsp_client(duplicate_bufnr, primary)
			detach_lsp_client(duplicate_bufnr, duplicate)
		end
		stop_lsp_client(duplicate)
	end

	return primary
end

local function ensure_lsp(bufnr)
	if not config.lsp.enabled or vim.bo[bufnr].filetype ~= "ylc" then
		return
	end

	local cmd = config.lsp.cmd or default_lsp_cmd()
	if type(cmd) == "string" then
		cmd = { cmd }
	end

	local lsp_config = {
		name = config.lsp.name,
		cmd = cmd,
		root_dir = lsp_root_dir(bufnr),
	}

	if consolidate_ylc_lsp_clients(bufnr, lsp_config) then
		return
	end

	vim.lsp.start(lsp_config, {
		bufnr = bufnr,
		reuse_client = function(client, conf)
			return ylc_lsp_client_reusable(client, conf)
		end,
	})

	vim.schedule(function()
		if vim.api.nvim_buf_is_valid(bufnr) then
			consolidate_ylc_lsp_clients(bufnr, lsp_config)
		end
	end)
end

local function definition_location_range(location)
	if not location then
		return nil
	end
	return location.range or location.targetSelectionRange or location.targetRange
end

local function definition_location_uri(location)
	if not location then
		return nil
	end
	return location.uri or location.targetUri
end

local function definition_location_key(location)
	local uri = definition_location_uri(location)
	local range = definition_location_range(location)
	if not (uri and range and range.start and range["end"]) then
		return nil
	end

	return table.concat({
		uri,
		range.start.line,
		range.start.character,
		range["end"].line,
		range["end"].character,
	}, ":")
end

local function add_definition_location(locations, seen, location)
	local key = definition_location_key(location)
	if key and not seen[key] then
		seen[key] = true
		locations[#locations + 1] = location
	end
end

local function add_definition_result(locations, seen, result)
	if not result then
		return
	end

	if definition_location_uri(result) then
		add_definition_location(locations, seen, result)
		return
	end

	for _, location in ipairs(result) do
		add_definition_location(locations, seen, location)
	end
end

local function current_buffer_text(bufnr)
	return table.concat(vim.api.nvim_buf_get_lines(bufnr, 0, -1, false), "\n")
end

local function current_buffer_lines(bufnr)
	return vim.api.nvim_buf_get_lines(bufnr, 0, -1, false)
end

local function buffer_has_notebook_markers(bufnr)
	for _, line in ipairs(current_buffer_lines(bufnr)) do
		if is_cell_marker(line) then
			return true
		end
	end
	return false
end

local function get_notebook_prelude_text(bufnr)
	local lines = current_buffer_lines(bufnr)
	local end_idx = #lines

	for i, line in ipairs(lines) do
		if is_cell_marker(line) then
			end_idx = i - 1
			break
		end
	end

	while end_idx > 0 and is_blank(lines[end_idx]) do
		end_idx = end_idx - 1
	end

	if end_idx <= 0 then
		return ""
	end

	local selected = {}
	for i = 1, end_idx do
		selected[#selected + 1] = lines[i]
	end
	return table.concat(selected, "\n")
end

local function find_current_notebook_cell_range(bufnr)
	local lines = current_buffer_lines(bufnr)
	if #lines == 0 then
		return nil
	end

	local cursor_row = vim.api.nvim_win_get_cursor(0)[1]
	if not buffer_has_notebook_markers(bufnr) then
		return nil
	end

	local marker_row = nil
	if is_cell_marker(lines[cursor_row]) then
		marker_row = cursor_row
	end

	local start_line = 1
	if marker_row then
		start_line = marker_row + 1
	else
		for row = cursor_row, 1, -1 do
			if is_cell_marker(lines[row]) then
				start_line = row + 1
				break
			end
		end
	end

	local search_from = marker_row and (marker_row + 1) or (cursor_row + 1)
	local end_line = #lines
	for row = search_from, #lines do
		if is_cell_marker(lines[row]) then
			end_line = row - 1
			break
		end
	end

	while start_line <= end_line and is_blank(lines[start_line]) do
		start_line = start_line + 1
	end

	while end_line >= start_line and is_blank(lines[end_line]) do
		end_line = end_line - 1
	end

	if start_line > end_line then
		return nil
	end

	return {
		start_row = start_line - 1,
		end_row = end_line,
	}
end

local function get_text_for_notebook_cell(bufnr)
	local range = find_current_notebook_cell_range(bufnr)
	if not range then
		return nil
	end

	local lines = vim.api.nvim_buf_get_lines(bufnr, range.start_row, range.end_row, false)
	if not lines or #lines == 0 then
		return nil
	end

	return table.concat(lines, "\n"), range
end

local function select_notebook_cell_range(range)
	local start_line = range.start_row + 1
	local end_line = range.end_row
	local end_text = vim.api.nvim_buf_get_lines(0, end_line - 1, end_line, false)[1] or ""
	local end_col = math.max(#end_text, 1)

	vim.fn.setpos("'<", { 0, start_line, 1, 0 })
	vim.fn.setpos("'>", { 0, end_line, end_col, 0 })
	vim.api.nvim_win_set_cursor(0, { start_line, 0 })
	vim.cmd("normal! gv")
end

local function get_text_for_range(bufnr, start_row, start_col, end_row, end_col)
	local lines = vim.api.nvim_buf_get_text(bufnr, start_row, start_col, end_row, end_col, {})
	if not lines or #lines == 0 then
		return ""
	end
	return table.concat(lines, "\n")
end

local function get_selection_range_from_lsp(bufnr)
	local clients = vim.lsp.get_clients({ bufnr = bufnr })
	if not clients or vim.tbl_isempty(clients) then
		return nil, "no lsp client attached"
	end

	local supports_selection_range = false
	for _, client in ipairs(clients) do
		if client:supports_method("textDocument/selectionRange") then
			supports_selection_range = true
			break
		end
	end

	if not supports_selection_range then
		return nil, "no attached lsp client supports selectionRange"
	end

	local cursor = vim.api.nvim_win_get_cursor(0)
	local params = {
		textDocument = vim.lsp.util.make_text_document_params(bufnr),
		positions = {
			{
				line = cursor[1] - 1,
				character = cursor[2],
			},
		},
	}

	local responses = vim.lsp.buf_request_sync(bufnr, "textDocument/selectionRange", params, 1000)
	if not responses then
		return nil, "selectionRange request timed out"
	end

	for _, response in pairs(responses) do
		local result = response.result
		if result and result[1] and result[1].range then
			return result[1].range
		end
	end

	return nil, "selectionRange returned no range"
end

local function get_text_from_lsp_selection_range(bufnr)
	local range = get_selection_range_from_lsp(bufnr)
	if not range then
		return nil
	end

	return get_text_for_range(bufnr, range.start.line, range.start.character, range["end"].line, range["end"].character)
end

local function range_end_to_visual_pos(bufnr, range)
	local end_line = range["end"].line
	local end_char = range["end"].character

	if end_char > 0 then
		return end_line + 1, end_char
	end

	if end_line > range.start.line then
		local prev_line = vim.api.nvim_buf_get_lines(bufnr, end_line - 1, end_line, false)[1] or ""
		return end_line, math.max(#prev_line, 1)
	end

	return end_line + 1, 1
end

local function select_range(bufnr, range)
	local start_line = range.start.line + 1
	local start_col = range.start.character + 1
	local end_line, end_col = range_end_to_visual_pos(bufnr, range)

	vim.fn.setpos("'<", { 0, start_line, start_col, 0 })
	vim.fn.setpos("'>", { 0, end_line, end_col, 0 })
	vim.api.nvim_win_set_cursor(0, { start_line, range.start.character })
	vim.cmd("normal! gv")
end

local function current_line_is_blank()
	return is_blank(vim.api.nvim_get_current_line())
end

local function focus_terminal_end()
	if not (state.term_buf and vim.api.nvim_buf_is_valid(state.term_buf)) then
		return
	end
	if not (state.term_win and vim.api.nvim_win_is_valid(state.term_win)) then
		return
	end

	vim.schedule(function()
		if vim.api.nvim_win_is_valid(state.term_win) and vim.api.nvim_buf_is_valid(state.term_buf) then
			local last_line = vim.api.nvim_buf_line_count(state.term_buf)
			vim.api.nvim_win_set_cursor(state.term_win, { math.max(last_line, 1), 0 })
		end
	end)
end

local function send_to_job(text)
	if state.backend == "kitty" then
		if not is_kitty_running() then
			return false
		end

		local args = { "send-text" }
		if state.kitty_window_id then
			args[#args + 1] = "--match"
			args[#args + 1] = "id:" .. state.kitty_window_id
		end
		args[#args + 1] = "--stdin"

		local ok, output = kitty_remote(args, escape_text(text))
		if not ok then
			notify("Failed to send text to Kitty: " .. vim.trim(output or ""), vim.log.levels.ERROR)
			return false
		end
		return true
	end

	if not is_job_running() then
		return false
	end

	vim.fn.chansend(state.job_id, escape_text(text))
	return true
end

local function open_window()
	local old_term_buf = state.term_buf

	if state.term_win and vim.api.nvim_win_is_valid(state.term_win) then
		vim.api.nvim_set_current_win(state.term_win)
	else
		vim.cmd(config.open_cmd)
		state.term_win = vim.api.nvim_get_current_win()
	end

	state.term_buf = vim.api.nvim_create_buf(false, true)
	vim.api.nvim_win_set_buf(state.term_win, state.term_buf)
	if old_term_buf and vim.api.nvim_buf_is_valid(old_term_buf) then
		pcall(vim.api.nvim_buf_delete, old_term_buf, { force = true })
	end

	vim.bo[state.term_buf].bufhidden = "hide"
	vim.wo[state.term_win].number = false
	vim.wo[state.term_win].relativenumber = false
end

local function build_cmd(script_path)
	local cmd = tbl_copy(config.cmd)
	cmd[#cmd + 1] = script_path
	cmd[#cmd + 1] = config.interactive_flag
	return cmd
end

local function build_notebook_cmd()
	return tbl_copy(config.cmd)
end

local function build_debug_cmd(script_path, notebook)
	local cmd = tbl_copy(config.debugger_cmd)
	local target = notebook and build_notebook_cmd() or build_cmd(script_path)
	cmd[#cmd + 1] = "-o"
	cmd[#cmd + 1] = "run"
	cmd[#cmd + 1] = "--"
	for _, arg in ipairs(target) do
		cmd[#cmd + 1] = arg
	end
	return cmd
end

local function backend_for_opts(opts)
	if opts.terminal_backend then
		return opts.terminal_backend
	end
	if opts.notebook and config.notebook_terminal_backend then
		return config.notebook_terminal_backend
	end
	return config.terminal_backend or "nvim"
end

local function build_kitty_launch_cmd(term_cmd)
	local cmd = kitty_cmd()
	cmd[#cmd + 1] = "--title"
	cmd[#cmd + 1] = state.kitty_title
	cmd[#cmd + 1] = "--listen-on"
	cmd[#cmd + 1] = kitty_target()
	cmd[#cmd + 1] = "--override"
	cmd[#cmd + 1] = "allow_remote_control=yes"

	for _, arg in ipairs(config.kitty.extra_args or {}) do
		cmd[#cmd + 1] = arg
	end

	for _, arg in ipairs(term_cmd) do
		cmd[#cmd + 1] = arg
	end

	return cmd
end

local function wait_for_kitty()
	local deadline = vim.loop.hrtime() + ((config.kitty.startup_delay_ms or 250) * 1000000)
	while vim.loop.hrtime() < deadline do
		if is_kitty_running() then
			return true
		end
		vim.wait(25)
	end
	return is_kitty_running()
end

local function open_kitty(term_cmd)
	if vim.fn.executable(kitty_executable()) == 0 then
		notify("Kitty executable not found: " .. kitty_executable(), vim.log.levels.ERROR)
		return false
	end

	state.backend = "kitty"
	state.kitty_socket = kitty_socket_path()
	state.kitty_title = kitty_title()

	pcall(vim.fn.delete, state.kitty_socket)

	local job_id = vim.fn.jobstart(build_kitty_launch_cmd(term_cmd), {
		env = current_env(),
		detach = true,
	})
	if job_id <= 0 then
		notify("Failed to start external Kitty YLC terminal", vim.log.levels.ERROR)
		reset_state()
		return false
	end

	if not wait_for_kitty() then
		notify("Started Kitty but remote control is not ready yet", vim.log.levels.WARN)
	end

	return true
end

function M.stop()
	if state.backend == "kitty" then
		if is_kitty_running() then
			local args = { "close-window" }
			if state.kitty_window_id then
				args[#args + 1] = "--match"
				args[#args + 1] = "id:" .. state.kitty_window_id
			end
			kitty_remote(args)
		end
		reset_state()
		return
	end

	if is_job_running() then
		vim.fn.jobstop(state.job_id)
	end
	if state.term_win and vim.api.nvim_win_is_valid(state.term_win) then
		vim.api.nvim_win_close(state.term_win, true)
	end
	reset_state()
end

function M.open(opts)
	opts = opts or {}

	local script_path = opts.script_path or current_file_path()
	if not script_path then
		notify("Current buffer has no file on disk", vim.log.levels.ERROR)
		return
	end

	if not opts.debug and not opts.raw_cmd and not opts.notebook and is_notebook_path(script_path) then
		M.open_notebook(opts)
		return
	end

	if is_job_running() then
		M.stop()
	end

	if opts.debug then
		state.debug_active = true
	end

	state.script_path = script_path
	local term_cmd
	if opts.raw_cmd then
		term_cmd = tbl_copy(opts.raw_cmd)
	elseif opts.notebook then
		term_cmd = opts.debug and build_debug_cmd(script_path, true) or build_notebook_cmd()
	else
		term_cmd = opts.debug and build_debug_cmd(script_path, false) or build_cmd(script_path)
	end

	local backend = backend_for_opts(opts)
	if backend == "kitty" then
		if open_kitty(term_cmd) then
			if opts.debug then
				notify("Started external Kitty YLC debugger for " .. script_path)
			elseif opts.notebook then
				notify("Started external Kitty YLC notebook for " .. script_path)
			else
				notify("Started external Kitty YLC for " .. script_path)
			end
		end
		return
	elseif backend ~= "nvim" then
		notify("Unknown YLC terminal backend: " .. tostring(backend), vim.log.levels.ERROR)
		reset_state()
		return
	end

	local origin_win = vim.api.nvim_get_current_win()
	open_window()
	state.backend = "nvim"

	state.job_id = vim.fn.termopen(term_cmd, {
		env = current_env(),
		on_stdout = function()
			focus_terminal_end()
		end,
		on_exit = function(_, code)
			vim.schedule(function()
				local exited_script = state.script_path
				local should_close = config.close_term_on_successful_exit and code == 0
				if should_close then
					if state.term_win and vim.api.nvim_win_is_valid(state.term_win) then
						vim.api.nvim_win_close(state.term_win, true)
					end
					reset_state()
					return
				end

				state.job_id = nil
				notify(
					string.format("YLC terminal exited with code %d for %s", code, exited_script or "<unknown>"),
					vim.log.levels.WARN
				)
			end)
		end,
	})

	vim.api.nvim_set_current_win(origin_win)
	if opts.debug then
		notify("Started YLC under lldb for " .. script_path)
	elseif opts.notebook then
		notify("Started YLC notebook for " .. script_path)
	else
		notify("Started YLC for " .. script_path)
	end
end

function M.open_notebook(opts)
	opts = opts or {}
	local bufnr = vim.api.nvim_get_current_buf()
	local script_path = current_file_path()
	if not script_path then
		notify("Current buffer has no file on disk", vim.log.levels.ERROR)
		return
	end

	local prelude = get_notebook_prelude_text(bufnr)

	M.open({
		script_path = script_path,
		notebook = true,
		debug = opts.debug,
		terminal_backend = opts.terminal_backend,
	})

	if prelude ~= "" then
		vim.defer_fn(function()
			send_to_job(prelude)
		end, 50)
	end
end

function M.open_debug()
	if current_buffer_is_notebook() then
		M.open_notebook({ debug = true })
		return
	end

	M.open({ debug = true })
end

function M.open_kitty(opts)
	opts = opts or {}
	opts.terminal_backend = "kitty"
	M.open(opts)
end

function M.restart()
	local terminal_backend = state.backend
	if current_buffer_is_notebook() then
		M.open_notebook({ terminal_backend = terminal_backend })
		return
	end

	M.open({ terminal_backend = terminal_backend })
end

function M.reload_or_open()
	if is_job_running() and state.script_path then
		M.restart()
		return
	end

	M.open()
end

function M.ensure_open()
	local script_path = current_file_path()
	if not script_path then
		notify("Current buffer has no file on disk", vim.log.levels.ERROR)
		return false
	end

	if is_job_running() and state.script_path == script_path then
		return true
	end

	if current_buffer_is_notebook() then
		M.open_notebook()
		return is_job_running()
	end

	M.open({ script_path = script_path })
	return is_job_running()
end

function M.send(text)
	if not text or text == "" then
		notify("Nothing to send", vim.log.levels.WARN)
		return
	end

	if not M.ensure_open() then
		notify("YLC process is not running", vim.log.levels.ERROR)
		return
	end

	send_to_job(text)
end

function M.definition()
	local bufnr = vim.api.nvim_get_current_buf()
	ensure_lsp(bufnr)

	local params = vim.lsp.util.make_position_params(0, "utf-16")
	vim.lsp.buf_request_all(bufnr, "textDocument/definition", params, function(responses)
		local locations = {}
		local seen = {}

		for _, response in pairs(responses or {}) do
			add_definition_result(locations, seen, response.result)
		end

		if #locations == 0 then
			notify("No definition found", vim.log.levels.INFO)
			return
		end

		if #locations == 1 then
			vim.lsp.util.jump_to_location(locations[1], "utf-16", true)
			return
		end

		local items = vim.lsp.util.locations_to_items(locations, "utf-16")
		vim.fn.setqflist({}, " ", {
			title = "YLC definitions",
			items = items,
		})
		vim.cmd("copen")
	end)
end

function M.send_buffer()
	M.send(table.concat(vim.api.nvim_buf_get_lines(0, 0, -1, false), "\n"))
end

function M.send_current_line()
	M.send(vim.api.nvim_get_current_line())
end

function M.send_current_node()
	local bufnr = vim.api.nvim_get_current_buf()
	if current_line_is_blank() then
		return
	end

	local text, err = get_text_from_lsp_selection_range(bufnr)
	if not text or text == "" then
		notify((err or "No LSP selection range available") .. ", sending current line", vim.log.levels.WARN)
		M.send_current_line()
		return
	end

	M.send(text)
end

function M.send_current_notebook_cell()
	local bufnr = vim.api.nvim_get_current_buf()
	if current_line_is_blank() then
		return
	end

	local text = get_text_for_notebook_cell(bufnr)
	if not text or text == "" then
		notify("No notebook cell found at cursor", vim.log.levels.WARN)
		return
	end

	M.send(text)
end

function M.select_current_node()
	local bufnr = vim.api.nvim_get_current_buf()
	if current_line_is_blank() then
		return
	end

	local range, err = get_selection_range_from_lsp(bufnr)
	if not range then
		notify((err or "No LSP selection range available") .. ", sending current line", vim.log.levels.WARN)
		M.send_current_line()
		return
	end

	select_range(bufnr, range)
end

function M.select_and_send_current_node()
	local bufnr = vim.api.nvim_get_current_buf()
	if current_line_is_blank() then
		return
	end

	local range, err = get_selection_range_from_lsp(bufnr)
	if not range then
		notify(err or "No LSP selection range available", vim.log.levels.WARN)
		return
	end

	select_range(bufnr, range)

	local text =
		get_text_for_range(bufnr, range.start.line, range.start.character, range["end"].line, range["end"].character)
	if not text or text == "" then
		notify("LSP selection range returned empty text, sending current line", vim.log.levels.WARN)
		M.send_current_line()
		return
	end

	M.send(text)
end

function M.select_and_send_current_chunk()
	local bufnr = vim.api.nvim_get_current_buf()
	if current_line_is_blank() then
		return
	end

	local text, range = get_text_for_notebook_cell(bufnr)
	if text and range then
		select_notebook_cell_range(range)
		M.send(text)
		return
	end

	M.select_and_send_current_node()
end

function M.send_selection_or_current_chunk()
	local mode = vim.fn.mode()
	if current_buffer_is_notebook() and (mode == "v" or mode == "V" or mode == "\22") then
		M.send_visual_selection()
		return
	end

	if mode == "v" or mode == "V" or mode == "\22" then
		M.send_visual_selection()
		return
	end

	M.select_and_send_current_chunk()
end

function M.send_current_paragraph()
	local bufnr = vim.api.nvim_get_current_buf()
	local line_count = vim.api.nvim_buf_line_count(bufnr)
	local row = vim.api.nvim_win_get_cursor(0)[1]

	local start_row = row
	while start_row > 1 and not is_blank(vim.fn.getline(start_row - 1)) do
		start_row = start_row - 1
	end

	local end_row = row
	while end_row < line_count and not is_blank(vim.fn.getline(end_row + 1)) do
		end_row = end_row + 1
	end

	local lines = vim.api.nvim_buf_get_lines(bufnr, start_row - 1, end_row, false)
	M.send(table.concat(lines, "\n"))
end

function M.send_visual_selection()
	local start_pos = vim.fn.getpos("'<")
	local end_pos = vim.fn.getpos("'>")
	local start_row = start_pos[2]
	local start_col = start_pos[3]
	local end_row = end_pos[2]
	local end_col = end_pos[3]
	local visual_mode = vim.fn.visualmode()

	if start_row == 0 or end_row == 0 then
		notify("No visual selection available", vim.log.levels.WARN)
		return
	end

	if start_row > end_row or (start_row == end_row and start_col > end_col) then
		start_row, end_row = end_row, start_row
		start_col, end_col = end_col, start_col
	end

	local lines = vim.api.nvim_buf_get_lines(0, start_row - 1, end_row, false)
	if #lines == 0 then
		notify("Nothing selected", vim.log.levels.WARN)
		return
	end

	if visual_mode == "V" then
		M.send(table.concat(lines, "\n"))
		return
	end

	if visual_mode == "\22" then
		notify("Blockwise visual send is not supported", vim.log.levels.WARN)
		return
	end

	lines[1] = string.sub(lines[1], math.max(start_col, 1))
	lines[#lines] = string.sub(lines[#lines], 1, math.max(end_col, 1))

	M.send(table.concat(lines, "\n"))
end

function M.setup(opts)
	config = vim.tbl_deep_extend("force", config, opts or {})

	vim.api.nvim_create_autocmd({ "FileType" }, {
		group = autocmd_group,
		pattern = "ylc",
		callback = function(args)
			ensure_lsp(args.buf)
		end,
	})

	vim.api.nvim_create_autocmd({ "LspAttach" }, {
		group = autocmd_group,
		callback = function(args)
			if vim.bo[args.buf].filetype ~= "ylc" then
				return
			end
			vim.schedule(function()
				if vim.api.nvim_buf_is_valid(args.buf) then
					ensure_lsp(args.buf)
				end
			end)
		end,
	})

	for _, bufnr in ipairs(vim.api.nvim_list_bufs()) do
		if vim.api.nvim_buf_is_loaded(bufnr) and vim.bo[bufnr].filetype == "ylc" then
			ensure_lsp(bufnr)
		end
	end
end

return M
