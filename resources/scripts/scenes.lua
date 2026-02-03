-- Leo Engine scene demo: title/gameplay/options with fade transitions.

local graphics = leo.graphics
local font_api = leo.font

local SCREEN_W = 1280
local SCREEN_H = 720

local font = nil
local base_size = 32
local line_height = 0

local current_index = 1
local next_index = 1
local transition_state = "fading_in"
local transition_timer = 0
local transition_duration = 0.6

local last_left = false
local last_right = false
local last_up = false
local last_down = false

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

local function draw_text(text, x, y, size, r, g, b, a)
  font_api.print({
    font = font,
    text = text,
    x = x,
    y = y,
    size = size,
    r = r,
    g = g,
    b = b,
    a = a or 255,
  })
end

local scenes = {}

scenes[1] = {
  name = "TITLE SCENE",
  bg = { 20, 36, 92 },
  draw = function()
    draw_rect(0, 0, SCREEN_W, SCREEN_H, 20, 36, 92, 255)
    draw_rect(120, 120, SCREEN_W - 240, 220, 32, 64, 140, 255)
    graphics.drawCircleFilled({
      x = SCREEN_W * 0.5,
      y = 260,
      radius = 80,
      r = 240,
      g = 200,
      b = 80,
      a = 255,
    })
    draw_text("LEO ENGINE", 160, 150, 48, 255, 255, 255, 255)
    draw_text("PRESS ARROWS TO SWITCH", 180, 380, 26, 220, 230, 255, 255)
    graphics.drawTriangleFilled({
      x1 = SCREEN_W * 0.5 - 40,
      y1 = 450,
      x2 = SCREEN_W * 0.5 + 40,
      y2 = 450,
      x3 = SCREEN_W * 0.5,
      y3 = 510,
      r = 255,
      g = 120,
      b = 120,
      a = 255,
    })
  end,
}

scenes[2] = {
  name = "GAMEPLAY SCENE",
  bg = { 18, 24, 22 },
  draw = function()
    draw_rect(0, 0, SCREEN_W, SCREEN_H, 18, 24, 22, 255)
    draw_rect(0, SCREEN_H - 140, SCREEN_W, 140, 28, 40, 32, 255)
    graphics.drawRectangleFilled({
      x = 220,
      y = 380,
      w = 60,
      h = 60,
      r = 120,
      g = 220,
      b = 140,
      a = 255,
    })
    graphics.drawRectangleFilled({
      x = 340,
      y = 340,
      w = 90,
      h = 90,
      r = 220,
      g = 140,
      b = 120,
      a = 255,
    })
    graphics.drawCircleFilled({
      x = 560,
      y = 360,
      radius = 50,
      r = 80,
      g = 180,
      b = 240,
      a = 255,
    })
    graphics.drawLine({
      x1 = 700,
      y1 = 300,
      x2 = 980,
      y2 = 480,
      r = 255,
      g = 220,
      b = 120,
      a = 255,
    })
    draw_text("SCORE 000120", 60, 60, 28, 240, 240, 240, 255)
    draw_text("WAVE 02", 60, 100, 22, 180, 210, 255, 255)
  end,
}

scenes[3] = {
  name = "OPTIONS SCENE",
  bg = { 36, 24, 48 },
  draw = function()
    draw_rect(0, 0, SCREEN_W, SCREEN_H, 36, 24, 48, 255)
    draw_text("OPTIONS", 120, 80, 40, 255, 255, 255, 255)
    draw_text("MUSIC", 140, 170, 24, 220, 210, 255, 255)
    draw_rect(260, 176, 500, 16, 70, 70, 90, 255)
    draw_rect(260, 176, 360, 16, 200, 140, 255, 255)
    draw_text("SFX", 140, 230, 24, 220, 210, 255, 255)
    draw_rect(260, 236, 500, 16, 70, 70, 90, 255)
    draw_rect(260, 236, 280, 16, 120, 220, 180, 255)
    draw_text("SCREEN SHAKE", 140, 290, 24, 220, 210, 255, 255)
    draw_rect(260, 296, 500, 16, 70, 70, 90, 255)
    draw_rect(260, 296, 420, 16, 255, 200, 120, 255)
    graphics.drawRectangleOutline({
      x = 120,
      y = 360,
      w = 300,
      h = 120,
      r = 200,
      g = 160,
      b = 255,
      a = 255,
    })
    draw_text("BACK", 210, 400, 26, 200, 160, 255, 255)
  end,
}

local function begin_transition(target_index)
  if transition_state ~= "idle" then
    return
  end
  if target_index == current_index then
    return
  end
  next_index = target_index
  transition_state = "fading_out"
  transition_timer = 0
end

local function update_transition(dt)
  if transition_state == "idle" then
    return
  end
  transition_timer = transition_timer + dt
  if transition_timer >= transition_duration then
    transition_timer = transition_duration
    if transition_state == "fading_out" then
      current_index = next_index
      transition_state = "fading_in"
      transition_timer = 0
    else
      transition_state = "idle"
      transition_timer = 0
    end
  end
end

local function draw_fade_overlay()
  if transition_state == "idle" then
    return
  end
  local alpha = 0
  if transition_state == "fading_out" then
    alpha = math.floor(255 * (transition_timer / transition_duration))
  else
    alpha = math.floor(255 * (1 - (transition_timer / transition_duration)))
  end
  if alpha > 0 then
    draw_rect(0, 0, SCREEN_W, SCREEN_H, 0, 0, 0, alpha)
  end
end

local function handle_input(input)
  if not input or not input.keyboard then
    return
  end

  local left = input.keyboard:isDown("left")
  local right = input.keyboard:isDown("right")
  local up = input.keyboard:isDown("up")
  local down = input.keyboard:isDown("down")

  if left and not last_left then
    local target = current_index - 1
    if target < 1 then
      target = #scenes
    end
    begin_transition(target)
  elseif right and not last_right then
    local target = current_index + 1
    if target > #scenes then
      target = 1
    end
    begin_transition(target)
  elseif up and not last_up then
    local target = current_index - 1
    if target < 1 then
      target = #scenes
    end
    begin_transition(target)
  elseif down and not last_down then
    local target = current_index + 1
    if target > #scenes then
      target = 1
    end
    begin_transition(target)
  end

  last_left = left
  last_right = right
  last_up = up
  last_down = down
end

function leo.load()
  font = font_api.new("resources/font/font.ttf", base_size)
  line_height = font:getLineHeight()
end

function leo.update(dt, input)
  handle_input(input)
  update_transition(dt)
end

function leo.draw()
  local scene = scenes[current_index]
  if scene and scene.draw then
    scene.draw()
  end

  draw_text(scene.name, 40, SCREEN_H - line_height - 20, 20, 230, 230, 230, 255)
  draw_fade_overlay()
end

function leo.shutdown()
end
