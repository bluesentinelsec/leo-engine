-- Leo Engine gamepad input demo (single controller).

local graphics = leo.graphics
local math_api = leo.math

local gamepad_state = nil

local buttons = {
  { name = "south", x = 980, y = 260, w = 56, h = 56, color = { 120, 220, 120 } },
  { name = "east", x = 1048, y = 200, w = 56, h = 56, color = { 230, 120, 120 } },
  { name = "west", x = 912, y = 200, w = 56, h = 56, color = { 120, 170, 230 } },
  { name = "north", x = 980, y = 140, w = 56, h = 56, color = { 230, 220, 120 } },

  { name = "dpadup", x = 220, y = 200, w = 48, h = 48, color = { 180, 200, 220 } },
  { name = "dpaddown", x = 220, y = 292, w = 48, h = 48, color = { 180, 200, 220 } },
  { name = "dpadleft", x = 172, y = 246, w = 48, h = 48, color = { 180, 200, 220 } },
  { name = "dpadright", x = 268, y = 246, w = 48, h = 48, color = { 180, 200, 220 } },

  { name = "leftshoulder", x = 160, y = 90, w = 180, h = 30, color = { 160, 180, 220 } },
  { name = "rightshoulder", x = 860, y = 90, w = 180, h = 30, color = { 160, 180, 220 } },

  { name = "back", x = 570, y = 220, w = 60, h = 38, color = { 150, 160, 190 } },
  { name = "start", x = 650, y = 220, w = 60, h = 38, color = { 150, 160, 190 } },

  { name = "leftstick", x = 360, y = 460, w = 64, h = 64, color = { 150, 210, 180 } },
  { name = "rightstick", x = 820, y = 460, w = 64, h = 64, color = { 150, 210, 180 } },
}

local sticks = {
  { x = 360, y = 420, axis_x = "leftx", axis_y = "lefty" },
  { x = 820, y = 420, axis_x = "rightx", axis_y = "righty" },
}

local triggers = {
  { name = "lefttrigger", x = 160, y = 130, w = 180, h = 18, color = { 160, 200, 255 } },
  { name = "righttrigger", x = 860, y = 130, w = 180, h = 18, color = { 255, 180, 140 } },
}

local button_fx = {}
local trigger_fx = {
  lefttrigger = { press = 0, release = 0 },
  righttrigger = { press = 0, release = 0 },
}

local function ensure_button(name)
  if not button_fx[name] then
    button_fx[name] = { press = 0, release = 0 }
  end
  return button_fx[name]
end

local function update_fx(dt, pad)
  for _, btn in ipairs(buttons) do
    local state = ensure_button(btn.name)
    if pad:isPressed(btn.name) then
      state.press = 0.18
    end
    if pad:isReleased(btn.name) then
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

  for _, trg in ipairs(triggers) do
    local state = trigger_fx[trg.name]
    if pad:isAxisPressed(trg.name, 0.5, "positive") then
      state.press = 0.18
    end
    if pad:isAxisReleased(trg.name, 0.5, "positive") then
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

local function draw_button(btn, pad)
  local state = ensure_button(btn.name)
  local is_down = pad:isDown(btn.name)
  local br, bg, bb = btn.color[1], btn.color[2], btn.color[3]

  graphics.drawRectangleRoundedFilled({
    x = btn.x,
    y = btn.y,
    w = btn.w,
    h = btn.h,
    radius = 10,
    r = 26,
    g = 30,
    b = 40,
    a = 255,
  })

  if is_down then
    graphics.drawRectangleRoundedFilled({
      x = btn.x + 4,
      y = btn.y + 4,
      w = btn.w - 8,
      h = btn.h - 8,
      radius = 8,
      r = br,
      g = bg,
      b = bb,
      a = 220,
    })
  end

  if state.press > 0 then
    graphics.drawCircleFilled({
      x = btn.x + 12,
      y = btn.y + 12,
      radius = 6,
      r = 255,
      g = 210,
      b = 120,
      a = 230,
    })
  end

  if state.release > 0 then
    graphics.drawCircleFilled({
      x = btn.x + btn.w - 12,
      y = btn.y + 12,
      radius = 6,
      r = 255,
      g = 90,
      b = 110,
      a = 230,
    })
  end

  graphics.drawRectangleRoundedOutline({
    x = btn.x,
    y = btn.y,
    w = btn.w,
    h = btn.h,
    radius = 10,
    r = 120,
    g = 140,
    b = 170,
    a = 220,
  })
end

local function draw_stick(stick, pad)
  local ax = pad:axis(stick.axis_x)
  local ay = pad:axis(stick.axis_y)
  ax = math_api.clamp(ax, -1, 1)
  ay = math_api.clamp(ay, -1, 1)

  local radius = 54
  local knob = 16
  local offset = 30

  graphics.drawCircleOutline({
    x = stick.x,
    y = stick.y,
    radius = radius,
    r = 90,
    g = 110,
    b = 140,
    a = 200,
  })

  graphics.drawCircleFilled({
    x = stick.x + ax * offset,
    y = stick.y + ay * offset,
    radius = knob,
    r = 140,
    g = 200,
    b = 220,
    a = 220,
  })
end

local function draw_trigger(trg, pad)
  local value = pad:axis(trg.name)
  value = math_api.clamp(value, 0, 1)

  graphics.drawRectangleRoundedFilled({
    x = trg.x,
    y = trg.y,
    w = trg.w,
    h = trg.h,
    radius = 6,
    r = 26,
    g = 30,
    b = 40,
    a = 255,
  })

  if value > 0 then
    graphics.drawRectangleRoundedFilled({
      x = trg.x + 2,
      y = trg.y + 2,
      w = (trg.w - 4) * value,
      h = trg.h - 4,
      radius = 5,
      r = trg.color[1],
      g = trg.color[2],
      b = trg.color[3],
      a = 220,
    })
  end

  local state = trigger_fx[trg.name]
  if state.press > 0 then
    graphics.drawCircleFilled({
      x = trg.x + trg.w - 10,
      y = trg.y - 6,
      radius = 6,
      r = 255,
      g = 210,
      b = 120,
      a = 230,
    })
  end
  if state.release > 0 then
    graphics.drawCircleFilled({
      x = trg.x + 10,
      y = trg.y - 6,
      radius = 6,
      r = 255,
      g = 90,
      b = 110,
      a = 230,
    })
  end

  graphics.drawRectangleRoundedOutline({
    x = trg.x,
    y = trg.y,
    w = trg.w,
    h = trg.h,
    radius = 6,
    r = 120,
    g = 140,
    b = 170,
    a = 220,
  })
end

local function draw_connection(screen_w, screen_h, connected)
  local w = 280
  local h = 60
  local x = (screen_w - w) * 0.5
  local y = 30
  local r, g, b = 220, 90, 90
  if connected then
    r, g, b = 90, 200, 120
  end

  graphics.drawRectangleRoundedFilled({
    x = x,
    y = y,
    w = w,
    h = h,
    radius = 12,
    r = 26,
    g = 30,
    b = 40,
    a = 255,
  })

  graphics.drawRectangleRoundedOutline({
    x = x,
    y = y,
    w = w,
    h = h,
    radius = 12,
    r = r,
    g = g,
    b = b,
    a = 230,
  })

  if not connected then
    graphics.drawLine({
      x1 = x + 20,
      y1 = y + 20,
      x2 = x + w - 20,
      y2 = y + h - 20,
      r = r,
      g = g,
      b = b,
      a = 220,
    })
    graphics.drawLine({
      x1 = x + 20,
      y1 = y + h - 20,
      x2 = x + w - 20,
      y2 = y + 20,
      r = r,
      g = g,
      b = b,
      a = 220,
    })
  end
end

function leo.load()
  button_fx = {}
  trigger_fx.lefttrigger = { press = 0, release = 0 }
  trigger_fx.righttrigger = { press = 0, release = 0 }
end

function leo.update(dt, input)
  gamepad_state = input.gamepads[1]
  if gamepad_state and gamepad_state:isConnected() then
    update_fx(dt, gamepad_state)
  end
end

function leo.draw()
  graphics.clear(18, 18, 22, 255)

  if not gamepad_state then
    return
  end

  local screen_w, screen_h = graphics.getSize()
  local connected = gamepad_state:isConnected()
  draw_connection(screen_w, screen_h, connected)

  if not connected then
    return
  end

  for _, trg in ipairs(triggers) do
    draw_trigger(trg, gamepad_state)
  end

  for _, stick in ipairs(sticks) do
    draw_stick(stick, gamepad_state)
  end

  for _, btn in ipairs(buttons) do
    draw_button(btn, gamepad_state)
  end
end

function leo.shutdown()
end
