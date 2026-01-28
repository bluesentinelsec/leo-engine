local graphics = leo.graphics
local collision = leo.collision
local camera_api = leo.camera

local WORLD_W = 4096
local WORLD_H = 4096

local player = {
  x = 256,
  y = 256,
  w = 32,
  h = 32,
  speed = 180
}

local trees = {}
local water = {}
local dirt_patches = {}
local solids = {}

local cam = nil

local function add_rect(list, x, y, w, h)
  list[#list + 1] = { x = x, y = y, w = w, h = h }
end

local function build_world()
  add_rect(water, 700, 600, 500, 380)
  add_rect(water, 1600, 1300, 700, 520)
  add_rect(water, 2800, 900, 520, 720)
  add_rect(water, 2600, 2500, 800, 500)

  for x = 200, 1800, 200 do
    add_rect(trees, x, 360, 48, 48)
    add_rect(trees, x, 760, 48, 48)
  end

  for y = 1200, 2400, 200 do
    add_rect(trees, 520, y, 48, 48)
    add_rect(trees, 920, y, 48, 48)
  end

  for i = 0, 10 do
    add_rect(dirt_patches, 2200 + i * 120, 3000 + (i % 3) * 60, 80, 50)
  end

  for i = 1, #trees do
    solids[#solids + 1] = trees[i]
  end
  for i = 1, #water do
    solids[#solids + 1] = water[i]
  end
end

local function clamp_player()
  if player.x < 0 then
    player.x = 0
  end
  if player.y < 0 then
    player.y = 0
  end
  if player.x + player.w > WORLD_W then
    player.x = WORLD_W - player.w
  end
  if player.y + player.h > WORLD_H then
    player.y = WORLD_H - player.h
  end
end

local function collides(rect)
  for i = 1, #solids do
    local s = solids[i]
    if collision.checkRecs(rect.x, rect.y, rect.w, rect.h, s.x, s.y, s.w, s.h) then
      return true
    end
  end
  return false
end

local function move_player(dx, dy)
  if dx ~= 0 then
    local test = { x = player.x + dx, y = player.y, w = player.w, h = player.h }
    if not collides(test) then
      player.x = test.x
    end
  end

  if dy ~= 0 then
    local test = { x = player.x, y = player.y + dy, w = player.w, h = player.h }
    if not collides(test) then
      player.y = test.y
    end
  end

  clamp_player()
end

local function draw_rects(list, r, g, b, a)
  for i = 1, #list do
    local rect = list[i]
    graphics.drawRectangleFilled(rect.x, rect.y, rect.w, rect.h, r, g, b, a)
  end
end

leo.load = function()
  build_world()
  cam = camera_api.new()
  cam:setBounds(0, 0, WORLD_W, WORLD_H)
  cam:setClamp(true)
  cam:setDeadzone(0, 0)
  cam:setSmoothTime(0.08)
end

leo.update = function(dt, input)
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
    move_x = (move_x / length) * speed
    move_y = (move_y / length) * speed
    move_player(move_x, move_y)
  end

  cam:setTarget(player.x + player.w * 0.5, player.y + player.h * 0.5)
  cam:update(dt)
end

leo.draw = function()
  graphics.clear(32, 28, 24, 255)

  graphics.beginCamera(cam)

  graphics.drawRectangleFilled(0, 0, WORLD_W, WORLD_H, 120, 92, 64, 255)
  draw_rects(dirt_patches, 140, 106, 74, 255)
  draw_rects(water, 48, 96, 180, 220)
  draw_rects(trees, 24, 92, 48, 255)
  graphics.drawRectangleFilled(player.x, player.y, player.w, player.h, 220, 200, 80, 255)

  graphics.endCamera()
end
