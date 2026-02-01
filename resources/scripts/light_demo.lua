-- Leo Engine 2D lighting demo (simple).

local graphics = leo.graphics
local font_api = leo.font
local camera_api = leo.camera
local math_api = leo.math

local screen_w = 1280
local screen_h = 720
local world_w = 3000
local world_h = 720

local font = nil
local cam = nil

local floor_y = 520
local pulse_time = 0
local player = {
  x = 120,
  y = 0,
  w = 32,
  h = 44,
  speed = 220,
}

local lights = {
  { x = 360, y = 140, radius = 320 },
  { x = 1180, y = 140, radius = 320, pulse = true },
  { x = 2000, y = 140, radius = 320 },
  { x = 2800, y = 140, radius = 320 },
}

local ambient = 0.12

local particles = {}
local particle_count = 320
local particle_clusters = {
  { x = 520, y = 220, radius = 220, strength = 1.2 },
  { x = 1480, y = 180, radius = 260, strength = 1.0 },
  { x = 2320, y = 260, radius = 240, strength = 1.4 },
}

local function light_intensity_at(light, x, y)
  local dx = x - light.x
  local dist = math.abs(dx)
  local radius = light.radius
  if light.pulse then
    local pulse = 0.78 + 0.22 * (0.5 + 0.5 * math.sin(pulse_time * 2.2))
    radius = radius * pulse
  end
  local t = 1 - math_api.clamp(dist / radius, 0, 1)
  return t * t
end

local function total_light_at(x, y)
  local sum = ambient
  for _, light in ipairs(lights) do
    sum = sum + light_intensity_at(light, x, y) * 0.9
  end
  return math_api.clamp(sum, 0, 1)
end

local function init_particles()
  particles = {}
  for i = 1, particle_count do
    particles[#particles + 1] = {
      x = math.random() * world_w,
      y = math.random() * (floor_y - 40) + 20,
      vx = (math.random() * 2 - 1) * 12,
      vy = (math.random() * 2 - 1) * 8,
      radius = 1.5 + math.random() * 3.5,
      alpha = 70 + math.random() * 120,
      hue = math.random(),
    }
  end
end

local function update_particles(dt)
  for _, p in ipairs(particles) do
    local drift_x = 0
    local drift_y = 0

    for _, cluster in ipairs(particle_clusters) do
      local dx = p.x - cluster.x
      local dy = p.y - cluster.y
      local dist = math.sqrt(dx * dx + dy * dy)
      if dist < cluster.radius then
        local t = 1 - (dist / cluster.radius)
        local swirl = cluster.strength * t
        drift_x = drift_x + (-dy / (dist + 1)) * 12 * swirl
        drift_y = drift_y + (dx / (dist + 1)) * 12 * swirl
      end
    end

    p.x = p.x + (p.vx + drift_x) * dt
    p.y = p.y + (p.vy + drift_y) * dt

    if p.x < -30 then
      p.x = world_w + 30
    elseif p.x > world_w + 30 then
      p.x = -30
    end

    if p.y < 10 then
      p.y = floor_y - 40
    elseif p.y > floor_y - 20 then
      p.y = 20
    end
  end
end

local function draw_particles()
  for _, p in ipairs(particles) do
    local r = 80
    local g = 220
    local b = 120
    if p.hue < 0.2 then
      r, g, b = 240, 90, 90
    elseif p.hue < 0.45 then
      r, g, b = 240, 200, 80
    end

    graphics.drawCircleFilled({
      x = p.x,
      y = p.y,
      radius = p.radius,
      r = r,
      g = g,
      b = b,
      a = p.alpha,
    })
  end
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
    if math.abs(axis) > 0.2 then
      move = move + axis
    end
    if pad:isDown("dpadleft") then
      move = move - 1
    elseif pad:isDown("dpadright") then
      move = move + 1
    end
  end

  player.x = player.x + move * player.speed * dt
  player.x = math_api.clamp(player.x, 40, world_w - 40)
  player.y = floor_y - player.h
end

local function draw_light_pool(light)
  local steps = 5
  local radius = light.radius
  if light.pulse then
    local pulse = 0.78 + 0.22 * (0.5 + 0.5 * math.sin(pulse_time * 2.2))
    radius = radius * pulse
  end
  for i = steps, 1, -1 do
    local t = i / steps
    graphics.drawCircleFilled({
      x = light.x,
      y = floor_y,
      radius = radius * t,
      r = 120 + math.floor(80 * t),
      g = 110 + math.floor(70 * t),
      b = 70 + math.floor(40 * t),
      a = 18 + math.floor(32 * t),
    })
  end

  graphics.drawLine({
    x1 = light.x,
    y1 = light.y + 16,
    x2 = light.x,
    y2 = floor_y - 16,
    r = 120,
    g = 120,
    b = 140,
    a = 40,
  })

  graphics.drawCircleFilled({
    x = light.x,
    y = light.y,
    radius = 10,
    r = 220,
    g = 210,
    b = 180,
    a = 255,
  })
end

local function draw_background()
  graphics.clear(12, 12, 18, 255)

  graphics.drawRectangleFilled({
    x = 0,
    y = floor_y,
    w = world_w,
    h = screen_h - floor_y,
    r = 26,
    g = 28,
    b = 34,
    a = 255,
  })

  graphics.drawRectangleFilled({
    x = 0,
    y = 0,
    w = world_w,
    h = floor_y,
    r = 16,
    g = 18,
    b = 24,
    a = 255,
  })

  for _, light in ipairs(lights) do
    draw_light_pool(light)
  end

  graphics.drawRectangleFilled({
    x = 0,
    y = floor_y,
    w = world_w,
    h = 6,
    r = 60,
    g = 50,
    b = 40,
    a = 220,
  })
end

local function draw_player()
  local center_x = player.x + player.w * 0.5
  local center_y = player.y + player.h * 0.5
  local brightness = total_light_at(center_x, center_y)

  local base_r, base_g, base_b = 90, 170, 220
  local r = math.floor(base_r * brightness)
  local g = math.floor(base_g * brightness)
  local b = math.floor(base_b * brightness)
  local min = 10
  if r < min then r = min end
  if g < min then g = min end
  if b < min then b = min end

  graphics.drawRectangleFilled({
    x = player.x,
    y = player.y,
    w = player.w,
    h = player.h,
    r = r,
    g = g,
    b = b,
    a = 255,
  })

  graphics.drawRectangleFilled({
    x = player.x + 6,
    y = player.y + 8,
    w = 6,
    h = 6,
    r = 20,
    g = 20,
    b = 24,
    a = 200,
  })
  graphics.drawRectangleFilled({
    x = player.x + player.w - 12,
    y = player.y + 8,
    w = 6,
    h = 6,
    r = 20,
    g = 20,
    b = 24,
    a = 200,
  })
end

local function draw_ui()
  if not font then
    return
  end

  local center_x = player.x + player.w * 0.5
  local center_y = player.y + player.h * 0.5
  local brightness = total_light_at(center_x, center_y)

  font_api.print({
    font = font,
    text = "Move: Left/Right or A/D",
    x = 20,
    y = 18,
    size = 18,
    r = 210,
    g = 210,
    b = 220,
    a = 220,
  })

  font_api.print({
    font = font,
    text = string.format("Light: %.2f", brightness),
    x = 20,
    y = 40,
    size = 18,
    r = 210,
    g = 210,
    b = 220,
    a = 220,
  })
end

function leo.load()
  local sw, sh = graphics.getSize()
  screen_w = sw
  screen_h = sh
  floor_y = math.floor(screen_h * 0.72)
  world_h = screen_h

  font = font_api.new("resources/fonts/font.ttf", 18)
  player.y = floor_y - player.h
  init_particles()

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
end

function leo.update(dt, input)
  update_player(dt, input)
  pulse_time = pulse_time + dt
  update_particles(dt)
  cam:update(dt, {
    target = { x = player.x, y = player.y },
  })
end

function leo.draw()
  graphics.beginCamera(cam)
  draw_background()
  draw_particles()
  draw_player()
  graphics.endCamera()
  draw_ui()
end

function leo.shutdown()
end
