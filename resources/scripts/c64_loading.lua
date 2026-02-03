-- Leo Engine C64 loading demo: BASIC prompt typing, then noisy color-cycling border.

local graphics = leo.graphics
local font_api = leo.font

local elapsed = 0
local stage = "load"
local stage_timer = 0
local typing_timer = 0
local cursor_timer = 0
local cursor_on = true

local typing_interval = 0.14
local cursor_interval = 0.45
local post_load_pause = 0.6
local post_run_pause = 0.4

local load_text = "LOAD \"*\",8,1"
local run_text = "RUN"
local load_count = 0
local run_count = 0
local font = nil
local base_size = 24
local line_height = 0

local SCREEN_W = 1280
local SCREEN_H = 720

local palette = {
  { 0, 0, 0 },       -- black
  { 255, 255, 255 }, -- white
  { 136, 0, 0 },     -- red
  { 170, 255, 238 }, -- cyan
  { 204, 68, 204 },  -- purple
  { 0, 204, 85 },    -- green
  { 0, 0, 170 },     -- blue
  { 238, 238, 119 }, -- yellow
  { 221, 136, 85 },  -- orange
  { 102, 68, 0 },    -- brown
  { 255, 119, 119 }, -- light red
  { 51, 51, 51 },    -- dark gray
  { 119, 119, 119 }, -- medium gray
  { 170, 255, 102 }, -- light green
  { 0, 136, 255 },   -- light blue
  { 187, 187, 187 }, -- light gray
}

local function pick_color(index)
  local color = palette[((index - 1) % #palette) + 1]
  return color[1], color[2], color[3]
end

local function draw_rect(x, y, w, h, r, g, b, a)
  graphics.drawRectangleFilled({
    x = x,
    y = y,
    w = w,
    h = h,
    r = r,
    g = g,
    b = b,
    a = a or 255,
  })
end

local function hash01(value)
  return (math.sin(value * 12.9898 + 78.233) * 43758.5453) % 1
end

local function border_color_index(line_index, step)
  local v = hash01(line_index * 0.31 + step * 0.17)
  return 1 + math.floor(v * #palette)
end

local function draw_border(screen_w, screen_h, thickness, stripe_h)
  local stripes = math.floor(thickness / stripe_h)
  local step = math.floor(elapsed * 240)
  local scroll = math.floor(elapsed * 80)

  for i = 0, stripes do
    local line = i + scroll
    local color_index = border_color_index(line, step)
    local r, g, b = pick_color(color_index)
    local y_top = i * stripe_h
    local y_bot = screen_h - thickness + i * stripe_h

    draw_rect(0, y_top, screen_w, stripe_h, r, g, b, 255)
    draw_rect(0, y_bot, screen_w, stripe_h, r, g, b, 255)
  end

  local side_stripes = math.floor((screen_h - thickness * 2) / stripe_h)
  for i = 0, side_stripes do
    local line = i + scroll + 37
    local color_index = border_color_index(line, step + 11)
    local r, g, b = pick_color(color_index)
    local y = thickness + i * stripe_h
    draw_rect(0, y, thickness, stripe_h, r, g, b, 255)
    draw_rect(screen_w - thickness, y, thickness, stripe_h, r, g, b, 255)
  end
end

function leo.load()
  font = font_api.new("resources/font/font.ttf", base_size)
  line_height = font:getLineHeight()
end

function leo.update(dt, input)
  elapsed = elapsed + dt

  cursor_timer = cursor_timer + dt
  if cursor_timer >= cursor_interval then
    cursor_timer = cursor_timer - cursor_interval
    cursor_on = not cursor_on
  end

  if stage == "load" then
    typing_timer = typing_timer + dt
    while typing_timer >= typing_interval and load_count < #load_text do
      typing_timer = typing_timer - typing_interval
      load_count = load_count + 1
    end
    if load_count >= #load_text then
      stage = "load_done"
      stage_timer = 0
    end
  elseif stage == "load_done" then
    stage_timer = stage_timer + dt
    if stage_timer >= post_load_pause then
      stage = "run"
      typing_timer = 0
    end
  elseif stage == "run" then
    typing_timer = typing_timer + dt
    while typing_timer >= typing_interval and run_count < #run_text do
      typing_timer = typing_timer - typing_interval
      run_count = run_count + 1
    end
    if run_count >= #run_text then
      stage = "run_done"
      stage_timer = 0
    end
  elseif stage == "run_done" then
    stage_timer = stage_timer + dt
    if stage_timer >= post_run_pause then
      stage = "loading"
    end
  end

  if input and input.keyboard and input.keyboard:isDown("escape") then
    leo.quit()
  end
end

local function draw_basic_screen(screen_w, screen_h, border, background_index, border_index)
  local bg_r, bg_g, bg_b = pick_color(background_index)
  local br_r, br_g, br_b = pick_color(border_index)
  draw_rect(0, 0, screen_w, screen_h, br_r, br_g, br_b, 255)
  draw_rect(border, border, screen_w - border * 2, screen_h - border * 2, bg_r, bg_g, bg_b, 255)
end

local function draw_basic_canvas(screen_w, screen_h, border, background_index)
  local bg_r, bg_g, bg_b = pick_color(background_index)
  draw_rect(border, border, screen_w - border * 2, screen_h - border * 2, bg_r, bg_g, bg_b, 255)
end

local function cursor_suffix()
  if cursor_on then
    return "_"
  end
  return ""
end

local function draw_basic_prompt(screen_w, screen_h, border)
  local text_r, text_g, text_b = pick_color(2)
  local text_x = border + 24
  local text_y = border + 20
  local char_w = math.floor(base_size * 0.6)

  font_api.print({
    font = font,
    text = "READY.",
    x = text_x,
    y = text_y,
    size = base_size,
    r = text_r,
    g = text_g,
    b = text_b,
    a = 255,
  })

  text_y = text_y + line_height

  local load_line = string.sub(load_text, 1, load_count)
  if stage == "load" then
    load_line = load_line .. cursor_suffix()
  end

  font_api.print({
    font = font,
    text = load_line,
    x = text_x,
    y = text_y,
    size = base_size,
    r = text_r,
    g = text_g,
    b = text_b,
    a = 255,
  })

  text_y = text_y + line_height

  if stage == "run" or stage == "run_done" or stage == "loading" then
    local run_line = string.sub(run_text, 1, run_count)
    if stage == "run" then
      run_line = run_line .. cursor_suffix()
    end
    font_api.print({
      font = font,
      text = run_line,
      x = text_x,
      y = text_y,
      size = base_size,
      r = text_r,
      g = text_g,
      b = text_b,
      a = 255,
    })
    text_y = text_y + line_height
  end

  if stage == "loading" then
    local text = "LOADING..."
    local phase = math.floor(elapsed * 8.4)
    for i = 1, #text do
      local ch = string.sub(text, i, i)
      local r, g, b = pick_color(((i + phase) % #palette) + 1)
      font_api.print({
        font = font,
        text = ch,
        x = text_x + (i - 1) * char_w,
        y = text_y,
        size = base_size,
        r = r,
        g = g,
        b = b,
        a = 255,
      })
    end
  end
end

function leo.draw()
  local screen_w = SCREEN_W
  local screen_h = SCREEN_H
  local border = 100
  local stripe_h = 2

  if stage == "loading" then
    draw_rect(0, 0, screen_w, screen_h, 0, 0, 0, 255)
    draw_border(screen_w, screen_h, border, stripe_h)
    draw_basic_canvas(screen_w, screen_h, border, 7)
    draw_basic_prompt(screen_w, screen_h, border)
  else
    draw_basic_screen(screen_w, screen_h, border, 7, 15)
    draw_basic_prompt(screen_w, screen_h, border)
  end
end

function leo.shutdown()
end
