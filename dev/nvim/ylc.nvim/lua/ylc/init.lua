local M = {}

local config = {
	cmd = { "ylc" },
	interactive_flag = "-i",
	open_cmd = "botright vsplit",
	close_term_on_successful_exit = true,
	env = {},
	debugger_cmd = { "lldb" },
	range_server = {
		enabled = true,
		cmd = nil,
		timeout_ms = 1000,
	},
	treesitter = {
		enabled = true,
		auto_register = true,
		language = "ylc",
		filetype = "ylc",
		parser_dir = nil,
		parser_library = nil,
		parser_files = { "src/parser.c" },
		runnable_node_types = {
			"let_binding",
			"match_expression",
			"if_expression",
			"for_expression",
			"type_declaration",
			"lambda_expression",
			"parenthesized_lambda",
			"parenthesized_let_lambda",
			"application_expression",
			"assignment_expression",
			"binary_expression",
			"yield_expression",
			"await_expression",
			"thunk_expression",
			"field_expression",
			"subscript_expression",
			"slice_expression",
			"list",
			"array",
			"tuple",
			"parenthesized_expression",
			"statement_sequence",
		},
	},
}

local state = {
	job_id = nil,
	term_buf = nil,
	term_win = nil,
	script_path = nil,
	treesitter_loaded = false,
	treesitter_load_error = nil,
	range_server_job_id = nil,
	range_server_stdout = "",
	range_server_next_id = 1,
	range_server_pending = {},
	debug_active = false,
	debug_fifo_path = nil,
	debug_fifo_handle = nil,
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

local function is_blank(line)
	return line == nil or line:match("^%s*$") ~= nil
end

local function set_from_list(items)
	local result = {}
	for _, item in ipairs(items) do
		result[item] = true
	end
	return result
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
	state.job_id = nil
	state.term_buf = nil
	state.term_win = nil
	state.script_path = nil
	state.debug_active = false
	if state.debug_fifo_handle then
		state.debug_fifo_handle:close()
		state.debug_fifo_handle = nil
	end
	if state.debug_fifo_path then
		pcall(vim.fn.delete, state.debug_fifo_path)
		state.debug_fifo_path = nil
	end
end

local function is_job_running()
	return state.job_id and vim.fn.jobwait({ state.job_id }, 0)[1] == -1
end

local function current_file_path()
	local path = vim.fn.expand("%:p")
	if path == nil or path == "" then
		return nil
	end
	return path
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

local function default_range_server_cmd()
	return { vim.fs.normalize(vim.fs.joinpath(module_dir(), "..", "..", "..", "..", "build", "tools", "ylc_range_server")) }
end

local function split_complete_lines(chunk)
	local lines = {}
	local start = 1

	while true do
		local nl = chunk:find("\n", start, true)
		if not nl then
			break
		end
		lines[#lines + 1] = chunk:sub(start, nl - 1)
		start = nl + 1
	end

	return lines, chunk:sub(start)
end

local function handle_range_server_line(line)
	if line == "" then
		return
	end

	local ok, decoded = pcall(vim.json.decode, line)
	if not ok or type(decoded) ~= "table" then
		return
	end

	local pending = state.range_server_pending[decoded.id]
	if pending then
		pending.response = decoded
		pending.done = true
	end
end

local function ensure_range_server()
	if not config.range_server.enabled then
		return false, "disabled"
	end

	if state.range_server_job_id and vim.fn.jobwait({ state.range_server_job_id }, 0)[1] == -1 then
		return true
	end

	local cmd = config.range_server.cmd or default_range_server_cmd()
	if type(cmd) == "string" then
		cmd = { cmd }
	end

	local exe = cmd[1]
	local stat = exe and (vim.uv or vim.loop).fs_stat(exe)
	if not stat then
		return false, "missing range server: " .. tostring(exe)
	end

	state.range_server_stdout = ""
	state.range_server_pending = {}
	state.range_server_job_id = vim.fn.jobstart(cmd, {
		stdout_buffered = false,
		on_stdout = function(_, data)
			if not data then
				return
			end

			local chunk = table.concat(data, "\n")
			if chunk == "" then
				return
			end

			state.range_server_stdout = state.range_server_stdout .. chunk
			local lines
			lines, state.range_server_stdout = split_complete_lines(state.range_server_stdout)
			for _, line in ipairs(lines) do
				handle_range_server_line(line)
			end
		end,
		on_exit = function()
			state.range_server_job_id = nil
		end,
	})

	return state.range_server_job_id > 0, "failed to start range server"
end

local function range_server_request(method, payload)
	local ok, reason = ensure_range_server()
	if not ok then
		return nil, reason
	end

	local id = state.range_server_next_id
	state.range_server_next_id = state.range_server_next_id + 1

	local request = vim.tbl_extend("force", payload or {}, {
		id = id,
		method = method,
	})

	state.range_server_pending[id] = { done = false, response = nil }
	vim.fn.chansend(state.range_server_job_id, vim.json.encode(request) .. "\n")

	local timeout_ms = config.range_server.timeout_ms
	local completed = vim.wait(timeout_ms, function()
		local pending = state.range_server_pending[id]
		return pending and pending.done
	end, 10)

	local pending = state.range_server_pending[id]
	state.range_server_pending[id] = nil

	if not completed or not pending or not pending.response then
		return nil, "range server timeout"
	end

	if pending.response.ok == false then
		return nil, pending.response.error or "range server error"
	end

	return pending.response
end

local function current_buffer_text(bufnr)
	return table.concat(vim.api.nvim_buf_get_lines(bufnr, 0, -1, false), "\n")
end

local function get_text_from_offsets(bufnr, start_offset, end_offset)
	local text = current_buffer_text(bufnr)
	return text:sub(start_offset + 1, end_offset)
end

local function get_text_from_range_server(bufnr)
	local path = current_file_path()
	if not path then
		return nil
	end

	local text = current_buffer_text(bufnr)
	local cursor = vim.api.nvim_win_get_cursor(0)
	local _, update_err = range_server_request("update", {
		path = path,
		text = text,
	})
	if update_err then
		return nil
	end

	local response, range_err = range_server_request("top_level_at", {
		path = path,
		line = cursor[1],
		column = cursor[2],
	})
	if range_err or not response then
		return nil
	end

	if not response.start_offset or not response.end_offset then
		return nil
	end

	return get_text_from_offsets(bufnr, response.start_offset, response.end_offset)
end

function M.sync_current_buffer_to_range_server()
	local bufnr = vim.api.nvim_get_current_buf()
	local path = current_file_path()
	if not path or not config.range_server.enabled then
		return false
	end

	local text = current_buffer_text(bufnr)
	local _, err = range_server_request("update", {
		path = path,
		text = text,
	})

	return err == nil
end

local function default_treesitter_parser_dir()
	return vim.fs.normalize(vim.fs.joinpath(module_dir(), "..", "..", "..", "..", "tree-sitter-ylc"))
end

local function default_treesitter_parser_library()
	return vim.fs.normalize(vim.fs.joinpath(default_treesitter_parser_dir(), "parser", "ylc.so"))
end

local function register_treesitter_parser()
	if not config.treesitter.enabled or not config.treesitter.auto_register then
		return
	end

	local ok, parsers = pcall(require, "nvim-treesitter.parsers")
	if not ok then
		return
	end

	local parser_configs = parsers.get_parser_configs()
	local language = config.treesitter.language
	local filetype = config.treesitter.filetype
	local parser_dir = config.treesitter.parser_dir or default_treesitter_parser_dir()
	local parser_files = config.treesitter.parser_files

	parser_configs[language] = vim.tbl_deep_extend("force", parser_configs[language] or {}, {
		install_info = {
			url = parser_dir,
			files = parser_files,
		},
		filetype = filetype,
	})

	if vim.treesitter and vim.treesitter.language and vim.treesitter.language.register then
		pcall(vim.treesitter.language.register, language, filetype)
	end
end

local function load_treesitter_parser()
	state.treesitter_loaded = false
	state.treesitter_load_error = nil

	if not config.treesitter.enabled then
		state.treesitter_load_error = "disabled"
		return
	end

	if not (vim.treesitter and vim.treesitter.language) then
		state.treesitter_load_error = "vim.treesitter.language unavailable"
		return
	end

	local language = config.treesitter.language
	local filetype = config.treesitter.filetype
	local parser_library = config.treesitter.parser_library or default_treesitter_parser_library()
	local stat = (vim.uv or vim.loop).fs_stat(parser_library)

	if not stat then
		state.treesitter_load_error = "missing parser library: " .. parser_library
		return
	end

	if not vim.treesitter.language.add then
		state.treesitter_load_error = "vim.treesitter.language.add unavailable"
		return
	end

	local ok, err = pcall(vim.treesitter.language.add, language, { path = parser_library })
	if not ok then
		state.treesitter_load_error = err
		return
	end

	state.treesitter_loaded = true
	if vim.treesitter.language.register then
		pcall(vim.treesitter.language.register, language, filetype)
	end
end

local function treesitter_status(bufnr)
	if not config.treesitter.enabled then
		return false, "disabled"
	end

	if not state.treesitter_loaded then
		return false, state.treesitter_load_error or "parser not loaded"
	end

	local ok = pcall(vim.treesitter.get_parser, bufnr, config.treesitter.language)
	if not ok then
		return false, "parser not active in current buffer"
	end

	return true, "ok"
end

local function get_node_text(node, bufnr)
	if not node then
		return nil
	end

	return vim.treesitter.get_node_text(node, bufnr)
end

local function get_text_for_range(bufnr, start_row, start_col, end_row, end_col)
	local lines = vim.api.nvim_buf_get_text(bufnr, start_row, start_col, end_row, end_col, {})
	if not lines or #lines == 0 then
		return ""
	end
	return table.concat(lines, "\n")
end

local function get_form_text_from_node_start(node, bufnr)
	if not node then
		return nil
	end

	local start_row, start_col = node:start()
	local line_count = vim.api.nvim_buf_line_count(bufnr)
	local depth = 0
	local in_string = false
	local string_delim = nil
	local escaping = false
	local end_row = nil
	local end_col = nil

	for row = start_row, line_count - 1 do
		local line = vim.api.nvim_buf_get_lines(bufnr, row, row + 1, false)[1] or ""
		local col_start = 1
		if row == start_row then
			col_start = start_col + 1
		end

		local col = col_start
		while col <= #line do
			local ch = line:sub(col, col)

			if not in_string and ch == "#" then
				break
			end

			if in_string then
				if escaping then
					escaping = false
				elseif ch == "\\" then
					escaping = true
				elseif ch == string_delim then
					in_string = false
					string_delim = nil
				end
			else
				if ch == '"' or ch == "'" or ch == "`" then
					in_string = true
					string_delim = ch
				elseif ch == "(" or ch == "[" or ch == "{" then
					depth = depth + 1
				elseif ch == ")" or ch == "]" or ch == "}" then
					depth = math.max(depth - 1, 0)
				elseif ch == ";" and depth == 0 then
					end_row = row
					end_col = col
					break
				end
			end

			col = col + 1
		end

		if end_row ~= nil then
			break
		end
	end

	if end_row == nil then
		local last_line = vim.api.nvim_buf_get_lines(bufnr, line_count - 1, line_count, false)[1] or ""
		return get_text_for_range(bufnr, start_row, start_col, line_count - 1, #last_line)
	end

	return get_text_for_range(bufnr, start_row, start_col, end_row, end_col)
end

local function current_line_is_blank()
	return is_blank(vim.api.nvim_get_current_line())
end

local function find_runnable_treesitter_node(bufnr)
	if not config.treesitter.enabled then
		return nil
	end

	local ok, parser = pcall(vim.treesitter.get_parser, bufnr, config.treesitter.language)
	if not ok or not parser then
		return nil
	end

	local trees = parser:parse()
	if not trees or not trees[1] then
		return nil
	end

	local root = trees[1]:root()
	if not root then
		return nil
	end

	local cursor = vim.api.nvim_win_get_cursor(0)
	local row = cursor[1] - 1
	local col = cursor[2]
	local node = root:named_descendant_for_range(row, col, row, col)
	if not node then
		return nil
	end

	local runnable_types = set_from_list(config.treesitter.runnable_node_types)
	local candidate = nil

	while node do
		local node_type = node:type()
		local parent = node:parent()

		if node_type == "statement_sequence" and parent and parent:type() == "source_file" then
			return candidate
		end

		if runnable_types[node_type] and node_type ~= "statement_sequence" then
			candidate = node
		end

		if parent and parent:type() == "source_file" then
			return candidate
		end

		node = parent
	end

	return candidate
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

local function open_window()
	vim.cmd(config.open_cmd)
	state.term_win = vim.api.nvim_get_current_win()
	state.term_buf = vim.api.nvim_create_buf(false, true)
	vim.api.nvim_win_set_buf(state.term_win, state.term_buf)
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

local function build_debug_cmd(script_path)
	local cmd = tbl_copy(config.debugger_cmd)
	local target = build_cmd(script_path)
	cmd[#cmd + 1] = "-o"
	cmd[#cmd + 1] = "process launch -i " .. state.debug_fifo_path
	cmd[#cmd + 1] = "--"
	for _, arg in ipairs(target) do
		cmd[#cmd + 1] = arg
	end
	return cmd
end

local function make_debug_fifo()
	local fifo_path = string.format("/tmp/ylc-debug-%d-%d.fifo", vim.fn.getpid(), vim.loop.hrtime())
	vim.fn.system({ "mkfifo", fifo_path })
	if vim.v.shell_error ~= 0 then
		return nil, "failed to create debug fifo"
	end
	return fifo_path
end

local function ensure_debug_fifo_writer()
	if not state.debug_fifo_path then
		return false
	end

	if state.debug_fifo_handle then
		return true
	end

	local handle = io.open(state.debug_fifo_path, "w")
	if not handle then
		return false
	end

	state.debug_fifo_handle = handle
	return true
end

function M.stop()
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

	if is_job_running() then
		M.stop()
	end

	if opts.debug then
		local fifo_path, fifo_err = make_debug_fifo()
		if not fifo_path then
			notify(fifo_err or "Failed to prepare debug fifo", vim.log.levels.ERROR)
			return
		end
		state.debug_fifo_path = fifo_path
		state.debug_active = true
	end

	local origin_win = vim.api.nvim_get_current_win()
	open_window()

	state.script_path = script_path
	local term_cmd = opts.debug and build_debug_cmd(script_path) or build_cmd(script_path)
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
				notify(string.format("YLC terminal exited with code %d for %s", code, exited_script or "<unknown>"), vim.log.levels.WARN)
			end)
		end,
	})

	if opts.debug and not ensure_debug_fifo_writer() then
		notify("Failed to open debug fifo writer", vim.log.levels.ERROR)
	end

	vim.api.nvim_set_current_win(origin_win)
	if opts.debug then
		notify("Started YLC under lldb for " .. script_path)
	else
		notify("Started YLC for " .. script_path)
	end
end

function M.open_debug()
	M.open({ debug = true })
end

function M.restart()
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

	if state.debug_active then
		if not ensure_debug_fifo_writer() then
			notify("Debug fifo is not available", vim.log.levels.ERROR)
			return
		end
		state.debug_fifo_handle:write(escape_text(text))
		state.debug_fifo_handle:flush()
		return
	end

	vim.fn.chansend(state.job_id, escape_text(text))
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

	local server_text = get_text_from_range_server(bufnr)
	if server_text and server_text ~= "" then
		M.send(server_text)
		return
	end

	local ts_ok = treesitter_status(bufnr)
	local node = find_runnable_treesitter_node(bufnr)
	if not node then
		if not ts_ok then
			notify("YLC Tree-sitter unavailable, falling back to current line", vim.log.levels.WARN)
		end
		M.send_current_line()
		return
	end

	local text = get_form_text_from_node_start(node, bufnr)
	if not text or text == "" then
		M.send_current_line()
		return
	end

	M.send(text)
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
	register_treesitter_parser()
	load_treesitter_parser()

	vim.api.nvim_create_autocmd({ "BufWritePost" }, {
		group = autocmd_group,
		pattern = "*.ylc",
		callback = function(args)
			if vim.bo[args.buf].filetype ~= "ylc" then
				return
			end

			local prev_buf = vim.api.nvim_get_current_buf()
			if prev_buf ~= args.buf then
				vim.api.nvim_buf_call(args.buf, function()
					M.sync_current_buffer_to_range_server()
				end)
				return
			end

			M.sync_current_buffer_to_range_server()
		end,
	})
end

function M.debug_treesitter()
	local bufnr = vim.api.nvim_get_current_buf()
	local ok, reason = treesitter_status(bufnr)
	if ok then
		notify("YLC Tree-sitter active", vim.log.levels.INFO)
	else
		notify("YLC Tree-sitter inactive: " .. reason, vim.log.levels.WARN)
	end
end

return M
