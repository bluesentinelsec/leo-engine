-- Leo Engine Galaxian (simple) demo.

local graphics = leo.graphics
local font_api = leo.font
local collision = leo.collision
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local font = nil

local player = {
  x = 0,
  y = 0,
  w = 44,
  h = 22,
  speed = 520,
  fire_delay = 0.32,
  fire_timer = 0,
}

local player_bullet = {
  x = 0,
  y = 0,
  w = 6,
  h = 18,
  vy = -820,
  active = false,
}

local enemy_bullets = {}
local enemy_fire_timer = 0
local enemy_fire_delay = 1.0

local formation = {
  rows = 4,
  cols = 8,
  start_x = 0,
  start_y = 120,
  gap_x = 72,
  gap_y = 54,
  dir = 1,
  speed = 50,
  drop = 20,
  offset_x = 0,
}

local enemies = {}
local dive_timer = 0
local dive_delay = 1.8

local score = 0
local lives = 3
local game_over = false
local win = false

local function reset_player()
  player.x = screen_w * 0.5 - player.w * 0.5
  player.y = screen_h - 70
  player.fire_timer = 0
  player_bullet.active = false
  enemy_bullets = {}
end

local function enemy_home_pos(enemy)
  local x = formation.start_x + (enemy.col - 1) * formation.gap_x + formation.offset_x
  local y = formation.start_y + (enemy.row - 1) * formation.gap_y
  return x, y
end

local function reset_enemies()
  enemies = {}
  formation.offset_x = 0
  formation.dir = 1

  local total_width = (formation.cols - 1) * formation.gap_x
  formation.start_x = (screen_w - total_width) * 0.5

  for row = 1, formation.rows do
    for col = 1, formation.cols do
      local enemy = {
        row = row,
        col = col,
        w = 36,
        h = 22,
        alive = true,
        state = "formation",
        x = 0,
        y = 0,
        vx = 0,
        vy = 0,
        color = row % 2 == 0 and { 230, 120, 140 } or { 140, 210, 230 },
      }
      enemy.x, enemy.y = enemy_home_pos(enemy)
      enemies[#enemies + 1] = enemy
    end
  end

  dive_timer = 0
end

local function reset_game()
  score = 0
  lives = 3
  game_over = false
  win = false
  reset_player()
  reset_enemies()
  enemy_fire_timer = 0
end

local function move_player(dt, input)
  local move = 0
  if input.keyboard:isDown("a") or input.keyboard:isDown("left") then
    move = move - 1
  end
  if input.keyboard:isDown("d") or input.keyboard:isDown("right") then
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
    end
    if pad:isDown("dpadright") then
      move = move + 1
    end
  end

  if move ~= 0 then
    player.x = player.x + move * player.speed * dt
    player.x = math_api.clamp(player.x, 20, screen_w - player.w - 20)
  end
end

local function fire_pressed(input)
  if input.keyboard:isPressed("space") or input.keyboard:isPressed("enter") then
    return true
  end
  local pad = input.gamepads[1]
  if pad and pad:isConnected() then
    if pad:isPressed("south") or pad:isPressed("rightshoulder") then
      return true
    end
  end
  return false
end

local function update_player_fire(dt, input)
  player.fire_timer = math.max(0, player.fire_timer - dt)

  if fire_pressed(input) and player.fire_timer <= 0 and not player_bullet.active then
    player_bullet.active = true
    player_bullet.x = player.x + player.w * 0.5 - player_bullet.w * 0.5
    player_bullet.y = player.y - player_bullet.h - 4
    player.fire_timer = player.fire_delay
  end
end

local function update_player_bullet(dt)
  if not player_bullet.active then
    return
  end

  player_bullet.y = player_bullet.y + player_bullet.vy * dt
  if player_bullet.y + player_bullet.h < 0 then
    player_bullet.active = false
    return
  end

  local brect = { x = player_bullet.x, y = player_bullet.y, w = player_bullet.w, h = player_bullet.h }
  for _, enemy in ipairs(enemies) do
    if enemy.alive and enemy.state ~= "respawn" then
      local erect = { x = enemy.x, y = enemy.y, w = enemy.w, h = enemy.h }
      if collision.checkRecs({ a = brect, b = erect }) then
        enemy.alive = false
        player_bullet.active = false
        score = score + 20
        break
      end
    end
  end
end

local function spawn_enemy_bullet()
  local candidates = {}
  for _, enemy in ipairs(enemies) do
    if enemy.alive and enemy.state ~= "respawn" then
      candidates[#candidates + 1] = enemy
    end
  end
  if #candidates == 0 then
    return
  end

  local shooter = candidates[math.random(#candidates)]
  enemy_bullets[#enemy_bullets + 1] = {
    x = shooter.x + shooter.w * 0.5 - 3,
    y = shooter.y + shooter.h + 4,
    w = 6,
    h = 16,
    vy = 340,
  }
end

local function update_enemy_bullets(dt)
  local player_rect = { x = player.x, y = player.y, w = player.w, h = player.h }
  for i = #enemy_bullets, 1, -1 do
    local b = enemy_bullets[i]
    b.y = b.y + b.vy * dt
    if b.y > screen_h + 40 then
      table.remove(enemy_bullets, i)
    else
      local rect = { x = b.x, y = b.y, w = b.w, h = b.h }
      if collision.checkRecs({ a = rect, b = player_rect }) then
        table.remove(enemy_bullets, i)
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
end

local function update_formation(dt)
  formation.offset_x = formation.offset_x + formation.speed * formation.dir * dt

  local left = 99999
  local right = -99999
  for _, enemy in ipairs(enemies) do
    if enemy.alive and enemy.state == "formation" then
      local ex, _ = enemy_home_pos(enemy)
      left = math.min(left, ex)
      right = math.max(right, ex + enemy.w)
    end
  end

  if left <= 60 or right >= screen_w - 60 then
    formation.dir = -formation.dir
    formation.start_y = formation.start_y + formation.drop
  end
end

local function pick_dive_enemy()
  local candidates = {}
  for _, enemy in ipairs(enemies) do
    if enemy.alive and enemy.state == "formation" then
      candidates[#candidates + 1] = enemy
    end
  end
  if #candidates == 0 then
    return nil
  end
  return candidates[math.random(#candidates)]
end

local function start_dive(enemy)
  enemy.state = "diving"
  enemy.x, enemy.y = enemy_home_pos(enemy)
  local target_x = player.x + player.w * 0.5
  local target_y = player.y
  local dx = target_x - enemy.x
  local dy = target_y - enemy.y
  local len = math.sqrt(dx * dx + dy * dy)
  if len == 0 then
    len = 1
  end
  enemy.vx = dx / len * 220
  enemy.vy = dy / len * 220
end

local function update_enemy(enemy, dt)
  if not enemy.alive then
    return
  end

  if enemy.state == "formation" then
    enemy.x, enemy.y = enemy_home_pos(enemy)
    return
  end

  if enemy.state == "diving" then
    enemy.x = enemy.x + enemy.vx * dt
    enemy.y = enemy.y + enemy.vy * dt
    if enemy.y > screen_h + 40 then
      enemy.state = "return"
    end
    return
  end

  if enemy.state == "return" then
    local hx, hy = enemy_home_pos(enemy)
    local dx = hx - enemy.x
    local dy = hy - enemy.y
    local dist = math.sqrt(dx * dx + dy * dy)
    if dist < 4 then
      enemy.state = "formation"
      enemy.x, enemy.y = hx, hy
      enemy.vx = 0
      enemy.vy = 0
    else
      enemy.x = enemy.x + dx / dist * 220 * dt
      enemy.y = enemy.y + dy / dist * 220 * dt
    end
  end
end

local function update_enemies(dt)
  update_formation(dt)

  dive_timer = dive_timer + dt
  if dive_timer >= dive_delay then
    dive_timer = dive_timer - dive_delay
    local enemy = pick_dive_enemy()
    if enemy then
      start_dive(enemy)
    end
  end

  for _, enemy in ipairs(enemies) do
    update_enemy(enemy, dt)
  end
end

local function update_enemy_fire(dt)
  enemy_fire_timer = enemy_fire_timer + dt
  if enemy_fire_timer >= enemy_fire_delay then
    enemy_fire_timer = enemy_fire_timer - enemy_fire_delay
    spawn_enemy_bullet()
  end
end

local function check_player_collisions()
  local player_rect = { x = player.x, y = player.y, w = player.w, h = player.h }
  for _, enemy in ipairs(enemies) do
    if enemy.alive then
      local erect = { x = enemy.x, y = enemy.y, w = enemy.w, h = enemy.h }
      if collision.checkRecs({ a = player_rect, b = erect }) then
        lives = lives - 1
        enemy.alive = false
        if lives <= 0 then
          game_over = true
          win = false
        else
          reset_player()
        end
        return
      end
    end
  end
end

local function alive_enemies()
  local count = 0
  for _, enemy in ipairs(enemies) do
    if enemy.alive then
      count = count + 1
    end
  end
  return count
end

local function update_game(dt, input)
  move_player(dt, input)
  update_player_fire(dt, input)
  update_player_bullet(dt)
  update_enemies(dt)
  update_enemy_fire(dt)
  update_enemy_bullets(dt)
  check_player_collisions()

  if alive_enemies() == 0 then
    game_over = true
    win = true
  end
end

function leo.load()
  math.randomseed(os.time())
  font = font_api.new("resources/fonts/font.ttf", 24)
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

local function draw_starfield()
  local step = 90
  local y = 40
  while y < screen_h - 40 do
    local x = 60
    while x < screen_w - 60 do
      graphics.drawPixel({
        x = x,
        y = y,
        r = 40,
        g = 50,
        b = 70,
        a = 140,
      })
      x = x + step
    end
    y = y + step
  end
end

function leo.draw()
  graphics.clear(10, 12, 18, 255)

  draw_starfield()

  -- Player
  graphics.drawTriangleFilled({
    x1 = player.x,
    y1 = player.y + player.h,
    x2 = player.x + player.w * 0.5,
    y2 = player.y,
    x3 = player.x + player.w,
    y3 = player.y + player.h,
    r = 120,
    g = 210,
    b = 240,
    a = 255,
  })

  -- Player bullet
  if player_bullet.active then
    graphics.drawRectangleFilled({
      x = player_bullet.x,
      y = player_bullet.y,
      w = player_bullet.w,
      h = player_bullet.h,
      r = 255,
      g = 230,
      b = 140,
      a = 255,
    })
  end

  -- Enemy bullets
  for _, b in ipairs(enemy_bullets) do
    graphics.drawRectangleFilled({
      x = b.x,
      y = b.y,
      w = b.w,
      h = b.h,
      r = 240,
      g = 120,
      b = 120,
      a = 240,
    })
  end

  -- Enemies
  for _, enemy in ipairs(enemies) do
    if enemy.alive then
      local r, g, b = enemy.color[1], enemy.color[2], enemy.color[3]
      if enemy.state == "diving" then
        r, g, b = 240, 210, 120
      end
      graphics.drawRectangleRoundedFilled({
        x = enemy.x,
        y = enemy.y,
        w = enemy.w,
        h = enemy.h,
        radius = 6,
        r = r,
        g = g,
        b = b,
        a = 240,
      })
    end
  end

  if font then
    font_api.print({
      font = font,
      text = string.format("Score: %d", score),
      x = 30,
      y = 24,
      size = 22,
      r = 220,
      g = 220,
      b = 230,
      a = 255,
    })
    font_api.print({
      font = font,
      text = string.format("Lives: %d", lives),
      x = screen_w - 150,
      y = 24,
      size = 22,
      r = 220,
      g = 220,
      b = 230,
      a = 255,
    })

    if game_over then
      local title = win and "You Win!" or "Game Over"
      font_api.print({
        font = font,
        text = title,
        x = screen_w * 0.5 - 80,
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
end

function leo.shutdown()
end
