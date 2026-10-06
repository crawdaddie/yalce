local M = {}

local DELIMITER = ';'
local DELIMITER_GAP = 1
local HIGHLIGHT_NS = vim.api.nvim_create_namespace('tracker_csv_columns')
local ODD_COLUMN = 'TrackerCsvOddColumn'
local EVEN_COLUMN = 'TrackerCsvEvenColumn'

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

local function is_comment(line)
  return line:match('^%s*#') ~= nil
end

local function delimiters(line)
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

local function cell_index(line, cursor)
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
