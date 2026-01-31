-- Leo Engine keyboard input demo.

local graphics = leo.graphics

local keys = {
  { name = "w", x = 140, y = 120, w = 72, h = 72 },
  { name = "a", x = 60, y = 200, w = 72, h = 72 },
  { name = "s", x = 140, y = 200, w = 72, h = 72 },
  { name = "d", x = 220, y = 200, w = 72, h = 72 },

  { name = "up", x = 860, y = 120, w = 72, h = 72 },
  { name = "left", x = 780, y = 200, w = 72, h = 72 },
  { name = "down", x = 860, y = 200, w = 72, h = 72 },
  { name = "right", x = 940, y = 200, w = 72, h = 72 },

  { name = "space", x = 260, y = 360, w = 520, h = 64 },
}

local key_fx = {}
local keyboard_state = nil

local function ensure_state(name)
  if not key_fx[name] then
    key_fx[name] = { press = 0, release = 0 }
  end
  return key_fx[name]
end

local function update_key_fx(dt, keyboard)
  for _, key in ipairs(keys) do
    local state = ensure_state(key.name)
    if keyboard:isPressed(key.name) then
      state.press = 0.18
    end
    if keyboard:isReleased(key.name) then
      state.release = 0.18
    end
    if state.press > 0 then
      state.press = state.press - dt
      if state.press < 0 then
        state.press = 0
      end
    end
    if state.release > 0 then
      state.release = state.release - dt
      if state.release < 0 then
        state.release = 0
      end
    end
  end
end

local function draw_key(key, keyboard)
  local state = ensure_state(key.name)
  local is_down = keyboard:isDown(key.name)

  graphics.drawRectangleRoundedFilled({
    x = key.x,
    y = key.y,
    w = key.w,
    h = key.h,
    radius = 10,
    r = 26,
    g = 30,
    b = 40,
    a = 255,
  })

  if is_down then
    graphics.drawRectangleRoundedFilled({
      x = key.x + 4,
      y = key.y + 4,
      w = key.w - 8,
      h = key.h - 8,
      radius = 8,
      r = 80,
      g = 190,
      b = 120,
      a = 220,
    })
  end

  if state.press > 0 then
    graphics.drawCircleFilled({
      x = key.x + 12,
      y = key.y + 12,
      radius = 6,
      r = 255,
      g = 210,
      b = 120,
      a = 230,
    })
  end

  if state.release > 0 then
    graphics.drawCircleFilled({
      x = key.x + key.w - 12,
      y = key.y + 12,
      radius = 6,
      r = 255,
      g = 90,
      b = 110,
      a = 230,
    })
  end

  graphics.drawRectangleRoundedOutline({
    x = key.x,
    y = key.y,
    w = key.w,
    h = key.h,
    radius = 10,
    r = 120,
    g = 140,
    b = 170,
    a = 220,
  })
end

function leo.load()
  key_fx = {}
end

function leo.update(dt, input)
  keyboard_state = input.keyboard
  update_key_fx(dt, input.keyboard)
end

function leo.draw()
  graphics.clear(18, 18, 22, 255)

  if not keyboard_state then
    return
  end

  for _, key in ipairs(keys) do
    draw_key(key, keyboard_state)
  end
end

function leo.shutdown()
end
