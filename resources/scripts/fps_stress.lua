-- Leo Engine FPS stress test (sprite animation spam).

local graphics = leo.graphics
local animation_api = leo.animation
local font_api = leo.font
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local world_w = 1280
local world_h = 720

local font = nil

local sprites = {}
local max_sprites = nil
local spawn_rate = 0.002
local spawn_accum = 0
local delete_rate = 0.004
local delete_accum = 0

local fps_last_time = 0
local fps_frames = 0
local fps_value = 0
local mouse_state = nil

local function rand_range(min, max)
  return min + (max - min) * math.random()
end

local function spawn_sprite(x, y)
  local anim = animation_api.newSheet({
    path = "resources/images/animation_test.png",
    frame_w = 64,
    frame_h = 64,
    frame_count = 3,
    frame_time = rand_range(0.08, 0.18),
    looping = true,
    playing = true,
  })

  local tint_choice = math.random()
  local r, g, b = 255, 255, 255
  if tint_choice < 0.33 then
    r, g, b = 255, math.random(80, 140), math.random(80, 140)
  elseif tint_choice < 0.66 then
    r, g, b = math.random(80, 140), 255, math.random(80, 140)
  else
    r, g, b = math.random(80, 140), math.random(80, 140), 255
  end

  local sprite = {
    anim = anim,
    x = math_api.clamp(x, 40, world_w - 40),
    y = math_api.clamp(y, 40, world_h - 40),
    ox = 32,
    oy = 32,
    angle = rand_range(-1.5, 1.5),
    rot_speed = rand_range(-4.0, 4.0),
    vx = rand_range(-140, 140),
    vy = rand_range(-120, 120),
    scale = rand_range(0.5, 1.4),
    scale_target = rand_range(0.4, 1.8),
    transform_timer = rand_range(0.3, 1.1),
    r = r,
    g = g,
    b = b,
    a = 255,
  }

  sprite.anim:update(rand_range(0, 0.5))
  sprites[#sprites + 1] = sprite
end

local function update_sprites(dt)
  for _, sprite in ipairs(sprites) do
    sprite.anim:update(dt)
    sprite.angle = sprite.angle + sprite.rot_speed * dt
    sprite.x = sprite.x + sprite.vx * dt
    sprite.y = sprite.y + sprite.vy * dt

    if sprite.x < 0 then
      sprite.x = 0
      sprite.vx = math.abs(sprite.vx)
    elseif sprite.x > world_w then
      sprite.x = world_w
      sprite.vx = -math.abs(sprite.vx)
    end

    if sprite.y < 0 then
      sprite.y = 0
      sprite.vy = math.abs(sprite.vy)
    elseif sprite.y > world_h then
      sprite.y = world_h
      sprite.vy = -math.abs(sprite.vy)
    end

    sprite.transform_timer = sprite.transform_timer - dt
    if sprite.transform_timer <= 0 then
      sprite.transform_timer = rand_range(0.35, 1.2)
      sprite.scale_target = rand_range(0.35, 2.0)
      sprite.rot_speed = rand_range(-5.0, 5.0)
    end

    local lerp = math.min(1, dt * 4)
    sprite.scale = sprite.scale + (sprite.scale_target - sprite.scale) * lerp
  end
end

local function update_fps()
  local now = leo.time.now()
  if fps_last_time == 0 then
    fps_last_time = now
  end
  fps_frames = fps_frames + 1
  local elapsed = now - fps_last_time
  if elapsed >= 0.5 then
    fps_value = fps_frames / elapsed
    fps_last_time = now
    fps_frames = 0
  end
end

function leo.load()
  math.randomseed(os.time())
  font = font_api.new("resources/fonts/font.ttf", 18)
  leo.mouse.setCursorVisible(false)
end

function leo.update(dt, input)
  mouse_state = input.mouse
  if input.mouse:isDown("left") then
    spawn_accum = spawn_accum + dt
    while spawn_accum >= spawn_rate and (not max_sprites or #sprites < max_sprites) do
      spawn_accum = spawn_accum - spawn_rate
      local mx = input.mouse.x + rand_range(-24, 24)
      local my = input.mouse.y + rand_range(-24, 24)
      spawn_sprite(mx, my)
    end
  else
    spawn_accum = 0
  end

  if input.mouse:isDown("right") then
    delete_accum = delete_accum + dt
    while delete_accum >= delete_rate and #sprites > 0 do
      delete_accum = delete_accum - delete_rate
      sprites[#sprites] = nil
    end
  else
    delete_accum = 0
  end

  update_sprites(dt)
  update_fps()
end

function leo.draw()
  graphics.clear(18, 18, 24, 255)

  for _, sprite in ipairs(sprites) do
    sprite.anim:draw({
      x = sprite.x,
      y = sprite.y,
      ox = sprite.ox,
      oy = sprite.oy,
      sx = sprite.scale,
      sy = sprite.scale,
      angle = sprite.angle,
      flipX = false,
      flipY = false,
      r = sprite.r,
      g = sprite.g,
      b = sprite.b,
      a = sprite.a,
    })
  end

  if font then
    graphics.drawRectangleFilled({
      x = 10,
      y = 12,
      w = 320,
      h = 90,
      r = 0,
      g = 0,
      b = 0,
      a = 180,
    })
    font_api.print({
      font = font,
      text = string.format("FPS: %.1f", fps_value),
      x = 20,
      y = 18,
      size = 20,
      r = 230,
      g = 230,
      b = 240,
      a = 255,
    })
    font_api.print({
      font = font,
      text = string.format("Sprites: %d", #sprites),
      x = 20,
      y = 42,
      size = 18,
      r = 200,
      g = 210,
      b = 230,
      a = 220,
    })
    font_api.print({
      font = font,
      text = "Hold Left Mouse: spawn | Right: delete",
      x = 20,
      y = 66,
      size = 16,
      r = 180,
      g = 190,
      b = 210,
      a = 220,
    })
  end

  if mouse_state then
    local mx = mouse_state.x
    local my = mouse_state.y
    graphics.drawLine({
      x1 = mx - 10,
      y1 = my,
      x2 = mx + 10,
      y2 = my,
      r = 220,
      g = 230,
      b = 240,
      a = 220,
    })
    graphics.drawLine({
      x1 = mx,
      y1 = my - 10,
      x2 = mx,
      y2 = my + 10,
      r = 220,
      g = 230,
      b = 240,
      a = 220,
    })
    graphics.drawCircleOutline({
      x = mx,
      y = my,
      radius = 8,
      r = 160,
      g = 190,
      b = 220,
      a = 200,
    })
  end
end

function leo.shutdown()
  leo.mouse.setCursorVisible(true)
end
