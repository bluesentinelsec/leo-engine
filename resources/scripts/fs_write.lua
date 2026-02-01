-- Leo Engine fs write demo (PhysFS write dir).

local graphics = leo.graphics
local font_api = leo.font
local fs = leo.fs

local font = nil
local write_dir = nil
local file_path = "saves/fs_demo.txt"
local status = "Press Space to write"
local write_count = 0
local last_list = {}
local last_read = nil

local function safe_call(fn)
  local ok, result = pcall(fn)
  if ok then
    return true, result
  end
  return false, result
end

local function do_write()
  write_count = write_count + 1
  local content = string.format(
    "Leo Engine fs.write demo\nwrites: %d\npath: %s\n", write_count, file_path
  )

  local ok, err = safe_call(function()
    fs.write({ path = file_path, data = content })
  end)

  if ok then
    status = string.format("Wrote %d bytes", #content)
  else
    status = "Write failed: " .. tostring(err)
  end
end

local function do_read()
  local ok, data = safe_call(function()
    return fs.readWriteDir({ path = file_path })
  end)

  if ok then
    last_read = data
    status = string.format("Read %d bytes", #data)
  else
    status = "Read failed: " .. tostring(data)
  end
end

local function do_list()
  local ok, entries = safe_call(function()
    return fs.listWriteDirFiles()
  end)

  if ok then
    last_list = entries
    status = string.format("Listed %d files", #entries)
  else
    status = "List failed: " .. tostring(entries)
  end
end

function leo.load()
  font = font_api.new("resources/fonts/font.ttf", 22)
  write_dir = fs.getWriteDir()
end

function leo.update(dt, input)
  if input.keyboard:isPressed("space") then
    do_write()
  end
  if input.keyboard:isPressed("r") then
    do_read()
  end
  if input.keyboard:isPressed("l") then
    do_list()
  end
end

function leo.draw()
  graphics.clear(16, 16, 22, 255)

  if not font then
    return
  end

  local x = 30
  local y = 30

  font_api.print({
    font = font,
    text = "FS Write Demo",
    x = x,
    y = y,
    size = 28,
    r = 230,
    g = 230,
    b = 240,
    a = 255,
  })
  y = y + 40

  font_api.print({
    font = font,
    text = "Write dir (PhysFS):",
    x = x,
    y = y,
    size = 20,
    r = 170,
    g = 200,
    b = 220,
    a = 255,
  })
  y = y + 26

  font_api.print({
    font = font,
    text = write_dir or "<not set>",
    x = x,
    y = y,
    size = 18,
    r = 190,
    g = 210,
    b = 230,
    a = 255,
  })
  y = y + 30

  font_api.print({
    font = font,
    text = "Path: " .. file_path,
    x = x,
    y = y,
    size = 18,
    r = 190,
    g = 210,
    b = 230,
    a = 255,
  })
  y = y + 28

  font_api.print({
    font = font,
    text = "Space = write, R = read, L = list",
    x = x,
    y = y,
    size = 18,
    r = 150,
    g = 170,
    b = 190,
    a = 255,
  })
  y = y + 28

  font_api.print({
    font = font,
    text = status,
    x = x,
    y = y,
    size = 18,
    r = 255,
    g = 220,
    b = 140,
    a = 255,
  })
  y = y + 30

  if last_read then
    font_api.print({
      font = font,
      text = "Last read preview:",
      x = x,
      y = y,
      size = 18,
      r = 150,
      g = 200,
      b = 170,
      a = 255,
    })
    y = y + 24

    local preview = last_read:gsub("\n", " ")
    if #preview > 120 then
      preview = preview:sub(1, 120) .. "..."
    end

    font_api.print({
      font = font,
      text = preview,
      x = x,
      y = y,
      size = 16,
      r = 170,
      g = 190,
      b = 210,
      a = 255,
    })
    y = y + 26
  end

  if #last_list > 0 then
    font_api.print({
      font = font,
      text = "Write dir files:",
      x = x,
      y = y,
      size = 18,
      r = 150,
      g = 200,
      b = 170,
      a = 255,
    })
    y = y + 24

    for i = 1, math.min(#last_list, 6) do
      font_api.print({
        font = font,
        text = "- " .. tostring(last_list[i]),
        x = x,
        y = y,
        size = 16,
        r = 170,
        g = 190,
        b = 210,
        a = 255,
      })
      y = y + 20
    end
  end
end

function leo.shutdown()
end
