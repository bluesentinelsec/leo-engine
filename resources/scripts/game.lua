local graphics = leo.graphics
local camera_api = leo.camera
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
  if cursor.x < 0 then
    cursor.x = 0
  end
  if cursor.y < 0 then
    cursor.y = 0
  end
  if cursor.x > map_w then
    cursor.x = map_w
  end
  if cursor.y > map_h then
    cursor.y = map_h
  end
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
  map = tiled.load("resources/maps/map.tmx")
  map_w, map_h = map:getPixelSize()

  cam = camera_api.new()
  cam:setBounds(0, 0, map_w, map_h)
  cam:setClamp(true)
  cam:setDeadzone(0, 0)
  cam:setSmoothTime(0.08)

  cursor.x = map_w * 0.5
  cursor.y = map_h * 0.5
end

leo.update = function(dt, input)
  if not map then
    return
  end

  move_cursor(dt, input)
  cam:setTarget(cursor.x, cursor.y)
  cam:update(dt)
end

leo.draw = function()
  graphics.clear(16, 16, 20, 255)

  if not map then
    return
  end

  graphics.beginCamera(cam)
  map:draw(0, 0)
  graphics.drawRectangleFilled(cursor.x - 6, cursor.y - 6, 12, 12, 255, 220, 120, 255)
  graphics.endCamera()
end
