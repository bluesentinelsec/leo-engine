-- Leo Engine platformer demo (simple SMB-inspired level 1).

local graphics = leo.graphics
local font_api = leo.font
local camera_api = leo.camera
local collision = leo.collision
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local world_w = 3600
local world_h = 720

local tile = 40
local ground_y = screen_h - tile * 2

local bricks = {}
local cam = nil
local font = nil

local player = {
  x = 0,
  y = 0,
  w = 28,
  h = 36,
  vx = 0,
  vy = 0,
  speed = 240,
  jump_speed = 460,
  on_ground = false,
}

local enemy = {
  x = 620,
  y = 0,
  w = 32,
  h = 28,
  vx = 0,
  vy = 0,
  dir = -1,
  speed = 90,
  alive = true,
}

local goal = {
  x = world_w - 120,
  y = ground_y - 140,
  w = 18,
  h = 160,
}

local score = 0
local lives = 3
local game_over = false
local win = false

local function add_brick(x, y, w, h)
  bricks[#bricks + 1] = { x = x, y = y, w = w, h = h }
end

local function add_platform(x, y, count)
  for i = 0, count - 1 do
    add_brick(x + i * tile, y, tile, tile)
  end
end

local function build_level()
  bricks = {}

  local tiles = math.floor(world_w / tile)
  for i = 0, tiles do
    add_brick(i * tile, ground_y, tile, tile)
  end

  add_platform(360, ground_y - tile * 3, 4)
  add_platform(860, ground_y - tile * 5, 3)
  add_platform(1180, ground_y - tile * 2, 2)
  add_platform(1600, ground_y - tile * 4, 5)
  add_platform(2140, ground_y - tile * 3, 3)
  add_platform(2650, ground_y - tile * 5, 4)

  add_platform(3100, ground_y - tile * 2, 2)
end

local function reset_player()
  player.x = 120
  player.y = ground_y - player.h
  player.vx = 0
  player.vy = 0
  player.on_ground = true
end

local function reset_enemy()
  enemy.x = 620
  enemy.y = ground_y - enemy.h
  enemy.vx = 0
  enemy.vy = 0
  enemy.dir = -1
  enemy.alive = true
end

local function reset_game()
  score = 0
  lives = 3
  game_over = false
  win = false
  build_level()
  reset_player()
  reset_enemy()
end

local function rect_overlap(a, b)
  return collision.checkRecs({ a = a, b = b })
end

local function resolve_collisions_x(entity)
  local rect = { x = entity.x, y = entity.y + 1, w = entity.w, h = entity.h - 2 }
  for _, brick in ipairs(bricks) do
    if rect_overlap(rect, brick) then
      if entity.vx > 0 then
        entity.x = brick.x - entity.w
      elseif entity.vx < 0 then
        entity.x = brick.x + brick.w
      end
      entity.vx = 0
      rect.x = entity.x
    end
  end
end

local function resolve_collisions_y(entity)
  local rect = { x = entity.x + 1, y = entity.y, w = entity.w - 2, h = entity.h }
  local on_ground = false
  for _, brick in ipairs(bricks) do
    if rect_overlap(rect, brick) then
      if entity.vy > 0 then
        entity.y = brick.y - entity.h
        entity.vy = 0
        on_ground = true
      elseif entity.vy < 0 then
        entity.y = brick.y + brick.h
        entity.vy = 0
      end
      rect.y = entity.y
    end
  end
  return on_ground
end

local function is_solid_at(x, y)
  for _, brick in ipairs(bricks) do
    if x >= brick.x and x <= brick.x + brick.w and y >= brick.y and y <= brick.y + brick.h then
      return true
    end
  end
  return false
end

local function update_player(dt, input)
  local move = 0
  if input.keyboard:isDown("left") or input.keyboard:isDown("a") then
    move = move - 1
  end
  if input.keyboard:isDown("right") or input.keyboard:isDown("d") then
    move = move + 1
  end

  local pad = input.gamepads[1]
  if pad and pad:isConnected() then
    local axis = pad:axis("leftx")
    if math.abs(axis) > 0.15 then
      move = move + axis
    end
    if pad:isDown("dpadleft") then
      move = move - 1
    elseif pad:isDown("dpadright") then
      move = move + 1
    end
  end

  player.vx = move * player.speed

  local jump_pressed = input.keyboard:isPressed("space") or input.keyboard:isPressed("up") or input.keyboard:isPressed("w")
  if pad and pad:isConnected() and pad:isPressed("south") then
    jump_pressed = true
  end

  if jump_pressed and player.on_ground then
    player.vy = -player.jump_speed
    player.on_ground = false
  end

  player.vy = player.vy + 1200 * dt

  player.x = player.x + player.vx * dt
  resolve_collisions_x(player)

  player.y = player.y + player.vy * dt
  player.on_ground = resolve_collisions_y(player)

  player.x = math_api.clamp(player.x, 0, world_w - player.w)
  player.y = math_api.clamp(player.y, 0, world_h - player.h)
end

local function update_enemy(dt)
  if not enemy.alive then
    return
  end

  enemy.vx = enemy.dir * enemy.speed
  enemy.vy = enemy.vy + 1200 * dt

  enemy.x = enemy.x + enemy.vx * dt
  resolve_collisions_x(enemy)

  enemy.y = enemy.y + enemy.vy * dt
  resolve_collisions_y(enemy)

  local foot_x = enemy.x + (enemy.dir > 0 and enemy.w + 2 or -2)
  local foot_y = enemy.y + enemy.h + 4
  if not is_solid_at(foot_x, foot_y) then
    enemy.dir = -enemy.dir
  end
end

local function check_enemy_hits()
  if not enemy.alive then
    return
  end

  local prect = { x = player.x, y = player.y, w = player.w, h = player.h }
  local erect = { x = enemy.x, y = enemy.y, w = enemy.w, h = enemy.h }
  if rect_overlap(prect, erect) then
    local from_above = player.vy > 0 and (player.y + player.h - enemy.y) < 16
    if from_above then
      enemy.alive = false
      player.vy = -player.jump_speed * 0.6
      score = score + 200
    else
      lives = lives - 1
      if lives <= 0 then
        game_over = true
        win = false
      else
        reset_player()
      end
    end
  end
end

local function check_goal()
  local prect = { x = player.x, y = player.y, w = player.w, h = player.h }
  local grect = { x = goal.x, y = goal.y, w = goal.w, h = goal.h }
  if rect_overlap(prect, grect) then
    game_over = true
    win = true
  end
end

local function update_game(dt, input)
  update_player(dt, input)
  update_enemy(dt)
  check_enemy_hits()
  check_goal()

  cam:update(dt, {
    target = { x = player.x + player.w * 0.5, y = player.y + player.h * 0.5 },
  })
end

local function draw_bricks()
  for _, brick in ipairs(bricks) do
    graphics.drawRectangleFilled({
      x = brick.x,
      y = brick.y,
      w = brick.w,
      h = brick.h,
      r = 150,
      g = 90,
      b = 40,
      a = 255,
    })
    graphics.drawRectangleOutline({
      x = brick.x + 4,
      y = brick.y + 4,
      w = brick.w - 8,
      h = brick.h - 8,
      r = 200,
      g = 130,
      b = 80,
      a = 220,
    })
  end
end

local function draw_player()
  graphics.drawRectangleFilled({
    x = player.x,
    y = player.y,
    w = player.w,
    h = player.h,
    r = 220,
    g = 70,
    b = 60,
    a = 255,
  })
  graphics.drawRectangleFilled({
    x = player.x + 4,
    y = player.y + 6,
    w = player.w - 8,
    h = 10,
    r = 240,
    g = 200,
    b = 180,
    a = 220,
  })
end

local function draw_enemy()
  if not enemy.alive then
    return
  end
  graphics.drawRectangleFilled({
    x = enemy.x,
    y = enemy.y,
    w = enemy.w,
    h = enemy.h,
    r = 170,
    g = 100,
    b = 60,
    a = 255,
  })
  graphics.drawRectangleFilled({
    x = enemy.x + 6,
    y = enemy.y + 8,
    w = 6,
    h = 6,
    r = 40,
    g = 30,
    b = 20,
    a = 220,
  })
  graphics.drawRectangleFilled({
    x = enemy.x + enemy.w - 12,
    y = enemy.y + 8,
    w = 6,
    h = 6,
    r = 40,
    g = 30,
    b = 20,
    a = 220,
  })
end

local function draw_goal()
  graphics.drawRectangleFilled({
    x = goal.x,
    y = goal.y,
    w = goal.w,
    h = goal.h,
    r = 220,
    g = 220,
    b = 230,
    a = 255,
  })
  graphics.drawRectangleFilled({
    x = goal.x + goal.w,
    y = goal.y + 12,
    w = 34,
    h = 22,
    r = 90,
    g = 170,
    b = 240,
    a = 255,
  })
end

local function draw_ui()
  if not font then
    return
  end

  font_api.print({
    font = font,
    text = string.format("Score: %d", score),
    x = 20,
    y = 18,
    size = 20,
    r = 230,
    g = 230,
    b = 230,
    a = 255,
  })
  font_api.print({
    font = font,
    text = string.format("Lives: %d", lives),
    x = 20,
    y = 42,
    size = 20,
    r = 230,
    g = 230,
    b = 230,
    a = 255,
  })

  if game_over then
    local title = win and "You Win!" or "Game Over"
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
    font_api.print({
      font = font,
      text = "Enter/Start: Play Again",
      x = screen_w * 0.5 - 170,
      y = screen_h * 0.5 + 10,
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
      y = screen_h * 0.5 + 36,
      size = 18,
      r = 190,
      g = 210,
      b = 230,
      a = 255,
    })
  end
end

function leo.load()
  local sw, sh = graphics.getSize()
  screen_w = sw
  screen_h = sh

  cam = camera_api.new({
    position = { x = player.x, y = player.y },
    target = { x = player.x, y = player.y },
    offset = { x = screen_w * 0.5, y = screen_h * 0.5 },
    zoom = 1.0,
    rotation = 0.0,
    deadzone = { w = 240, h = 140 },
    smooth_time = 0.12,
    bounds = { x = 0, y = 0, w = world_w, h = world_h },
    clamp = true,
  })

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
  graphics.clear(120, 180, 235, 255)

  graphics.beginCamera(cam)
  draw_bricks()
  draw_goal()
  draw_enemy()
  draw_player()
  graphics.endCamera()

  draw_ui()
end

function leo.shutdown()
end
