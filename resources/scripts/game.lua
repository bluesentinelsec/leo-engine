local graphics = leo.graphics
local camera_api = leo.camera
local math_api = leo.math
local tiled = leo.tiled

local map = nil
local cam = nil
local map_w = 0
local map_h = 0

local cursor = {
  x = 0,
  y = 0,
  speed = 220
}

local function clamp_cursor()
  cursor.x = math_api.clamp(cursor.x, 0, map_w)
  cursor.y = math_api.clamp(cursor.y, 0, map_h)
end

local function move_cursor(dt, input)
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
    local speed = cursor.speed * dt
    cursor.x = cursor.x + (move_x / length) * speed
    cursor.y = cursor.y + (move_y / length) * speed
  end

  clamp_cursor()
end

leo.load = function()
  map = tiled.load({ path = "resources/maps/map.tmx" })
  map_w, map_h = map:getPixelSize()

  cursor.x = map_w * 0.5
  cursor.y = map_h * 0.5

  cam = camera_api.new({
    position = { x = cursor.x, y = cursor.y },
    target = { x = cursor.x, y = cursor.y },
    bounds = { x = 0, y = 0, w = map_w, h = map_h },
    clamp = true,
    deadzone = { w = 0, h = 0 },
    smooth_time = 0.08,
  })
end

leo.update = function(dt, input)
  if not map then
    return
  end

  move_cursor(dt, input)
  cam:update(dt, { target = { x = cursor.x, y = cursor.y } })
end

leo.draw = function()
  graphics.clear(16, 16, 20, 255)

  if not map then
    return
  end

  graphics.beginCamera(cam)
  map:draw({ x = 0, y = 0 })
  graphics.drawRectangleFilled({
    x = cursor.x - 6,
    y = cursor.y - 6,
    w = 12,
    h = 12,
    r = 255,
    g = 220,
    b = 120,
    a = 255,
  })
  graphics.endCamera()
end
