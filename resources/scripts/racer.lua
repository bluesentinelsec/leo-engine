-- Leo Engine pseudo-3D racer demo (fixed 1280x720 virtual space).

local graphics = leo.graphics
local font_api = leo.font
local window = leo.window

local VIRTUAL_W = 1280
local VIRTUAL_H = 720

local font = nil
local base_size = 20
local line_height = 0

local track = {
  { length = 240, curve = 0.0, label = "straight" },
  { length = 220, curve = 0.55, label = "curve right" },
  { length = 200, curve = 0.0, label = "straight" },
  { length = 220, curve = -0.6, label = "curve left" },
  { length = 260, curve = 0.0, label = "straight" },
  { length = 200, curve = -0.45, label = "curve left" },
  { length = 220, curve = 0.0, label = "straight" },
  { length = 220, curve = 0.5, label = "curve right" },
}

local segment_index = 1
local segment_pos = 0
local curve_current = 0
local curve_target = 0
local curve_label = "straight"

local speed = 160
local min_speed = 60
local max_speed = 240
local accel = 160
local brake = 240
local coast = 90

local player_offset = 0
local track_distance = 0

local function clamp(value, min_value, max_value)
  if value < min_value then
    return min_value
  end
  if value > max_value then
    return max_value
  end
  return value
end

local function draw_rect_filled(x, y, w, h, r, g, b, a)
  graphics.drawRectangleFilled({
    x = x,
    y = y,
    w = w,
    h = h,
    r = r,
    g = g,
    b = b,
    a = a,
  })
end

local function draw_rect_outline(x, y, w, h, r, g, b, a)
  graphics.drawRectangleOutline({
    x = x,
    y = y,
    w = w,
    h = h,
    r = r,
    g = g,
    b = b,
    a = a,
  })
end

local function print_text(text, x, y, size, r, g, b, a)
  font_api.print({
    font = font,
    text = text,
    x = x,
    y = y,
    size = size,
    r = r,
    g = g,
    b = b,
    a = a,
  })
end

local function advance_for(size)
  return line_height * (size / base_size)
end

local function update_track(dt)
  local seg = track[segment_index]
  segment_pos = segment_pos + speed * dt
  track_distance = track_distance + speed * dt
  if track_distance > 100000 then
    track_distance = track_distance - 100000
  end

  while segment_pos >= seg.length do
    segment_pos = segment_pos - seg.length
    segment_index = segment_index + 1
    if segment_index > #track then
      segment_index = 1
    end
    seg = track[segment_index]
    curve_target = seg.curve
    curve_label = seg.label
  end

  local blend_rate = 1.6
  if curve_target == 0.0 then
    blend_rate = 4.0
  end
  local blend = dt * blend_rate
  if blend > 1 then
    blend = 1
  end
  curve_current = curve_current + (curve_target - curve_current) * blend
  if curve_target == 0.0 and math.abs(curve_current) < 0.001 then
    curve_current = 0.0
  end
end

local function update_speed(dt, keyboard)
  if not keyboard then
    return
  end

  local accelerating = keyboard:isDown("up")
  local braking = keyboard:isDown("down")

  if accelerating then
    speed = speed + accel * dt
  elseif braking then
    speed = speed - brake * dt
  else
    speed = speed - coast * dt
  end

  speed = clamp(speed, min_speed, max_speed)
end

local function update_steering(dt, keyboard)
  if not keyboard then
    return
  end

  local steer = 0
  if keyboard:isDown("left") then
    steer = steer - 1
  end
  if keyboard:isDown("right") then
    steer = steer + 1
  end

  local steer_speed = 1.2
  player_offset = player_offset + steer * steer_speed * dt

  local drift = curve_current * (speed / max_speed) * 0.45
  player_offset = player_offset - drift * dt

  player_offset = clamp(player_offset, -1.25, 1.25)
end

local function draw_road()
  local horizon = math.floor(VIRTUAL_H * 0.32)
  local segments = 120
  local max_road_w = VIRTUAL_W * 0.92
  local min_road_w = VIRTUAL_W * 0.08
  local curve_scale = VIRTUAL_W * 0.4
  local stripe_length = 14
  local stripe_offset = math.floor(track_distance / stripe_length)

  local curve_visual = curve_current
  if curve_target == 0.0 then
    curve_visual = 0.0
  end

  for i = 0, segments - 1 do
    local t1 = (i + 1) / segments

    local y0 = horizon + (i / segments) * (VIRTUAL_H - horizon)
    local y1 = horizon + t1 * (VIRTUAL_H - horizon)
    local row_h = y1 - y0 + 1

    local w1 = min_road_w + (max_road_w - min_road_w) * t1 * t1

    local offset1 = curve_visual * curve_scale * (1 - t1) * (1 - t1)

    local center_x = VIRTUAL_W * 0.5 + offset1
    local road_x = center_x - w1 * 0.5

    local stripe = ((stripe_offset + i) % 2 == 0)
    local grass_r = stripe and 36 or 28
    local grass_g = stripe and 140 or 120
    local grass_b = stripe and 70 or 60

    draw_rect_filled(0, y0, VIRTUAL_W, row_h, grass_r, grass_g, grass_b, 255)

    local rumble_w = w1 * 0.08
    local rumble_r = stripe and 220 or 235
    local rumble_g = stripe and 60 or 235
    local rumble_b = stripe and 60 or 235

    draw_rect_filled(road_x - rumble_w, y0, rumble_w, row_h, rumble_r, rumble_g, rumble_b, 255)
    draw_rect_filled(road_x + w1, y0, rumble_w, row_h, rumble_r, rumble_g, rumble_b, 255)
    draw_rect_filled(road_x, y0, w1, row_h, 48, 48, 52, 255)

    if (stripe_offset + i) % 3 == 0 then
      local lane_count = 3
      local lane_w = w1 / lane_count
      local line_w = w1 * 0.02
      for lane = 1, lane_count - 1 do
        local lx = road_x + lane_w * lane - line_w * 0.5
        draw_rect_filled(lx, y0, line_w, row_h, 220, 220, 220, 210)
      end
    end
  end
end

local function draw_car()
  local car_w = VIRTUAL_W * 0.09
  local car_h = VIRTUAL_H * 0.07
  local car_y = VIRTUAL_H * 0.78
  local road_space = VIRTUAL_W * 0.26
  local car_x = VIRTUAL_W * 0.5 + player_offset * road_space - car_w * 0.5
  local wheel_w = car_w * 0.22
  local wheel_h = car_h * 0.3
  local wheel_y = car_y + car_h - wheel_h * 0.4

  draw_rect_filled(car_x + 8, car_y + 10, car_w, car_h, 20, 20, 24, 140)

  draw_rect_filled(car_x, car_y, car_w, car_h, 240, 70, 60, 255)

  draw_rect_filled(car_x - wheel_w * 0.3, wheel_y, wheel_w, wheel_h, 30, 30, 36, 240)
  draw_rect_filled(car_x + car_w - wheel_w * 0.7, wheel_y, wheel_w, wheel_h, 30, 30, 36, 240)

  draw_rect_filled(car_x + car_w * 0.2, car_y + car_h * 0.2, car_w * 0.6, car_h * 0.35, 255, 220, 180, 220)

  draw_rect_filled(car_x + car_w * 0.15, car_y + car_h * 0.72, car_w * 0.7, car_h * 0.12, 255, 230, 90, 220)

  draw_rect_outline(car_x, car_y, car_w, car_h, 140, 40, 40, 240)
end

local function draw_hud()
  local x = 24
  local y = 20

  print_text("Pseudo 3D Racer", x, y, 22, 230, 230, 240, 255)
  y = y + advance_for(22) + 4

  print_text(string.format("speed: %d", math.floor(speed + 0.5)), x, y, 18, 190, 210, 220, 255)
  y = y + advance_for(18)

  print_text(string.format("track: %s", curve_label), x, y, 18, 190, 210, 220, 255)

  print_text("controls: arrows to steer, up/down to speed", x, VIRTUAL_H - 28, 16, 150, 170, 190, 255)
end

function leo.load()
  font = font_api.new("resources/fonts/font.ttf", base_size)
  line_height = font:getLineHeight()
  curve_target = track[segment_index].curve
  curve_label = track[segment_index].label
end

function leo.update(dt, input)
  update_speed(dt, input.keyboard)
  update_steering(dt, input.keyboard)
  update_track(dt)
end

function leo.draw()
  graphics.clear(10, 12, 22, 255)
  draw_road()
  draw_car()
  draw_hud()
end

function leo.shutdown()
end
