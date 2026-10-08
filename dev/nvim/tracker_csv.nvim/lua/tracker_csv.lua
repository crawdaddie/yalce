local M = {}

local DELIMITER = ';'
local DELIMITER_GAP = 1
local HIGHLIGHT_NS = vim.api.nvim_create_namespace('tracker_csv_columns')
local ODD_COLUMN = 'TrackerCsvOddColumn'
local EVEN_COLUMN = 'TrackerCsvEvenColumn'
local NOTE_NAMES = { 'c', 'c#', 'd', 'd#', 'e', 'f', 'f#', 'g', 'g#', 'a', 'a#', 'b' }
local is_comment
local delimiters
local cell_index

local function replace_at(text, start, finish, value)
  return text:sub(1, start - 1) .. value .. text:sub(finish + 1)
end

local function token_at(text, cursor, pattern, convert)
  local offset = 1

  while true do
    local match = { text:find(pattern, offset) }
    local start, finish = match[1], match[2]
    if not start then
      return nil
    end

    if cursor >= start - 1 and cursor < finish then
      return start, finish, convert(unpack(match, 3))
    end

    offset = finish + 1
  end
end

local function adjust_parameter(cell, cursor, delta)
  local start, finish = token_at(
    cell,
    cursor,
    '([%a])([%x][%x])',
    function()
      return true
    end
  )

  if not start then
    return nil
  end

  local name = cell:sub(start, start)
  local number = tonumber(cell:sub(start + 1, finish), 16)
  number = (number + delta) % 256
  return replace_at(cell, start, finish, name .. string.format('%02x', number))
end

local function adjust_instrument(cell, cursor, delta)
  local start = token_at(
    cell,
    cursor,
    '([%x][%x])([a-gA-G][#b]?%-?%d)',
    function()
      return true
    end
  )

  if not start then
    start = token_at(cell, cursor, '(%d%d)', function()
      return true
    end)

    if start then
      local previous = cell:sub(start - 1, start - 1)
      local following = cell:sub(start + 2, start + 2)
      local valid_start = previous == '' or previous:match('%s') or previous == '('
      local valid_end = following == '' or following:match('%s') or following == ')' or following == ';'

      if not valid_start or not valid_end then
        return nil
      end
    end
  end

  if not start or cursor >= start + 1 then
    return nil
  end

  local number = tonumber(cell:sub(start, start + 1), 10)
  number = (number + delta) % 100
  return replace_at(cell, start, start + 1, string.format('%02d', number))
end

local function adjust_note(cell, cursor, delta)
  local start, finish, note = token_at(
    cell,
    cursor,
    '([a-gA-G])([#b]?)(%-?%d)',
    function(letter, accidental, octave)
      return { letter:lower(), accidental, tonumber(octave) }
    end
  )

  if not start then
    return nil
  end

  local pitch = ({ c = 0, d = 2, e = 4, f = 5, g = 7, a = 9, b = 11 })[note[1]]
  if note[2] == '#' then
    pitch = pitch + 1
  elseif note[2] == 'b' then
    pitch = pitch - 1
  end

  local midi = note[3] * 12 + pitch + delta
  local octave = math.floor(midi / 12)
  local name = NOTE_NAMES[(midi % 12) + 1]
  return replace_at(cell, start, finish, name .. octave)
end

local function adjust_line(line, cursor, delta)
  if is_comment(line) then
    return nil
  end

  local column = cell_index(line, cursor)
  local separators = delimiters(line)
  local start = (separators[column - 1] or 0) + 1
  local finish = separators[column] or #line
  local cell = line:sub(start, finish)
  local cell_cursor = cursor - start + 1
  local updated = adjust_instrument(cell, cell_cursor, delta)

  if not updated then
    updated = adjust_parameter(cell, cell_cursor, delta)
  end

  if not updated then
    updated = adjust_note(cell, cell_cursor, delta)
  end

  if not updated then
    return nil
  end

  return replace_at(line, start, finish, updated)
end

local function fields(line)
  local result = {}
  local field = {}
  local depth = 0

  for index = 1, #line do
    local char = line:sub(index, index)

    if char == '(' then
      depth = depth + 1
    elseif char == ')' and depth > 0 then
      depth = depth - 1
    end

    if char == DELIMITER and depth == 0 then
      result[#result + 1] = table.concat(field)
      field = {}
    else
      field[#field + 1] = char
    end
  end

  result[#result + 1] = table.concat(field)
  return result
end

is_comment = function(line)
  return line:match('^%s*#') ~= nil
end

delimiters = function(line)
  local result = {}
  local depth = 0

  for index = 1, #line do
    local char = line:sub(index, index)

    if char == '(' then
      depth = depth + 1
    elseif char == ')' and depth > 0 then
      depth = depth - 1
    elseif char == DELIMITER and depth == 0 then
      result[#result + 1] = index - 1
    end
  end

  return result
end

cell_index = function(line, cursor)
  local column = 1

  for _, delimiter in ipairs(delimiters(line)) do
    if cursor < delimiter then
      return column
    end

    column = column + 1
  end

  return column
end

local function cell_start(line, column)
  local separators = delimiters(line)

  return separators[column - 1] or 0
end

local function trim_padding(field)
  local result = vim.trim(field)
  local output = {}
  local depth = 0

  for index = 1, #result do
    local char = result:sub(index, index)

    if char == '(' then
      depth = depth + 1
    elseif char == ')' and depth > 0 then
      depth = depth - 1
    end

    if char == DELIMITER and depth > 0 then
      while output[#output] == ' ' do
        output[#output] = nil
      end

      output[#output + 1] = ' '
    end

    output[#output + 1] = char
  end

  return table.concat(output)
end

local function normalize(row)
  for column, field in ipairs(row) do
    row[column] = trim_padding(field)
  end
end

local function width(field)
  return vim.fn.strdisplaywidth(field)
end

local function pad(field, size)
  return field .. string.rep(' ', size - width(field))
end

local function aligned(entries)
  local widths = {}

  for _, entry in ipairs(entries) do
    if entry.row then
      local row = entry.row
      normalize(row)

      for column, field in ipairs(row) do
        widths[column] = math.max(widths[column] or 0, width(field))
      end
    end
  end

  for _, entry in ipairs(entries) do
    if entry.row then
      local row = entry.row

      for column = 1, #row - 1 do
        row[column] = pad(row[column], widths[column] + DELIMITER_GAP)
      end
    end
  end
end

local function lines()
  return vim.api.nvim_buf_get_lines(0, 0, -1, false)
end

local function write(rows)
  local output = {}

  for _, entry in ipairs(rows) do
    if entry.row then
      output[#output + 1] = table.concat(entry.row, DELIMITER)
    else
      output[#output + 1] = entry.line
    end
  end

  vim.api.nvim_buf_set_lines(0, 0, -1, false, output)
end

function M.unalign()
  local rows = {}

  for _, line in ipairs(lines()) do
    if is_comment(line) then
      rows[#rows + 1] = { line = line }
    else
      local row = fields(line)
      normalize(row)
      rows[#rows + 1] = { row = row }
    end
  end

  write(rows)
end

function M.align(force)
  if not force and vim.b.tracker_csv_aligned then
    return
  end

  local rows = {}

  for _, line in ipairs(lines()) do
    if is_comment(line) then
      rows[#rows + 1] = { line = line }
    else
      rows[#rows + 1] = { row = fields(line) }
    end
  end

  aligned(rows)
  write(rows)
  vim.b.tracker_csv_aligned = true
end

function M.toggle()
  if vim.b.tracker_csv_aligned then
    M.unalign()
    vim.b.tracker_csv_aligned = false
    return
  end

  M.align(true)
end

function M.move_cell(delta)
  local source_row, cursor = unpack(vim.api.nvim_win_get_cursor(0))
  local target_row = source_row + delta
  local buffer_lines = lines()

  if target_row < 1 or target_row > #buffer_lines then
    return
  end

  if is_comment(buffer_lines[source_row]) or
      is_comment(buffer_lines[target_row]) then
    return
  end

  local column = cell_index(buffer_lines[source_row], cursor)
  local source = fields(buffer_lines[source_row])
  local target = fields(buffer_lines[target_row])

  while #target < column do
    target[#target + 1] = ''
  end

  source[column], target[column] = target[column], source[column]

  vim.api.nvim_buf_set_lines(0, source_row - 1, source_row, false, {
    table.concat(source, DELIMITER),
  })
  vim.api.nvim_buf_set_lines(0, target_row - 1, target_row, false, {
    table.concat(target, DELIMITER),
  })

  M.align(true)
  M.refresh()

  local updated = vim.api.nvim_buf_get_lines(0, target_row - 1, target_row, false)[1]
  vim.api.nvim_win_set_cursor(0, {
    target_row,
    cell_start(updated, column),
  })
end

function M.adjust(delta)
  local _, cursor = unpack(vim.api.nvim_win_get_cursor(0))
  local line = vim.api.nvim_get_current_line()

  local updated = adjust_line(line, cursor, delta)
  if not updated then
    return
  end

  vim.api.nvim_set_current_line(updated)
  M.align(true)
  M.refresh()
end

function M.clear_cell()
  local _, cursor = unpack(vim.api.nvim_win_get_cursor(0))
  local line = vim.api.nvim_get_current_line()

  if is_comment(line) then
    return
  end

  local cells = fields(line)
  local column = cell_index(line, cursor)
  if not cells[column] then
    return
  end

  cells[column] = ''
  vim.api.nvim_set_current_line(table.concat(cells, DELIMITER))
  M.refresh()
end

function M.adjust_visual(delta)
  local first = vim.fn.getpos("'<")
  local last = vim.fn.getpos("'>")
  local start_row = math.min(first[2], last[2])
  local end_row = math.max(first[2], last[2])
  local cursor = math.min(first[3], last[3]) - 1
  local selected = vim.api.nvim_buf_get_lines(0, start_row - 1, end_row, false)

  for index, line in ipairs(selected) do
    local updated = adjust_line(line, cursor, delta * (index - 1))
    if updated then
      selected[index] = updated
    end
  end

  vim.api.nvim_buf_set_lines(0, start_row - 1, end_row, false, selected)
  M.align(true)
  M.refresh()
end

function M.refresh()
  vim.api.nvim_buf_clear_namespace(0, HIGHLIGHT_NS, 0, -1)

  vim.api.nvim_set_hl(0, ODD_COLUMN, {
    bg = '#252a34',
    default = true,
  })
  vim.api.nvim_set_hl(0, EVEN_COLUMN, {
    bg = '#303641',
    default = true,
  })

  for row, line in ipairs(lines()) do
    if not is_comment(line) and #line > 0 then
      local start = 0
      local column = 1

      for _, delimiter in ipairs(delimiters(line)) do
        vim.api.nvim_buf_add_highlight(
          0,
          HIGHLIGHT_NS,
          column % 2 == 1 and ODD_COLUMN or EVEN_COLUMN,
          row - 1,
          start,
          delimiter
        )

        start = delimiter
        column = column + 1
      end

      vim.api.nvim_buf_add_highlight(
        0,
        HIGHLIGHT_NS,
        column % 2 == 1 and ODD_COLUMN or EVEN_COLUMN,
        row - 1,
        start,
        -1
      )
    end
  end
end

return M
