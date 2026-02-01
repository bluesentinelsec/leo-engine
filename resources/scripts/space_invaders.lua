-- Leo Engine Space Invaders (simple).

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
  w = 64,
  h = 20,
  speed = 520,
  fire_delay = 0.35,
  fire_timer = 0,
}

local bullet = {
  x = 0,
  y = 0,
  w = 6,
  h = 18,
  vy = -720,
  active = false,
}

local enemy_bullets = {}
local enemy_fire_timer = 0
local enemy_fire_delay = 1.1

local enemies = {}
local enemy_rows = 5
local enemy_cols = 10
local enemy_w = 52
local enemy_h = 28
local enemy_gap_x = 18
local enemy_gap_y = 16
local enemy_dir = 1
local enemy_speed = 60
local enemy_step = 22
local enemy_origin_x = 0
local enemy_origin_y = 0

local score = 0
local lives = 3
local game_over = false
local win = false

local function reset_player()
  player.x = (screen_w - player.w) * 0.5
  player.y = screen_h - 60
  player.fire_timer = 0
  bullet.active = false
  enemy_bullets = {}
end

local function reset_enemies()
  enemies = {}
  enemy_origin_x = 120
  enemy_origin_y = 120
  enemy_dir = 1

  for r = 1, enemy_rows do
    for c = 1, enemy_cols do
      local x = enemy_origin_x + (c - 1) * (enemy_w + enemy_gap_x)
      local y = enemy_origin_y + (r - 1) * (enemy_h + enemy_gap_y)
      enemies[#enemies + 1] = {
        x = x,
        y = y,
        w = enemy_w,
        h = enemy_h,
        alive = true,
      }
    end
  end
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

local function alive_enemies()
  local count = 0
  for _, e in ipairs(enemies) do
    if e.alive then
      count = count + 1
    end
  end
  return count
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

  if fire_pressed(input) and player.fire_timer <= 0 and not bullet.active then
    bullet.active = true
    bullet.x = player.x + player.w * 0.5 - bullet.w * 0.5
    bullet.y = player.y - bullet.h - 4
    player.fire_timer = player.fire_delay
  end
end

local function update_bullet(dt)
  if not bullet.active then
    return
  end

  bullet.y = bullet.y + bullet.vy * dt
  if bullet.y + bullet.h < 0 then
    bullet.active = false
    return
  end

  local bullet_rect = { x = bullet.x, y = bullet.y, w = bullet.w, h = bullet.h }
  for _, enemy in ipairs(enemies) do
    if enemy.alive then
      local enemy_rect = { x = enemy.x, y = enemy.y, w = enemy.w, h = enemy.h }
      local hit = collision.checkRecs({ a = bullet_rect, b = enemy_rect })
      if hit then
        enemy.alive = false
        bullet.active = false
        score = score + 10
        break
      end
    end
  end
end

local function spawn_enemy_bullet()
  local alive = {}
  for _, enemy in ipairs(enemies) do
    if enemy.alive then
      alive[#alive + 1] = enemy
    end
  end
  if #alive == 0 then
    return
  end

  local shooter = alive[math.random(#alive)]
  enemy_bullets[#enemy_bullets + 1] = {
    x = shooter.x + shooter.w * 0.5 - 4,
    y = shooter.y + shooter.h + 6,
    w = 8,
    h = 16,
    vy = 320,
  }
end

local function update_enemy_bullets(dt)
  local player_rect = { x = player.x, y = player.y, w = player.w, h = player.h }
  for i = #enemy_bullets, 1, -1 do
    local b = enemy_bullets[i]
    b.y = b.y + b.vy * dt
    local rect = { x = b.x, y = b.y, w = b.w, h = b.h }

    if b.y > screen_h + 40 then
      table.remove(enemy_bullets, i)
    else
      local hit = collision.checkRecs({ a = rect, b = player_rect })
      if hit then
        table.remove(enemy_bullets, i)
        lives = lives - 1
        if lives <= 0 then
          game_over = true
          win = false
        end
      end
    end
  end
end

local function update_enemies(dt)
  local left = 99999
  local right = -99999
  for _, enemy in ipairs(enemies) do
    if enemy.alive then
      enemy.x = enemy.x + enemy_speed * enemy_dir * dt
      left = math.min(left, enemy.x)
      right = math.max(right, enemy.x + enemy.w)
    end
  end

  if left <= 40 or right >= screen_w - 40 then
    enemy_dir = -enemy_dir
    for _, enemy in ipairs(enemies) do
      if enemy.alive then
        enemy.y = enemy.y + enemy_step
        if enemy.y + enemy.h >= player.y then
          game_over = true
          win = false
        end
      end
    end
  end
end

local function update_enemy_fire(dt)
  enemy_fire_timer = enemy_fire_timer + dt
  if enemy_fire_timer >= enemy_fire_delay then
    enemy_fire_timer = enemy_fire_timer - enemy_fire_delay
    spawn_enemy_bullet()
  end
end

local function update_game(dt, input)
  move_player(dt, input)
  update_player_fire(dt, input)
  update_bullet(dt)
  update_enemies(dt)
  update_enemy_fire(dt)
  update_enemy_bullets(dt)

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

local function draw_stars()
  local step = 80
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
  graphics.clear(16, 16, 22, 255)

  draw_stars()

  -- Player
  graphics.drawRectangleFilled({
    x = player.x,
    y = player.y,
    w = player.w,
    h = player.h,
    r = 120,
    g = 200,
    b = 240,
    a = 255,
  })

  -- Player bullet
  if bullet.active then
    graphics.drawRectangleFilled({
      x = bullet.x,
      y = bullet.y,
      w = bullet.w,
      h = bullet.h,
      r = 255,
      g = 220,
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
      graphics.drawRectangleRoundedFilled({
        x = enemy.x,
        y = enemy.y,
        w = enemy.w,
        h = enemy.h,
        radius = 6,
        r = 200,
        g = 180,
        b = 120,
        a = 230,
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
