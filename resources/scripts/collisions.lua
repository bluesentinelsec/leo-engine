-- Leo Engine collision demo (named arguments).

local graphics = leo.graphics
local collision = leo.collision

local t = 0
local mouse_state = nil

local layout = {
  left = 40,
  top = 40,
  col_w = 560,
  row_h = 120,
  col_gap = 40,
  row_gap = 20,
}

local function cell_origin(row, col)
  local x = layout.left + (col - 1) * (layout.col_w + layout.col_gap)
  local y = layout.top + (row - 1) * (layout.row_h + layout.row_gap)
  return x, y
end

local function draw_cell_outline(x, y)
  graphics.drawRectangleOutline({
    x = x,
    y = y,
    w = layout.col_w,
    h = layout.row_h,
    r = 40,
    g = 50,
    b = 70,
    a = 120,
  })
end

local function draw_indicator(x, y, hit)
  local r, g, b = 220, 90, 90
  if hit then
    r, g, b = 90, 200, 120
  end
  graphics.drawCircleFilled({
    x = x + 12,
    y = y + 12,
    radius = 6,
    r = r,
    g = g,
    b = b,
    a = 220,
  })
end

local function draw_point(px, py, hit)
  local r, g, b = 220, 120, 120
  if hit then
    r, g, b = 120, 220, 160
  end
  graphics.drawCircleFilled({
    x = px,
    y = py,
    radius = 5,
    r = r,
    g = g,
    b = b,
    a = 240,
  })
end

function leo.load()
  t = 0
end

function leo.update(dt, input)
  t = t + dt
  mouse_state = input.mouse
end

function leo.draw()
  graphics.clear(18, 18, 22, 255)

  if not mouse_state then
    return
  end

  local mx = mouse_state.x
  local my = mouse_state.y

  -- Row 1, Col 1: rect vs rect
  do
    local x, y = cell_origin(1, 1)
    draw_cell_outline(x, y)

    local rect_a = { x = x + 40, y = y + 30, w = 140, h = 60 }
    local rect_b = { x = x + 260 + math.sin(t * 0.9) * 120, y = y + 30, w = 140, h = 60 }
    local hit = collision.checkRecs({ a = rect_a, b = rect_b })

    graphics.drawRectangleOutline({
      x = rect_a.x,
      y = rect_a.y,
      w = rect_a.w,
      h = rect_a.h,
      r = 120,
      g = 140,
      b = 170,
      a = 220,
    })

    local r, g, b = 90, 140, 220
    if hit then
      r, g, b = 90, 200, 120
    end
    graphics.drawRectangleFilled({
      x = rect_b.x,
      y = rect_b.y,
      w = rect_b.w,
      h = rect_b.h,
      r = r,
      g = g,
      b = b,
      a = 200,
    })

    draw_indicator(x, y, hit)
  end

  -- Row 1, Col 2: circle vs circle
  do
    local x, y = cell_origin(1, 2)
    draw_cell_outline(x, y)

    local c1 = { x = x + 140, y = y + 60 }
    local c2 = { x = x + 320 + math.sin(t * 0.8) * 110, y = y + 60 }
    local r1, r2 = 40, 40
    local hit = collision.checkCircles({ c1 = c1, r1 = r1, c2 = c2, r2 = r2 })

    graphics.drawCircleOutline({
      x = c1.x,
      y = c1.y,
      radius = r1,
      r = 120,
      g = 140,
      b = 170,
      a = 220,
    })

    local r, g, b = 90, 140, 220
    if hit then
      r, g, b = 90, 200, 120
    end
    graphics.drawCircleFilled({
      x = c2.x,
      y = c2.y,
      radius = r2,
      r = r,
      g = g,
      b = b,
      a = 200,
    })

    draw_indicator(x, y, hit)
  end

  -- Row 2, Col 1: circle vs rect
  do
    local x, y = cell_origin(2, 1)
    draw_cell_outline(x, y)

    local rect = { x = x + 40, y = y + 26, w = 180, h = 70 }
    local center = { x = x + 340 + math.sin(t * 1.1) * 120, y = y + 60 }
    local radius = 30
    local hit = collision.checkCircleRec({ center = center, radius = radius, rect = rect })

    graphics.drawRectangleOutline({
      x = rect.x,
      y = rect.y,
      w = rect.w,
      h = rect.h,
      r = 120,
      g = 140,
      b = 170,
      a = 220,
    })

    local r, g, b = 140, 90, 220
    if hit then
      r, g, b = 90, 200, 120
    end
    graphics.drawCircleFilled({
      x = center.x,
      y = center.y,
      radius = radius,
      r = r,
      g = g,
      b = b,
      a = 200,
    })

    draw_indicator(x, y, hit)
  end

  -- Row 2, Col 2: circle vs line
  do
    local x, y = cell_origin(2, 2)
    draw_cell_outline(x, y)

    local p1 = { x = x + 40, y = y + 40 }
    local p2 = { x = x + 480, y = y + 80 }
    local center = { x = x + 260 + math.sin(t * 1.0) * 140, y = y + 40 + math.cos(t * 0.7) * 30 }
    local radius = 24
    local hit = collision.checkCircleLine({ center = center, radius = radius, p1 = p1, p2 = p2 })

    graphics.drawLine({
      x1 = p1.x,
      y1 = p1.y,
      x2 = p2.x,
      y2 = p2.y,
      r = 120,
      g = 140,
      b = 170,
      a = 220,
    })

    local r, g, b = 220, 140, 90
    if hit then
      r, g, b = 90, 200, 120
    end
    graphics.drawCircleOutline({
      x = center.x,
      y = center.y,
      radius = radius,
      r = r,
      g = g,
      b = b,
      a = 220,
    })

    draw_indicator(x, y, hit)
  end

  -- Row 3, Col 1: line vs line
  do
    local x, y = cell_origin(3, 1)
    draw_cell_outline(x, y)

    local p1 = { x = x + 40, y = y + 30 }
    local p2 = { x = x + 480, y = y + 90 }
    local cx = x + 260
    local cy = y + 60
    local angle = t * 1.2
    local len = 200
    local dx = math.cos(angle) * len * 0.5
    local dy = math.sin(angle) * len * 0.5
    local p3 = { x = cx - dx, y = cy - dy }
    local p4 = { x = cx + dx, y = cy + dy }

    local hit = collision.checkLines({ p1 = p1, p2 = p2, p3 = p3, p4 = p4 })

    graphics.drawLine({
      x1 = p1.x,
      y1 = p1.y,
      x2 = p2.x,
      y2 = p2.y,
      r = 120,
      g = 140,
      b = 170,
      a = 220,
    })

    local r, g, b = 140, 200, 230
    if hit then
      r, g, b = 90, 200, 120
    end
    graphics.drawLine({
      x1 = p3.x,
      y1 = p3.y,
      x2 = p4.x,
      y2 = p4.y,
      r = r,
      g = g,
      b = b,
      a = 220,
    })

    draw_indicator(x, y, hit)
  end

  -- Row 3, Col 2: point vs rect
  do
    local x, y = cell_origin(3, 2)
    draw_cell_outline(x, y)

    local rect = { x = x + 120, y = y + 24, w = 220, h = 70 }
    local point = { x = mx, y = my }
    local hit = collision.checkPointRec({ point = point, rect = rect })

    graphics.drawRectangleOutline({
      x = rect.x,
      y = rect.y,
      w = rect.w,
      h = rect.h,
      r = 120,
      g = 140,
      b = 170,
      a = 220,
    })

    draw_point(point.x, point.y, hit)
    draw_indicator(x, y, hit)
  end

  -- Row 4, Col 1: point vs circle
  do
    local x, y = cell_origin(4, 1)
    draw_cell_outline(x, y)

    local center = { x = x + 220, y = y + 60 }
    local radius = 46
    local point = { x = mx, y = my }
    local hit = collision.checkPointCircle({ point = point, center = center, radius = radius })

    graphics.drawCircleOutline({
      x = center.x,
      y = center.y,
      radius = radius,
      r = 120,
      g = 140,
      b = 170,
      a = 220,
    })

    draw_point(point.x, point.y, hit)
    draw_indicator(x, y, hit)
  end

  -- Row 4, Col 2: point vs triangle
  do
    local x, y = cell_origin(4, 2)
    draw_cell_outline(x, y)

    local p1 = { x = x + 140, y = y + 94 }
    local p2 = { x = x + 280, y = y + 26 }
    local p3 = { x = x + 420, y = y + 94 }
    local point = { x = mx, y = my }
    local hit = collision.checkPointTriangle({ point = point, p1 = p1, p2 = p2, p3 = p3 })

    graphics.drawTriangleOutline({
      x1 = p1.x,
      y1 = p1.y,
      x2 = p2.x,
      y2 = p2.y,
      x3 = p3.x,
      y3 = p3.y,
      r = 120,
      g = 140,
      b = 170,
      a = 220,
    })

    draw_point(point.x, point.y, hit)
    draw_indicator(x, y, hit)
  end

  -- Row 5, Col 1: point vs line (animated point)
  do
    local x, y = cell_origin(5, 1)
    draw_cell_outline(x, y)

    local p1 = { x = x + 60, y = y + 30 }
    local p2 = { x = x + 480, y = y + 90 }
    local line_t = (math.sin(t * 1.4) + 1) * 0.5
    local on_line = math.sin(t * 1.4) > 0
    local dx = p2.x - p1.x
    local dy = p2.y - p1.y
    local len = math.sqrt(dx * dx + dy * dy)
    local nx = 0
    local ny = 0
    if len > 0 then
      nx = -dy / len
      ny = dx / len
    end
    local offset = on_line and 0 or 12
    local point = {
      x = p1.x + dx * line_t + nx * offset,
      y = p1.y + dy * line_t + ny * offset,
    }
    local hit = collision.checkPointLine({ point = point, p1 = p1, p2 = p2 })

    graphics.drawLine({
      x1 = p1.x,
      y1 = p1.y,
      x2 = p2.x,
      y2 = p2.y,
      r = 120,
      g = 140,
      b = 170,
      a = 220,
    })

    draw_point(point.x, point.y, hit)
    draw_indicator(x, y, hit)
  end

  -- Row 5, Col 2: point vs polygon
  do
    local x, y = cell_origin(5, 2)
    draw_cell_outline(x, y)

    local poly = {
      x + 140, y + 90,
      x + 200, y + 30,
      x + 320, y + 24,
      x + 420, y + 80,
      x + 300, y + 96,
      x + 200, y + 82,
    }
    local point = { x = mx, y = my }
    local hit = collision.checkPointPoly({ point = point, points = poly })

    graphics.drawPolyOutline({
      points = poly,
      r = 120,
      g = 140,
      b = 170,
      a = 220,
    })

    draw_point(point.x, point.y, hit)
    draw_indicator(x, y, hit)
  end
end

function leo.shutdown()
end
