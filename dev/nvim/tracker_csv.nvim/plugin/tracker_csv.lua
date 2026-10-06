if vim.g.loaded_tracker_csv then
  return
end

vim.g.loaded_tracker_csv = true

local tracker = require('tracker_csv')

vim.api.nvim_create_user_command('TrackerAlign', function(args)
  tracker.align(args.bang)
end, { bang = true })

vim.api.nvim_create_user_command('TrackerUnalign', function()
  tracker.unalign()
end, {})

vim.api.nvim_create_user_command('TrackerToggle', function()
  tracker.toggle()
end, {})

vim.api.nvim_create_user_command('TrackerMoveCellUp', function()
  tracker.move_cell(-1)
end, {})

vim.api.nvim_create_user_command('TrackerMoveCellDown', function()
  tracker.move_cell(1)
end, {})
