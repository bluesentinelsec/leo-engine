-- Leo Engine Missile Command (simple) demo.

local graphics = leo.graphics
local font_api = leo.font
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local font = nil

local ground_y = screen_h - 70

local cities = {}
local bases = {}

local incoming = {}
local player_missiles = {}
local explosions = {}

local wave = 1
local missiles_total = 0
local missiles_spawned = 0
local spawn_timer = 0

local score = 0
local lives = 1
local game_over = false
local win = false
local mouse_state = nil

local function init_cities_and_bases()
  cities = {}
  local city_w = 40
  local city_h = 24
  local city_y = ground_y - city_h
  local spacing = (screen_w - 2 * 120) / 6

  for i = 1, 6 do
    cities[#cities + 1] = {
      x = 120 + (i - 1) * spacing,
      y = city_y,
      w = city_w,
      h = city_h,
      alive = true,
    }
  end

  bases = {
    { x = 160, y = ground_y, w = 36, h = 20, ammo = 10, alive = true },
    { x = screen_w * 0.5, y = ground_y, w = 36, h = 20, ammo = 10, alive = true },
    { x = screen_w - 160, y = ground_y, w = 36, h = 20, ammo = 10, alive = true },
  }
end

local function reset_player_state()
  player_missiles = {}
  explosions = {}
end

local function reset_wave()
  missiles_spawned = 0
  missiles_total = 12 + (wave - 1) * 3
  spawn_timer = 0.6

  for _, base in ipairs(bases) do
    if base.alive then
      base.ammo = 10
    end
  end
end

local function reset_game()
  wave = 1
  score = 0
  lives = 1
  game_over = false
  win = false
  init_cities_and_bases()
  incoming = {}
  reset_player_state()
  reset_wave()
end

local function alive_cities()
  local count = 0
  for _, city in ipairs(cities) do
    if city.alive then
      count = count + 1
    end
  end
  return count
end

local function any_targets()
  if alive_cities() > 0 then
    return true
  end
  for _, base in ipairs(bases) do
    if base.alive then
      return true
    end
  end
  return false
end

local function choose_target()
  local candidates = {}
  for _, city in ipairs(cities) do
    if city.alive then
      candidates[#candidates + 1] = { kind = "city", ref = city }
    end
  end
  for _, base in ipairs(bases) do
    if base.alive then
      candidates[#candidates + 1] = { kind = "base", ref = base }
    end
  end
  if #candidates == 0 then
    return nil
  end
  return candidates[math.random(#candidates)]
end

local function spawn_incoming()
  local target = choose_target()
  if not target then
    return
  end

  local start_x = math.random(60, screen_w - 60)
  local start_y = 10
  local target_x = target.ref.x
  local target_y = target.ref.y

  local dx = target_x - start_x
  local dy = target_y - start_y
  local len = math.sqrt(dx * dx + dy * dy)
  if len < 1 then
    len = 1
  end

  local speed = 90 + wave * 12
  incoming[#incoming + 1] = {
    x = start_x,
    y = start_y,
    start_x = start_x,
    start_y = start_y,
    target_x = target_x,
    target_y = target_y,
    dir_x = dx / len,
    dir_y = dy / len,
    speed = speed,
    target = target,
  }
end

local function nearest_base(x)
  local best = nil
  local best_dist = nil
  for _, base in ipairs(bases) do
    if base.alive and base.ammo > 0 then
      local dx = x - base.x
      local dist = math.abs(dx)
      if not best_dist or dist < best_dist then
        best_dist = dist
        best = base
      end
    end
  end
  return best
end

local function launch_player_missile(target_x, target_y)
  local base = nearest_base(target_x)
  if not base then
    return
  end

  local dx = target_x - base.x
  local dy = target_y - base.y
  local len = math.sqrt(dx * dx + dy * dy)
  if len < 1 then
    len = 1
  end

  base.ammo = base.ammo - 1

  player_missiles[#player_missiles + 1] = {
    x = base.x,
    y = base.y,
    start_x = base.x,
    start_y = base.y,
    target_x = target_x,
    target_y = target_y,
    dir_x = dx / len,
    dir_y = dy / len,
    speed = 520,
  }
end

local function spawn_explosion(x, y, max_radius)
  explosions[#explosions + 1] = {
    x = x,
    y = y,
    radius = 2,
    max_radius = max_radius or 50,
    grow = true,
  }
end

local function update_explosions(dt)
  for i = #explosions, 1, -1 do
    local e = explosions[i]
    if e.grow then
      e.radius = e.radius + 180 * dt
      if e.radius >= e.max_radius then
        e.radius = e.max_radius
        e.grow = false
      end
    else
      e.radius = e.radius - 180 * dt
      if e.radius <= 0 then
        table.remove(explosions, i)
      end
    end
  end
end

local function update_incoming(dt)
  for i = #incoming, 1, -1 do
    local m = incoming[i]
    m.x = m.x + m.dir_x * m.speed * dt
    m.y = m.y + m.dir_y * m.speed * dt

    local dx = m.target_x - m.x
    local dy = m.target_y - m.y
    if (dx * dx + dy * dy) <= (m.speed * dt) ^ 2 then
      if m.target.kind == "city" then
        m.target.ref.alive = false
      elseif m.target.kind == "base" then
        m.target.ref.alive = false
        m.target.ref.ammo = 0
      end
      spawn_explosion(m.target_x, m.target_y, 46)
      table.remove(incoming, i)
    end
  end
end

local function update_player_missiles(dt)
  for i = #player_missiles, 1, -1 do
    local m = player_missiles[i]
    m.x = m.x + m.dir_x * m.speed * dt
    m.y = m.y + m.dir_y * m.speed * dt

    local dx = m.target_x - m.x
    local dy = m.target_y - m.y
    if (dx * dx + dy * dy) <= (m.speed * dt) ^ 2 then
      spawn_explosion(m.target_x, m.target_y, 64)
      table.remove(player_missiles, i)
    end
  end
end

local function check_explosion_hits()
  for i = #incoming, 1, -1 do
    local m = incoming[i]
    local destroyed = false
    for _, e in ipairs(explosions) do
      local dx = m.x - e.x
      local dy = m.y - e.y
      if dx * dx + dy * dy <= e.radius * e.radius then
        destroyed = true
        score = score + 25
        break
      end
    end
    if destroyed then
      table.remove(incoming, i)
    end
  end
end

local function update_wave(dt)
  if not any_targets() then
    game_over = true
    win = false
    return
  end

  spawn_timer = spawn_timer - dt
  if missiles_spawned < missiles_total and spawn_timer <= 0 then
    spawn_incoming()
    missiles_spawned = missiles_spawned + 1
    spawn_timer = math.max(0.45, 1.0 - wave * 0.04)
  end

  if missiles_spawned >= missiles_total and #incoming == 0 and #explosions == 0 then
    wave = wave + 1
    reset_player_state()
    reset_wave()
  end
end

local function handle_input(input)
  if input.mouse:isPressed("left") then
    local mx = math_api.clamp(input.mouse.x, 0, screen_w)
    local my = math_api.clamp(input.mouse.y, 0, ground_y)
    launch_player_missile(mx, my)
  end
end

local function update_game(dt, input)
  handle_input(input)
  update_player_missiles(dt)
  update_incoming(dt)
  update_explosions(dt)
  check_explosion_hits()
  update_wave(dt)

  if alive_cities() == 0 then
    game_over = true
    win = false
  end
end

local function draw_background()
  graphics.clear(10, 12, 18, 255)

  graphics.drawRectangleFilled({
    x = 0,
    y = ground_y,
    w = screen_w,
    h = screen_h - ground_y,
    r = 24,
    g = 48,
    b = 30,
    a = 255,
  })

  graphics.drawLine({
    x1 = 0,
    y1 = ground_y,
    x2 = screen_w,
    y2 = ground_y,
    r = 80,
    g = 110,
    b = 90,
    a = 220,
  })
end

local function draw_cities()
  for _, city in ipairs(cities) do
    if city.alive then
      graphics.drawRectangleFilled({
        x = city.x - city.w * 0.5,
        y = city.y,
        w = city.w,
        h = city.h,
        r = 80,
        g = 160,
        b = 210,
        a = 255,
      })
    else
      graphics.drawRectangleOutline({
        x = city.x - city.w * 0.5,
        y = city.y,
        w = city.w,
        h = city.h,
        r = 80,
        g = 80,
        b = 90,
        a = 120,
      })
    end
  end
end

local function draw_bases()
  for _, base in ipairs(bases) do
    if base.alive then
      graphics.drawRectangleFilled({
        x = base.x - base.w * 0.5,
        y = base.y - base.h,
        w = base.w,
        h = base.h,
        r = 180,
        g = 180,
        b = 200,
        a = 255,
      })
      graphics.drawTriangleFilled({
        x1 = base.x - 12,
        y1 = base.y - base.h,
        x2 = base.x + 12,
        y2 = base.y - base.h,
        x3 = base.x,
        y3 = base.y - base.h - 16,
        r = 220,
        g = 220,
        b = 240,
        a = 255,
      })
    else
      graphics.drawRectangleOutline({
        x = base.x - base.w * 0.5,
        y = base.y - base.h,
        w = base.w,
        h = base.h,
        r = 80,
        g = 80,
        b = 90,
        a = 120,
      })
    end
  end
end

local function draw_missiles()
  for _, m in ipairs(incoming) do
    graphics.drawLine({
      x1 = m.start_x,
      y1 = m.start_y,
      x2 = m.x,
      y2 = m.y,
      r = 200,
      g = 80,
      b = 90,
      a = 210,
    })
    graphics.drawCircleFilled({
      x = m.x,
      y = m.y,
      radius = 3,
      r = 230,
      g = 100,
      b = 120,
      a = 255,
    })
  end

  for _, m in ipairs(player_missiles) do
    graphics.drawLine({
      x1 = m.start_x,
      y1 = m.start_y,
      x2 = m.x,
      y2 = m.y,
      r = 200,
      g = 220,
      b = 120,
      a = 220,
    })
  end
end

local function draw_explosions()
  for _, e in ipairs(explosions) do
    graphics.drawCircleOutline({
      x = e.x,
      y = e.y,
      radius = e.radius,
      r = 240,
      g = 220,
      b = 140,
      a = 220,
    })
    graphics.drawCircleFilled({
      x = e.x,
      y = e.y,
      radius = e.radius * 0.6,
      r = 240,
      g = 200,
      b = 120,
      a = 120,
    })
  end
end

local function draw_crosshair(mouse)
  local mx = mouse.x
  local my = mouse.y
  graphics.drawLine({
    x1 = mx - 12,
    y1 = my,
    x2 = mx + 12,
    y2 = my,
    r = 200,
    g = 220,
    b = 240,
    a = 220,
  })
  graphics.drawLine({
    x1 = mx,
    y1 = my - 12,
    x2 = mx,
    y2 = my + 12,
    r = 200,
    g = 220,
    b = 240,
    a = 220,
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
    r = 220,
    g = 220,
    b = 230,
    a = 255,
  })

  font_api.print({
    font = font,
    text = string.format("Wave: %d", wave),
    x = 20,
    y = 40,
    size = 18,
    r = 200,
    g = 210,
    b = 230,
    a = 220,
  })

  local ammo_text = string.format("Ammo: %d / %d / %d", bases[1].ammo, bases[2].ammo, bases[3].ammo)
  font_api.print({
    font = font,
    text = ammo_text,
    x = screen_w - 250,
    y = 18,
    size = 18,
    r = 210,
    g = 210,
    b = 220,
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
  leo.mouse.setCursorVisible(false)
  reset_game()
end

function leo.update(dt, input)
  mouse_state = input.mouse
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
  draw_background()
  draw_cities()
  draw_bases()
  draw_missiles()
  draw_explosions()
  if mouse_state then
    draw_crosshair(mouse_state)
  end
  draw_ui()
end

function leo.shutdown()
  leo.mouse.setCursorVisible(true)
end
