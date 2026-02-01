-- Leo Engine Asteroids (simple) demo.

local graphics = leo.graphics
local font_api = leo.font
local collision = leo.collision
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local font = nil

local ship = {
  x = screen_w * 0.5,
  y = screen_h * 0.5,
  vx = 0,
  vy = 0,
  angle = -math.pi * 0.5,
  radius = 16,
  rotate_speed = 3.1,
  thrust = 260,
  drag = 0.985,
  fire_delay = 0.2,
  fire_timer = 0,
}

local bullets = {}
local bullet_speed = 640
local bullet_life = 1.2

local asteroids = {}
local start_asteroids = 6

local score = 0
local lives = 3
local game_over = false
local win = false

local function wrap_position(obj)
  if obj.x < -40 then
    obj.x = screen_w + 40
  elseif obj.x > screen_w + 40 then
    obj.x = -40
  end

  if obj.y < -40 then
    obj.y = screen_h + 40
  elseif obj.y > screen_h + 40 then
    obj.y = -40
  end
end

local function spawn_asteroid(size, x, y)
  local radius = size == 3 and 46 or (size == 2 and 28 or 16)
  local speed = size == 3 and 70 or (size == 2 and 110 or 150)
  local angle = math.random() * math.pi * 2

  asteroids[#asteroids + 1] = {
    x = x or math.random(80, screen_w - 80),
    y = y or math.random(80, screen_h - 80),
    vx = math.cos(angle) * speed,
    vy = math.sin(angle) * speed,
    radius = radius,
    size = size,
    spin = (math.random() * 2 - 1) * 0.8,
    rot = math.random() * math.pi * 2,
  }
end

local function reset_ship()
  ship.x = screen_w * 0.5
  ship.y = screen_h * 0.5
  ship.vx = 0
  ship.vy = 0
  ship.angle = -math.pi * 0.5
  ship.fire_timer = 0
end

local function reset_game()
  score = 0
  lives = 3
  game_over = false
  win = false
  bullets = {}
  asteroids = {}
  reset_ship()

  for i = 1, start_asteroids do
    spawn_asteroid(3)
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

local function update_ship(dt, input)
  local rotate = 0
  if input.keyboard:isDown("left") or input.keyboard:isDown("a") then
    rotate = rotate - 1
  end
  if input.keyboard:isDown("right") or input.keyboard:isDown("d") then
    rotate = rotate + 1
  end

  local thrusting = false
  if input.keyboard:isDown("up") or input.keyboard:isDown("w") then
    thrusting = true
  end

  local pad = input.gamepads[1]
  if pad and pad:isConnected() then
    local axis = pad:axis("leftx")
    if math.abs(axis) > 0.2 then
      rotate = rotate + axis
    end
    if pad:isDown("dpadleft") then
      rotate = rotate - 1
    elseif pad:isDown("dpadright") then
      rotate = rotate + 1
    end

    local ay = pad:axis("lefty")
    if ay < -0.3 then
      thrusting = true
    end
  end

  ship.angle = ship.angle + rotate * ship.rotate_speed * dt

  if thrusting then
    ship.vx = ship.vx + math.cos(ship.angle) * ship.thrust * dt
    ship.vy = ship.vy + math.sin(ship.angle) * ship.thrust * dt
  end

  ship.vx = ship.vx * ship.drag
  ship.vy = ship.vy * ship.drag

  ship.x = ship.x + ship.vx * dt
  ship.y = ship.y + ship.vy * dt
  wrap_position(ship)

  ship.fire_timer = math.max(0, ship.fire_timer - dt)
  if fire_pressed(input) and ship.fire_timer <= 0 then
    ship.fire_timer = ship.fire_delay
    bullets[#bullets + 1] = {
      x = ship.x + math.cos(ship.angle) * (ship.radius + 4),
      y = ship.y + math.sin(ship.angle) * (ship.radius + 4),
      vx = math.cos(ship.angle) * bullet_speed + ship.vx * 0.4,
      vy = math.sin(ship.angle) * bullet_speed + ship.vy * 0.4,
      life = bullet_life,
      radius = 3,
    }
  end
end

local function update_bullets(dt)
  for i = #bullets, 1, -1 do
    local b = bullets[i]
    b.x = b.x + b.vx * dt
    b.y = b.y + b.vy * dt
    b.life = b.life - dt
    if b.life <= 0 then
      table.remove(bullets, i)
    else
      wrap_position(b)
    end
  end
end

local function split_asteroid(asteroid)
  if asteroid.size <= 1 then
    return
  end

  local next_size = asteroid.size - 1
  spawn_asteroid(next_size, asteroid.x + math.random(-8, 8), asteroid.y + math.random(-8, 8))
  spawn_asteroid(next_size, asteroid.x + math.random(-8, 8), asteroid.y + math.random(-8, 8))
end

local function update_asteroids(dt)
  for _, a in ipairs(asteroids) do
    a.x = a.x + a.vx * dt
    a.y = a.y + a.vy * dt
    a.rot = a.rot + a.spin * dt
    wrap_position(a)
  end
end

local function check_bullet_hits()
  for i = #bullets, 1, -1 do
    local b = bullets[i]
    local hit_index = nil

    for j, a in ipairs(asteroids) do
      local hit = collision.checkCircles({
        c1 = { x = b.x, y = b.y },
        r1 = b.radius,
        c2 = { x = a.x, y = a.y },
        r2 = a.radius,
      })
      if hit then
        hit_index = j
        break
      end
    end

    if hit_index then
      local asteroid = asteroids[hit_index]
      table.remove(asteroids, hit_index)
      table.remove(bullets, i)

      score = score + (asteroid.size == 3 and 20 or (asteroid.size == 2 and 50 or 100))
      split_asteroid(asteroid)
    end
  end

  if #asteroids == 0 then
    game_over = true
    win = true
  end
end

local function check_ship_hits()
  for _, a in ipairs(asteroids) do
    local hit = collision.checkCircles({
      c1 = { x = ship.x, y = ship.y },
      r1 = ship.radius,
      c2 = { x = a.x, y = a.y },
      r2 = a.radius - 2,
    })
    if hit then
      lives = lives - 1
      if lives <= 0 then
        game_over = true
        win = false
      else
        reset_ship()
      end
      return
    end
  end
end

local function update_game(dt, input)
  update_ship(dt, input)
  update_bullets(dt)
  update_asteroids(dt)
  check_bullet_hits()
  check_ship_hits()
end

local function draw_ship()
  local head_x = ship.x + math.cos(ship.angle) * ship.radius
  local head_y = ship.y + math.sin(ship.angle) * ship.radius
  local left_angle = ship.angle + math.pi * 0.75
  local right_angle = ship.angle - math.pi * 0.75
  local left_x = ship.x + math.cos(left_angle) * ship.radius * 0.9
  local left_y = ship.y + math.sin(left_angle) * ship.radius * 0.9
  local right_x = ship.x + math.cos(right_angle) * ship.radius * 0.9
  local right_y = ship.y + math.sin(right_angle) * ship.radius * 0.9

  graphics.drawTriangleOutline({
    x1 = head_x,
    y1 = head_y,
    x2 = left_x,
    y2 = left_y,
    x3 = right_x,
    y3 = right_y,
    r = 200,
    g = 220,
    b = 240,
    a = 255,
  })
end

local function draw_asteroids()
  for _, a in ipairs(asteroids) do
    graphics.drawCircleOutline({
      x = a.x,
      y = a.y,
      radius = a.radius,
      r = 160,
      g = 180,
      b = 210,
      a = 230,
    })
  end
end

local function draw_bullets()
  for _, b in ipairs(bullets) do
    graphics.drawCircleFilled({
      x = b.x,
      y = b.y,
      radius = b.radius,
      r = 255,
      g = 230,
      b = 160,
      a = 255,
    })
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

function leo.draw()
  graphics.clear(12, 12, 18, 255)

  draw_asteroids()
  draw_bullets()
  draw_ship()

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
