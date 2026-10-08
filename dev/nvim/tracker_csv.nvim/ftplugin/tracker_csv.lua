local tracker = require('tracker_csv')

vim.bo.commentstring = '# %s'

vim.keymap.set('n', 'dd', function()
  vim.api.nvim_set_current_line('')
end, {
  buffer = 0,
  desc = 'Clear tracker row',
})

vim.keymap.set('n', '<A-k>', '<cmd>TrackerMoveCellUp<cr>', {
  buffer = 0,
  desc = 'Move tracker cell up',
})
vim.keymap.set('n', '<A-j>', '<cmd>TrackerMoveCellDown<cr>', {
  buffer = 0,
  desc = 'Move tracker cell down',
})

vim.keymap.set('n', '<A-d>', function()
  tracker.clear_cell()
end, {
  buffer = 0,
  desc = 'Clear tracker cell',
})

vim.keymap.set('n', '<C-a>', function()
  tracker.adjust(1)
end, {
  buffer = 0,
  desc = 'Increment tracker value',
})

vim.keymap.set('n', '<C-x>', function()
  tracker.adjust(-1)
end, {
  buffer = 0,
  desc = 'Decrement tracker value',
})

vim.keymap.set('x', 'g<C-a>', function()
  tracker.adjust_visual(1)
end, {
  buffer = 0,
  desc = 'Increment tracker column sequentially',
})

vim.keymap.set('x', 'g<C-x>', function()
  tracker.adjust_visual(-1)
end, {
  buffer = 0,
  desc = 'Decrement tracker column sequentially',
})

vim.api.nvim_create_autocmd('BufEnter', {
  buffer = 0,
  callback = function()
    tracker.align()
    tracker.refresh()
  end,
})

vim.api.nvim_create_autocmd({ 'TextChanged', 'TextChangedI' }, {
  buffer = 0,
  callback = function()
    tracker.refresh()
  end,
})

vim.api.nvim_create_autocmd('BufWritePre', {
  buffer = 0,
  callback = function()
    tracker.align(true)
    tracker.refresh()
  end,
})
