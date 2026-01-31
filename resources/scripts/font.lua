-- Leo Engine TTF font demo.

local graphics = leo.graphics
local font_api = leo.font

local font = nil
local base_size = 32
local line_height = 0
local counter = 1
local counter_timer = 0
local counter_interval = 0.5

local function advance_for(size)
  return line_height * (size / base_size)
end

function leo.load()
  font = font_api.new("resources/fonts/font.ttf", base_size)
  line_height = font:getLineHeight()
end

function leo.update(dt, input)
  counter_timer = counter_timer + dt
  if counter_timer >= counter_interval then
    counter_timer = counter_timer - counter_interval
    counter = counter + 1
    if counter > 10 then
      counter = 1
    end
  end
end

function leo.draw()
  graphics.clear(16, 16, 22, 255)

  local x = 40
  local y = 40

  font_api.print({
    font = font,
    text = "TTF Font Demo",
    x = x,
    y = y,
    size = base_size,
    r = 230,
    g = 230,
    b = 240,
    a = 255,
  })
  y = y + line_height + 12

  font_api.print({
    font = font,
    text = "font_api.print with table args",
    x = x,
    y = y,
    size = 28,
    r = 160,
    g = 200,
    b = 255,
    a = 255,
  })
  y = y + advance_for(28) + 12

  font_api.print({
    font = font,
    text = "font_api.print size 20",
    x = x,
    y = y,
    size = 20,
    r = 255,
    g = 220,
    b = 140,
    a = 255,
  })
  y = y + advance_for(20) + 12

  font_api.print({
    font = font,
    text = "font_api.print size 32 (default)",
    x = x,
    y = y,
    size = base_size,
    r = 200,
    g = 255,
    b = 180,
    a = 255,
  })
  y = y + line_height + 12

  font_api.print({
    font = font,
    text = "font_api.print size 48 with RGBA",
    x = x,
    y = y,
    size = 48,
    r = 255,
    g = 160,
    b = 200,
    a = 255,
  })
  y = y + advance_for(48) + 16

  font_api.print({
    font = font,
    text = "line height: " .. string.format("%.1f", line_height),
    x = x,
    y = y,
    size = base_size,
    r = 190,
    g = 190,
    b = 210,
    a = 255,
  })
  y = y + line_height + 12

  font_api.print({
    font = font,
    text = string.format("counter: %d", counter),
    x = x,
    y = y,
    size = 26,
    r = 140,
    g = 240,
    b = 200,
    a = 255,
  })
end

function leo.shutdown()
end
