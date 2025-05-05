local term_buf
local term_win
local repl_id


vim.cmd([[
function! _EscapeText_ylc(text)
  let lines = split(a:text, '\n', 1)

  if len(lines) <= 1
    return [a:text, "\n\n"]
  endif

  let result = []

  for i in range(len(lines) - 1)
    let line = lines[i]
    if line =~ '\S'
      call add(result, line . ' \')
    endif
  endfor

  if lines[-1] =~ '\S'
    call add(result, lines[-1])
  endif

  return [join(result, "\n"), "\n\n"]
endfunction
]])

function OpenYlcRepl()
  -- Store the current buffer (this will be our REPL buffer)
  local repl_buf = vim.api.nvim_get_current_buf()
  local repl_win = vim.api.nvim_get_current_win()

  -- -- Kill existing terminal process if it exists
  -- if repl_id and vim.api.nvim_chan_is_valid(repl_id) then
  --   vim.fn.jobstop(repl_id)
  -- end
  -- -- Close existing terminal window if it exists
  -- if term_win and vim.api.nvim_win_is_valid(term_win) then
  --   vim.api.nvim_win_close(term_win, true)
  -- end

  -- Create a new window to the right for the terminal
  vim.cmd('vsplit')
  vim.cmd('wincmd L')
  term_win = vim.api.nvim_get_current_win()

  -- Prompt for the command
  ---- Get the current buffer's filename
  local current_file = vim.fn.expand('%:p')
  -- Get the current environment variables
  -- Get just the specific environment variable you want to pass
  local ylc_base_dir = vim.fn.getenv("YLC_BASE_DIR") or ""

  -- Prompt for the command, including the current filename
  local cmd = vim.fn.input("=> ", "ylc " .. current_file .. " -i")
  -- local cmd = vim.fn.input("=> ", "ylc -i")

  -- Start the terminal in the new window
  term_buf = vim.api.nvim_create_buf(false, true)
  vim.api.nvim_win_set_buf(term_win, term_buf)
  repl_id = vim.fn.termopen(cmd, {
    on_stdout = function(_, _, _)
      if vim.api.nvim_win_is_valid(term_win) then
        vim.schedule(function()
          vim.api.nvim_win_set_cursor(term_win, { vim.api.nvim_buf_line_count(term_buf), 0 })
        end)
      end
    end,
    env = {
      YLC_BASE_DIR = ylc_base_dir
    }
  })


  -- Move focus back to the original REPL window
  vim.api.nvim_set_current_win(repl_win)

  -- Set buffer-local variables and syntax for the REPL buffer
  vim.api.nvim_buf_set_var(repl_buf, 'slime_config', { jobid = repl_id })
  vim.api.nvim_buf_set_option(repl_buf, 'syntax', 'ylc')
  vim.api.nvim_buf_set_option(repl_buf, 'filetype', 'ylc')
end
