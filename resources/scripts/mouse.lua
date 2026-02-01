-- Leo Engine mouse input demo.

local graphics = leo.graphics

local mouse_state = nil

local buttons = {
  { name = "left", color = { 230, 90, 90 } },
  { name = "middle", color = { 230, 200, 90 } },
  { name = "right", color = { 90, 200, 240 } },
  { name = "x1", color = { 170, 110, 255 } },
  { name = "x2", color = { 120, 230, 160 } },
}

local button_fx = {}
local wheel_fx = { x = 0, y = 0 }

local function ensure_button(name)
  if not button_fx[name] then
    button_fx[name] = { press = 0, release = 0 }
  end
  return button_fx[name]
end

local function update_button_fx(dt, mouse)
  for _, btn in ipairs(buttons) do
    local state = ensure_button(btn.name)
    if mouse:isPressed(btn.name) then
      state.press = 0.18
    end
    if mouse:isReleased(btn.name) then
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

local function draw_button(btn, index, screen_w, screen_h, mouse)
  local state = ensure_button(btn.name)
  local is_down = mouse:isDown(btn.name)
  local spacing = 78
  local x = 60 + (index - 1) * spacing
  local y = screen_h - 120
  local w = 62
  local h = 62

  graphics.drawRectangleRoundedFilled({
    x = x,
    y = y,
    w = w,
    h = h,
    radius = 10,
    r = 26,
    g = 30,
    b = 40,
    a = 255,
  })

  local br, bg, bb = btn.color[1], btn.color[2], btn.color[3]
  if is_down then
    graphics.drawRectangleRoundedFilled({
      x = x + 4,
      y = y + 4,
      w = w - 8,
      h = h - 8,
      radius = 8,
      r = br,
      g = bg,
      b = bb,
      a = 220,
    })
  end

  if state.press > 0 then
    graphics.drawCircleFilled({
      x = x + 12,
      y = y + 12,
      radius = 6,
      r = 255,
      g = 210,
      b = 120,
      a = 230,
    })
  end

  if state.release > 0 then
    graphics.drawCircleFilled({
      x = x + w - 12,
      y = y + 12,
      radius = 6,
      r = 255,
      g = 90,
      b = 110,
      a = 230,
    })
  end

  graphics.drawRectangleRoundedOutline({
    x = x,
    y = y,
    w = w,
    h = h,
    radius = 10,
    r = 120,
    g = 140,
    b = 170,
    a = 220,
  })
end

function leo.load()
  button_fx = {}
  wheel_fx = { x = 0, y = 0 }
  leo.mouse.setCursorVisible(false)
end

function leo.update(dt, input)
  mouse_state = input.mouse
  update_button_fx(dt, input.mouse)

  wheel_fx.x = wheel_fx.x + input.mouse.wheelX * 18
  wheel_fx.y = wheel_fx.y + input.mouse.wheelY * 18
  wheel_fx.x = wheel_fx.x * 0.86
  wheel_fx.y = wheel_fx.y * 0.86
end

function leo.draw()
  graphics.clear(18, 18, 22, 255)

  if not mouse_state then
    return
  end

  local screen_w, screen_h = graphics.getSize()
  local mx = mouse_state.x
  local my = mouse_state.y

  graphics.drawLine({
    x1 = mx - 20,
    y1 = my,
    x2 = mx + 20,
    y2 = my,
    r = 200,
    g = 220,
    b = 240,
    a = 220,
  })
  graphics.drawLine({
    x1 = mx,
    y1 = my - 20,
    x2 = mx,
    y2 = my + 20,
    r = 200,
    g = 220,
    b = 240,
    a = 220,
  })

  graphics.drawCircleOutline({
    x = mx,
    y = my,
    radius = 14,
    r = 150,
    g = 180,
    b = 220,
    a = 200,
  })

  local dx = mouse_state.dx
  local dy = mouse_state.dy
  graphics.drawLine({
    x1 = mx,
    y1 = my,
    x2 = mx + dx * 6,
    y2 = my + dy * 6,
    r = 120,
    g = 240,
    b = 200,
    a = 200,
  })

  local center_x = screen_w * 0.5
  local center_y = screen_h - 200

  graphics.drawLine({
    x1 = center_x - 120,
    y1 = center_y,
    x2 = center_x + 120,
    y2 = center_y,
    r = 70,
    g = 90,
    b = 120,
    a = 180,
  })
  graphics.drawLine({
    x1 = center_x,
    y1 = center_y - 70,
    x2 = center_x,
    y2 = center_y + 70,
    r = 70,
    g = 90,
    b = 120,
    a = 180,
  })

  graphics.drawLine({
    x1 = center_x,
    y1 = center_y,
    x2 = center_x + wheel_fx.x,
    y2 = center_y,
    r = 180,
    g = 160,
    b = 255,
    a = 220,
  })
  graphics.drawLine({
    x1 = center_x,
    y1 = center_y,
    x2 = center_x,
    y2 = center_y - wheel_fx.y,
    r = 255,
    g = 190,
    b = 140,
    a = 220,
  })

  for index, btn in ipairs(buttons) do
    draw_button(btn, index, screen_w, screen_h, mouse_state)
  end
end

function leo.shutdown()
  leo.mouse.setCursorVisible(true)
end
