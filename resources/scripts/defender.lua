-- Leo Engine Defender (1981) - simple demo.

local graphics = leo.graphics
local font_api = leo.font
local collision = leo.collision
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local world_w = 3600
local font = nil

local ground_y = screen_h - 70

local player = {
  x = 0,
  y = 0,
  w = 40,
  h = 18,
  vx = 0,
  vy = 0,
  speed = 360,
  accel = 900,
  drag = 0.88,
  fire_delay = 0.22,
  fire_timer = 0,
}

local bullets = {}
local bullet_speed = 720

local enemies = {}
local enemy_count = 8

local humans = {}
local human_count = 6

local score = 0
local lives = 3
local game_over = false
local win = false

local function reset_player()
  player.x = world_w * 0.5
  player.y = screen_h * 0.5
  player.vx = 0
  player.vy = 0
  player.fire_timer = 0
end

local function reset_world()
  enemies = {}
  humans = {}
  bullets = {}

  for i = 1, enemy_count do
    enemies[#enemies + 1] = {
      x = math.random(200, world_w - 200),
      y = math.random(120, ground_y - 180),
      w = 34,
      h = 16,
      vx = (math.random() * 2 - 1) * 120,
      vy = (math.random() * 2 - 1) * 80,
      alive = true,
    }
  end

  for i = 1, human_count do
    humans[#humans + 1] = {
      x = math.random(120, world_w - 120),
      y = ground_y - 14,
      w = 16,
      h = 16,
      alive = true,
    }
  end
end

local function reset_game()
  score = 0
  lives = 3
  game_over = false
  win = false
  reset_player()
  reset_world()
end

local function wrap_world_x(x)
  if x < 0 then
    return x + world_w
  end
  if x > world_w then
    return x - world_w
  end
  return x
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

local function update_player(dt, input)
  local ax = 0
  local ay = 0

  if input.keyboard:isDown("left") or input.keyboard:isDown("a") then
    ax = ax - 1
  end
  if input.keyboard:isDown("right") or input.keyboard:isDown("d") then
    ax = ax + 1
  end
  if input.keyboard:isDown("up") or input.keyboard:isDown("w") then
    ay = ay - 1
  end
  if input.keyboard:isDown("down") or input.keyboard:isDown("s") then
    ay = ay + 1
  end

  local pad = input.gamepads[1]
  if pad and pad:isConnected() then
    local lx = pad:axis("leftx")
    local ly = pad:axis("lefty")
    if math.abs(lx) > 0.15 then
      ax = ax + lx
    end
    if math.abs(ly) > 0.15 then
      ay = ay + ly
    end

    if pad:isDown("dpadleft") then
      ax = ax - 1
    end
    if pad:isDown("dpadright") then
      ax = ax + 1
    end
    if pad:isDown("dpadup") then
      ay = ay - 1
    end
    if pad:isDown("dpaddown") then
      ay = ay + 1
    end
  end

  player.vx = (player.vx + ax * player.accel * dt) * player.drag
  player.vy = (player.vy + ay * player.accel * dt) * player.drag

  player.vx = math_api.clamp(player.vx, -player.speed, player.speed)
  player.vy = math_api.clamp(player.vy, -player.speed, player.speed)

  player.x = wrap_world_x(player.x + player.vx * dt)
  player.y = math_api.clamp(player.y + player.vy * dt, 60, ground_y - 40)

  player.fire_timer = math.max(0, player.fire_timer - dt)
  if fire_pressed(input) and player.fire_timer <= 0 then
    player.fire_timer = player.fire_delay
    local dir = (player.vx >= 0) and 1 or -1
    bullets[#bullets + 1] = {
      x = player.x + dir * 20,
      y = player.y,
      vx = dir * bullet_speed,
      life = 1.1,
      w = 10,
      h = 4,
    }
  end
end

local function update_bullets(dt)
  for i = #bullets, 1, -1 do
    local b = bullets[i]
    b.x = wrap_world_x(b.x + b.vx * dt)
    b.life = b.life - dt
    if b.life <= 0 then
      table.remove(bullets, i)
    end
  end
end

local function update_enemies(dt)
  for _, e in ipairs(enemies) do
    if e.alive then
      e.x = wrap_world_x(e.x + e.vx * dt)
      e.y = e.y + e.vy * dt

      if e.y < 80 then
        e.y = 80
        e.vy = -e.vy
      elseif e.y > ground_y - 80 then
        e.y = ground_y - 80
        e.vy = -e.vy
      end
    end
  end
end

local function check_bullet_hits()
  for i = #bullets, 1, -1 do
    local b = bullets[i]
    local brect = { x = b.x - b.w * 0.5, y = b.y - b.h * 0.5, w = b.w, h = b.h }
    local hit = false
    for _, e in ipairs(enemies) do
      if e.alive then
        local erect = { x = e.x - e.w * 0.5, y = e.y - e.h * 0.5, w = e.w, h = e.h }
        if collision.checkRecs({ a = brect, b = erect }) then
          e.alive = false
          score = score + 50
          hit = true
          break
        end
      end
    end
    if hit then
      table.remove(bullets, i)
    end
  end

  local remaining = 0
  for _, e in ipairs(enemies) do
    if e.alive then
      remaining = remaining + 1
    end
  end
  if remaining == 0 then
    game_over = true
    win = true
  end
end

local function check_player_hits()
  local prect = { x = player.x - player.w * 0.5, y = player.y - player.h * 0.5, w = player.w, h = player.h }
  for _, e in ipairs(enemies) do
    if e.alive then
      local erect = { x = e.x - e.w * 0.5, y = e.y - e.h * 0.5, w = e.w, h = e.h }
      if collision.checkRecs({ a = prect, b = erect }) then
        lives = lives - 1
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

local function update_game(dt, input)
  update_player(dt, input)
  update_bullets(dt)
  update_enemies(dt)
  check_bullet_hits()
  check_player_hits()
end

local function draw_background(camera_x)
  graphics.clear(10, 12, 18, 255)

  graphics.drawRectangleFilled({
    x = -20,
    y = ground_y,
    w = screen_w + 40,
    h = screen_h - ground_y + 40,
    r = 30,
    g = 60,
    b = 30,
    a = 255,
  })

  local star_step = 120
  local start = camera_x - (camera_x % star_step) - star_step
  local x = start
  while x < camera_x + screen_w + star_step do
    local sx = x - camera_x
    local y = 80 + ((x * 13) % (ground_y - 160))
    graphics.drawPixel({
      x = sx,
      y = y,
      r = 80,
      g = 90,
      b = 120,
      a = 190,
    })
    x = x + star_step
  end
end

local function draw_humans(camera_x)
  for _, h in ipairs(humans) do
    if h.alive then
      local x = h.x - camera_x
      graphics.drawRectangleFilled({
        x = x - h.w * 0.5,
        y = h.y - h.h,
        w = h.w,
        h = h.h,
        r = 200,
        g = 200,
        b = 160,
        a = 255,
      })
    end
  end
end

local function draw_enemies(camera_x)
  for _, e in ipairs(enemies) do
    if e.alive then
      graphics.drawRectangleFilled({
        x = e.x - e.w * 0.5 - camera_x,
        y = e.y - e.h * 0.5,
        w = e.w,
        h = e.h,
        r = 220,
        g = 90,
        b = 120,
        a = 255,
      })
    end
  end
end

local function draw_player(camera_x)
  local x = player.x - camera_x
  graphics.drawTriangleFilled({
    x1 = x - 18,
    y1 = player.y + 10,
    x2 = x + 22,
    y2 = player.y,
    x3 = x - 18,
    y3 = player.y - 10,
    r = 120,
    g = 210,
    b = 240,
    a = 255,
  })
end

local function draw_bullets(camera_x)
  for _, b in ipairs(bullets) do
    graphics.drawRectangleFilled({
      x = b.x - b.w * 0.5 - camera_x,
      y = b.y - b.h * 0.5,
      w = b.w,
      h = b.h,
      r = 255,
      g = 220,
      b = 140,
      a = 255,
    })
  end
end

local function draw_radar(camera_x)
  local radar_x = 20
  local radar_y = 20
  local radar_w = 240
  local radar_h = 40

  graphics.drawRectangleOutline({
    x = radar_x,
    y = radar_y,
    w = radar_w,
    h = radar_h,
    r = 100,
    g = 140,
    b = 180,
    a = 220,
  })

  local function blip(x, y, r, g, b)
    local bx = radar_x + (x / world_w) * radar_w
    local by = radar_y + (y / screen_h) * radar_h
    graphics.drawCircleFilled({
      x = bx,
      y = by,
      radius = 3,
      r = r,
      g = g,
      b = b,
      a = 220,
    })
  end

  blip(player.x, player.y, 120, 210, 240)
  for _, e in ipairs(enemies) do
    if e.alive then
      blip(e.x, e.y, 240, 120, 140)
    end
  end
  for _, h in ipairs(humans) do
    if h.alive then
      blip(h.x, h.y, 200, 200, 160)
    end
  end
end

local function draw_ui(camera_x)
  if not font then
    return
  end

  font_api.print({
    font = font,
    text = string.format("Score: %d", score),
    x = screen_w - 180,
    y = 18,
    size = 20,
    r = 220,
    g = 220,
    b = 230,
    a = 255,
  })

  font_api.print({
    font = font,
    text = string.format("Lives: %d", lives),
    x = screen_w - 180,
    y = 42,
    size = 20,
    r = 220,
    g = 220,
    b = 230,
    a = 255,
  })

  font_api.print({
    font = font,
    text = string.format("X: %d", math.floor(player.x)),
    x = screen_w - 180,
    y = 66,
    size = 16,
    r = 180,
    g = 190,
    b = 210,
    a = 220,
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
  local camera_x = player.x - screen_w * 0.5
  if camera_x < 0 then
    camera_x = 0
  elseif camera_x > world_w - screen_w then
    camera_x = world_w - screen_w
  end

  draw_background(camera_x)
  draw_humans(camera_x)
  draw_enemies(camera_x)
  draw_bullets(camera_x)
  draw_player(camera_x)
  draw_radar(camera_x)
  draw_ui(camera_x)
end

function leo.shutdown()
end
