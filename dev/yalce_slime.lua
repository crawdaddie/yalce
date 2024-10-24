local term_buf
local term_win

function OpenYlcRepl()
  local repl_buf = vim.api.nvim_get_current_buf()
  local repl_win = vim.api.nvim_get_current_win()

  if term_win and vim.api.nvim_win_is_valid(term_win) then
    vim.api.nvim_win_close(term_win, true)
  end

  vim.cmd('vsplit')
  vim.cmd('wincmd L')
  term_win = vim.api.nvim_get_current_win()

  local cmd = vim.fn.input("=> ", "ylc -i")

  -- Start the terminal in the new window
  term_buf = vim.api.nvim_create_buf(false, true)
  vim.api.nvim_win_set_buf(term_win, term_buf)
  repl_id = vim.fn.termopen(cmd)
  term_buf = vim.api.nvim_create_buf(false, true)
  vim.api.nvim_win_set_buf(term_win, term_buf)
  repl_id = vim.fn.termopen(cmd, {
    on_stdout = function(_, _, _)
      if vim.api.nvim_win_is_valid(term_win) then
        vim.schedule(function()
          vim.api.nvim_win_set_cursor(term_win, { vim.api.nvim_buf_line_count(term_buf), 0 })
        end)
      end
    end
  })


  -- Move focus back to the original REPL window
  vim.api.nvim_set_current_win(repl_win)

  vim.api.nvim_buf_set_var(repl_buf, 'slime_config', { jobid = repl_id })
  vim.api.nvim_buf_set_option(repl_buf, 'syntax', 'ylc')
end
