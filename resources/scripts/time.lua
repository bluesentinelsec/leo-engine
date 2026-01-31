-- Leo Engine time demo: run a function every 3 seconds.

local graphics = leo.graphics
local font_api = leo.font

local font = nil
local elapsed = 0
local tick_count = 0
local tick_interval = 3.0
local flash = false

local function on_tick()
  tick_count = tick_count + 1
  flash = not flash
end

function leo.load()
  font = font_api.new("resources/fonts/font.ttf", 28)
end

function leo.update(dt, input)
  elapsed = elapsed + dt
  if elapsed >= tick_interval then
    elapsed = elapsed - tick_interval
    on_tick()
  end
end

function leo.draw()
  if flash then
    graphics.clear(20, 24, 32, 255)
  else
    graphics.clear(16, 16, 22, 255)
  end

  font_api.print({
    font = font,
    text = string.format("ticks: %d", tick_count),
    x = 40,
    y = 40,
    size = 28,
    r = 220,
    g = 230,
    b = 255,
    a = 255,
  })

  font_api.print({
    font = font,
    text = string.format("next in: %.1fs", tick_interval - elapsed),
    x = 40,
    y = 80,
    size = 22,
    r = 170,
    g = 200,
    b = 220,
    a = 255,
  })

  font_api.print({
    font = font,
    text = "(function fires every 3 seconds)",
    x = 40,
    y = 118,
    size = 18,
    r = 150,
    g = 170,
    b = 190,
    a = 255,
  })
end

function leo.shutdown()
end
