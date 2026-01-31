-- Leo Engine camera demo: grid + animated player.

local graphics = leo.graphics
local camera_api = leo.camera
local animation_api = leo.animation

local cam = nil
local anim = nil

local world_w = 2400
local world_h = 1400
local grid_step = 120

local player = {
  x = world_w * 0.5,
  y = world_h * 0.5,
  speed = 260,
}

local function draw_grid()
  local x = 0
  while x <= world_w do
    graphics.drawLine({
      x1 = x,
      y1 = 0,
      x2 = x,
      y2 = world_h,
      r = 40,
      g = 50,
      b = 70,
      a = 120,
    })
    x = x + grid_step
  end

  local y = 0
  while y <= world_h do
    graphics.drawLine({
      x1 = 0,
      y1 = y,
      x2 = world_w,
      y2 = y,
      r = 40,
      g = 50,
      b = 70,
      a = 120,
    })
    y = y + grid_step
  end

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
end

local function move_player(dt, input)
  local move_x = 0
  local move_y = 0

  if input.keyboard:isDown("a") or input.keyboard:isDown("left") then
    move_x = move_x - 1
  end
  if input.keyboard:isDown("d") or input.keyboard:isDown("right") then
    move_x = move_x + 1
  end
  if input.keyboard:isDown("w") or input.keyboard:isDown("up") then
    move_y = move_y - 1
  end
  if input.keyboard:isDown("s") or input.keyboard:isDown("down") then
    move_y = move_y + 1
  end

  if move_x ~= 0 or move_y ~= 0 then
    local length = math.sqrt(move_x * move_x + move_y * move_y)
    local speed = player.speed * dt
    player.x = player.x + (move_x / length) * speed
    player.y = player.y + (move_y / length) * speed
  end

  if player.x < 0 then
    player.x = 0
  end
  if player.y < 0 then
    player.y = 0
  end
  if player.x > world_w then
    player.x = world_w
  end
  if player.y > world_h then
    player.y = world_h
  end
end

function leo.load()
  local screen_w, screen_h = graphics.getSize()

  cam = camera_api.new({
    position = { x = player.x, y = player.y },
    target = { x = player.x, y = player.y },
    offset = { x = screen_w * 0.5, y = screen_h * 0.5 },
    zoom = 1.0,
    rotation = 0.0,
    deadzone = { w = 220, h = 140 },
    smooth_time = 0.12,
    bounds = { x = 0, y = 0, w = world_w, h = world_h },
    clamp = true,
  })

  anim = animation_api.newSheet({
    path = "resources/images/animation_test.png",
    frame_w = 64,
    frame_h = 64,
    frame_count = 3,
    frame_time = 0.18,
    looping = true,
    playing = true,
  })
end

function leo.update(dt, input)
  move_player(dt, input)
  anim:update(dt)

  cam:update(dt, {
    target = { x = player.x, y = player.y },
  })
end

function leo.draw()
  graphics.clear(18, 18, 22, 255)

  graphics.beginCamera(cam)
  draw_grid()

  anim:draw({
    x = player.x,
    y = player.y,
    ox = 32,
    oy = 32,
    sx = 1.0,
    sy = 1.0,
    angle = 0,
    flipX = false,
    flipY = false,
    r = 255,
    g = 255,
    b = 255,
    a = 255,
  })

  graphics.endCamera()
end

function leo.shutdown()
end
