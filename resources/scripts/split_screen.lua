-- Leo Engine split-screen demo: two players, two cameras.

local graphics = leo.graphics
local camera_api = leo.camera
local math_api = leo.math

local world_w = 2600
local world_h = 1600
local grid_step = 160

local player1 = { x = 600, y = 500, speed = 280 }
local player2 = { x = 2000, y = 1100, speed = 280 }

local cam1 = nil
local cam2 = nil

local function clamp_player(p)
  p.x = math_api.clamp(p.x, 0, world_w)
  p.y = math_api.clamp(p.y, 0, world_h)
end

local function move_player(dt, input, p, left_key, right_key, up_key, down_key)
  local move_x = 0
  local move_y = 0

  if input.keyboard:isDown(left_key) then
    move_x = move_x - 1
  end
  if input.keyboard:isDown(right_key) then
    move_x = move_x + 1
  end
  if input.keyboard:isDown(up_key) then
    move_y = move_y - 1
  end
  if input.keyboard:isDown(down_key) then
    move_y = move_y + 1
  end

  if move_x ~= 0 or move_y ~= 0 then
    local length = math.sqrt(move_x * move_x + move_y * move_y)
    local speed = p.speed * dt
    p.x = p.x + (move_x / length) * speed
    p.y = p.y + (move_y / length) * speed
  end

  clamp_player(p)
end

local function draw_grid()
  graphics.drawGrid({
    x = 0,
    y = 0,
    w = world_w,
    h = world_h,
    step = grid_step,
    r = 36,
    g = 48,
    b = 68,
    a = 120,
  })

  graphics.drawRectangleOutline({
    x = 0,
    y = 0,
    w = world_w,
    h = world_h,
    r = 90,
    g = 110,
    b = 140,
    a = 200,
  })

  graphics.drawRectangleFilled({
    x = 400,
    y = 260,
    w = 320,
    h = 180,
    r = 60,
    g = 90,
    b = 140,
    a = 160,
  })

  graphics.drawCircleOutline({
    x = 1700,
    y = 420,
    radius = 140,
    r = 90,
    g = 140,
    b = 200,
    a = 180,
  })

  graphics.drawRectangleRoundedOutline({
    x = 1500,
    y = 900,
    w = 380,
    h = 220,
    radius = 18,
    r = 110,
    g = 140,
    b = 180,
    a = 200,
  })
end

local function draw_players()
  graphics.drawCircleFilled({
    x = player1.x,
    y = player1.y,
    radius = 26,
    r = 230,
    g = 80,
    b = 80,
    a = 255,
  })
  graphics.drawCircleOutline({
    x = player1.x,
    y = player1.y,
    radius = 30,
    r = 255,
    g = 180,
    b = 180,
    a = 220,
  })

  graphics.drawCircleFilled({
    x = player2.x,
    y = player2.y,
    radius = 26,
    r = 80,
    g = 220,
    b = 120,
    a = 255,
  })
  graphics.drawCircleOutline({
    x = player2.x,
    y = player2.y,
    radius = 30,
    r = 180,
    g = 255,
    b = 200,
    a = 220,
  })
end

function leo.load()
  local screen_w, screen_h = graphics.getSize()
  local half_h = screen_h * 0.5

  cam1 = camera_api.new({
    position = { x = player1.x, y = player1.y },
    target = { x = player1.x, y = player1.y },
    offset = { x = screen_w * 0.5, y = half_h * 0.5 },
    zoom = 1.0,
    rotation = 0.0,
    deadzone = { w = 220, h = 140 },
    smooth_time = 0.1,
    bounds = { x = 0, y = 0, w = world_w, h = world_h },
    clamp = true,
  })

  cam2 = camera_api.new({
    position = { x = player2.x, y = player2.y },
    target = { x = player2.x, y = player2.y },
    offset = { x = screen_w * 0.5, y = half_h * 0.5 },
    zoom = 1.0,
    rotation = 0.0,
    deadzone = { w = 220, h = 140 },
    smooth_time = 0.1,
    bounds = { x = 0, y = 0, w = world_w, h = world_h },
    clamp = true,
  })
end

function leo.update(dt, input)
  move_player(dt, input, player1, "a", "d", "w", "s")
  move_player(dt, input, player2, "left", "right", "up", "down")

  cam1:update(dt, { target = { x = player1.x, y = player1.y } })
  cam2:update(dt, { target = { x = player2.x, y = player2.y } })
end

function leo.draw()
  graphics.clear(16, 16, 22, 255)

  local screen_w, screen_h = graphics.getSize()
  local half_h = screen_h * 0.5

  graphics.beginViewport(0, 0, screen_w, half_h)
  graphics.beginCamera(cam1)
  draw_grid()
  draw_players()
  graphics.endCamera()
  graphics.endViewport()

  graphics.beginViewport(0, half_h, screen_w, half_h)
  graphics.beginCamera(cam2)
  draw_grid()
  draw_players()
  graphics.endCamera()
  graphics.endViewport()
  graphics.drawRectangleFilled({
    x = 0,
    y = half_h - 2,
    w = screen_w,
    h = 4,
    r = 20,
    g = 20,
    b = 26,
    a = 255,
  })
end

function leo.shutdown()
end
