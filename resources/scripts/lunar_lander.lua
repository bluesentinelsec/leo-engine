-- Leo Engine Lunar Lander (simple) demo.

local graphics = leo.graphics
local font_api = leo.font
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local font = nil

local ground_y = screen_h - 80
local pad_w = 180
local pad_h = 10
local pad_x = (screen_w - pad_w) * 0.5
local pad_y = ground_y - pad_h

local lander = {
  x = screen_w * 0.5,
  y = 140,
  vx = 0,
  vy = 0,
  angle = -math.pi * 0.5,
  radius = 18,
  rotate_speed = 2.4,
  thrust = 240,
  fuel = 100,
  max_fuel = 100,
}

local gravity = 180
local max_safe_speed = 90
local max_safe_angle = math.rad(12)

local thrusting = false
local score = 0
local game_over = false
local win = false

local function reset_lander()
  lander.x = screen_w * 0.5
  lander.y = 140
  lander.vx = 0
  lander.vy = 0
  lander.angle = -math.pi * 0.5
  lander.fuel = lander.max_fuel
  thrusting = false
end

local function reset_game()
  score = 0
  game_over = false
  win = false
  reset_lander()
end

local function update_lander(dt, input)
  local rotate = 0
  if input.keyboard:isDown("left") or input.keyboard:isDown("a") then
    rotate = rotate - 1
  end
  if input.keyboard:isDown("right") or input.keyboard:isDown("d") then
    rotate = rotate + 1
  end

  local pad = input.gamepads[1]
  if pad and pad:isConnected() then
    local axis = pad:axis("leftx")
    if math.abs(axis) > 0.15 then
      rotate = rotate + axis
    end
    if pad:isDown("dpadleft") then
      rotate = rotate - 1
    elseif pad:isDown("dpadright") then
      rotate = rotate + 1
    end
  end

  lander.angle = lander.angle + rotate * lander.rotate_speed * dt

  thrusting = input.keyboard:isDown("up") or input.keyboard:isDown("w") or input.keyboard:isDown("space")
  if pad and pad:isConnected() and pad:isDown("south") then
    thrusting = true
  end

  local ax = 0
  local ay = gravity

  if thrusting and lander.fuel > 0 then
    local dir_x = math.cos(lander.angle)
    local dir_y = math.sin(lander.angle)
    ax = ax + dir_x * lander.thrust
    ay = ay + dir_y * lander.thrust

    lander.fuel = math.max(0, lander.fuel - 24 * dt)
  else
    thrusting = false
  end

  lander.vx = lander.vx + ax * dt
  lander.vy = lander.vy + ay * dt

  lander.x = lander.x + lander.vx * dt
  lander.y = lander.y + lander.vy * dt

  lander.x = math_api.clamp(lander.x, lander.radius, screen_w - lander.radius)
  lander.y = math_api.clamp(lander.y, lander.radius, ground_y - lander.radius)
end

local function check_landing()
  local bottom = lander.y + lander.radius
  if bottom < ground_y - 2 then
    return
  end

  local speed = math.sqrt(lander.vx * lander.vx + lander.vy * lander.vy)
  local angle_off = math.abs(lander.angle + math.pi * 0.5)
  local on_pad = lander.x >= pad_x and lander.x <= pad_x + pad_w

  if on_pad and speed <= max_safe_speed and angle_off <= max_safe_angle then
    win = true
    game_over = true
    score = math.floor(100 + lander.fuel * 2)
  else
    win = false
    game_over = true
  end
end

local function update_game(dt, input)
  update_lander(dt, input)
  check_landing()
end

local function draw_background()
  graphics.clear(8, 10, 16, 255)

  local step = 80
  local y = 40
  while y < ground_y - 40 do
    local x = 60
    while x < screen_w - 60 do
      graphics.drawPixel({
        x = x,
        y = y,
        r = 60,
        g = 70,
        b = 100,
        a = 150,
      })
      x = x + step
    end
    y = y + step
  end
end

local function draw_ground()
  graphics.drawRectangleFilled({
    x = 0,
    y = ground_y,
    w = screen_w,
    h = screen_h - ground_y,
    r = 40,
    g = 44,
    b = 52,
    a = 255,
  })
  graphics.drawRectangleFilled({
    x = pad_x,
    y = pad_y,
    w = pad_w,
    h = pad_h,
    r = 220,
    g = 200,
    b = 120,
    a = 255,
  })
end

local function rotated_point(px, py, angle)
  local ca = math.cos(angle)
  local sa = math.sin(angle)
  return px * ca - py * sa, px * sa + py * ca
end

local function draw_lander()
  local size = lander.radius
  local p1x, p1y = rotated_point(0, -size, lander.angle)
  local p2x, p2y = rotated_point(-size * 0.7, size, lander.angle)
  local p3x, p3y = rotated_point(size * 0.7, size, lander.angle)

  graphics.drawTriangleFilled({
    x1 = lander.x + p1x,
    y1 = lander.y + p1y,
    x2 = lander.x + p2x,
    y2 = lander.y + p2y,
    x3 = lander.x + p3x,
    y3 = lander.y + p3y,
    r = 180,
    g = 210,
    b = 230,
    a = 255,
  })

  if thrusting and lander.fuel > 0 then
    local fx, fy = rotated_point(0, size * 1.4, lander.angle)
    local lx, ly = rotated_point(-size * 0.3, size * 0.7, lander.angle)
    local rx, ry = rotated_point(size * 0.3, size * 0.7, lander.angle)
    graphics.drawTriangleFilled({
      x1 = lander.x + lx,
      y1 = lander.y + ly,
      x2 = lander.x + fx,
      y2 = lander.y + fy,
      x3 = lander.x + rx,
      y3 = lander.y + ry,
      r = 240,
      g = 180,
      b = 80,
      a = 220,
    })
  end
end

local function draw_ui()
  if not font then
    return
  end

  font_api.print({
    font = font,
    text = string.format("Fuel: %d", math.floor(lander.fuel)),
    x = 20,
    y = 18,
    size = 20,
    r = 220,
    g = 220,
    b = 230,
    a = 255,
  })

  font_api.print({
    font = font,
    text = string.format("Speed: %d", math.floor(math.sqrt(lander.vx * lander.vx + lander.vy * lander.vy))),
    x = 20,
    y = 42,
    size = 18,
    r = 200,
    g = 210,
    b = 230,
    a = 220,
  })

  if game_over then
    local title = win and "Landed!" or "Crashed"
    font_api.print({
      font = font,
      text = title,
      x = screen_w * 0.5 - 70,
      y = screen_h * 0.5 - 40,
      size = 32,
      r = 255,
      g = 220,
      b = 140,
      a = 255,
    })
    if win then
      font_api.print({
        font = font,
        text = string.format("Score: %d", score),
        x = screen_w * 0.5 - 60,
        y = screen_h * 0.5 - 6,
        size = 20,
        r = 210,
        g = 220,
        b = 230,
        a = 220,
      })
    end
    font_api.print({
      font = font,
      text = "Enter/Start: Try Again",
      x = screen_w * 0.5 - 170,
      y = screen_h * 0.5 + 24,
      size = 18,
      r = 190,
      g = 210,
      b = 230,
      a = 255,
    })
    font_api.print({
      font = font,
      text = "Esc/Back: Quit",
      x = screen_w * 0.5 - 110,
      y = screen_h * 0.5 + 48,
      size = 18,
      r = 190,
      g = 210,
      b = 230,
      a = 255,
    })
  end
end

function leo.load()
  math.randomseed(os.time())
  font = font_api.new("resources/fonts/font.ttf", 20)
  reset_game()
end

function leo.update(dt, input)
  if game_over then
    local restart = input.keyboard:isPressed("enter") or input.keyboard:isPressed("space")
    local quit = input.keyboard:isPressed("escape")

    local pad = input.gamepads[1]
    if pad and pad:isConnected() then
      if pad:isPressed("start") or pad:isPressed("south") then
        restart = true
      end
      if pad:isPressed("back") then
        quit = true
      end
    end

    if restart then
      reset_game()
    elseif quit then
      leo.quit()
    end
    return
  end

  update_game(dt, input)
end

function leo.draw()
  draw_background()
  draw_ground()
  draw_lander()
  draw_ui()
end

function leo.shutdown()
end
