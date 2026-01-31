-- Leo Engine shape drawing demo.

local graphics = leo.graphics
local camera = leo.camera
local font_api = leo.font
local elapsed = 0
local vw = 1280
local vh = 720
local playfield_w = 3200
local playfield_h = 720
local scroll_cam = nil
local shape_cam = nil
local font = nil
local fps_timer = 0
local fps_frames = 0
local fps_value = 0
local last_dt = 0

local function clamp_color(value)
    if value < 0 then
        return 0
    end
    if value > 255 then
        return 255
    end
    return value
end

local function modulate_color(r, g, b, a, amp, freq, phase, alpha_amp, alpha_freq, alpha_phase)
    local r_m = 1.0 + amp * math.sin(elapsed * (freq * 0.9) + phase)
    local g_m = 1.0 + amp * math.sin(elapsed * (freq * 1.1) + phase + 2.1)
    local b_m = 1.0 + amp * math.sin(elapsed * (freq * 1.3) + phase + 4.2)
    local a_freq = alpha_freq or (freq * 0.8)
    local a_phase = alpha_phase or (phase + 1.7)
    local a_m = 1.0 + (alpha_amp or amp) * math.sin(elapsed * a_freq + a_phase)
    return clamp_color(r * r_m), clamp_color(g * g_m), clamp_color(b * b_m), clamp_color(a * a_m)
end

local function hash01(value)
    return (math.sin(value * 12.9898 + 78.233) * 43758.5453) % 1
end

local function draw_snow(width, height, count, x_offset)
    for i = 1, count do
        local seed = i * 31.7
        local x = hash01(seed) * width + (x_offset or 0)
        local speed = 20 + hash01(seed + 3.1) * 80
        local drift = (hash01(seed + 7.7) - 0.5) * 80
        local y = (elapsed * speed + hash01(seed + 5.9) * height) % height
        local px = x + math.sin(elapsed * 0.7 + seed) * drift
        local r = 200 + hash01(seed + 2.2) * 55
        local g = 200 + hash01(seed + 4.4) * 55
        local b = 220 + hash01(seed + 6.6) * 35
        local a = 140 + hash01(seed + 8.8) * 115
        graphics.drawPixel({
            x = px,
            y = y,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end
end

local function draw_backdrop(x0, y0, width, height, phase)
    local stripe_w = 80
    local t = elapsed * 0.5 + phase
    for i = 0, math.floor(width / stripe_w) do
        local x = x0 + i * stripe_w
        local r, g, b, a = modulate_color(30, 40, 70, 180, 0.35, 0.8, t + i * 0.3, 0.4)
        graphics.drawRectangleFilled({
            x = x,
            y = y0,
            w = stripe_w - 10,
            h = height,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end
end

local function wobble(base, amp, freq, phase)
    return base + math.sin(elapsed * freq + phase) * amp
end

local function pulse(base, amp, freq, phase)
    return base + math.sin(elapsed * freq + phase) * amp
end

local function spin(speed, phase)
    return elapsed * speed + phase
end

local function with_transform(sx, sy, rot, scale, draw_fn)
    shape_cam:setPosition(0, 0)
    shape_cam:setOffset(sx, sy)
    shape_cam:setRotation(rot)
    shape_cam:setZoom(scale)
    graphics.beginCamera(shape_cam)
    draw_fn()
    graphics.endCamera()
end

local function draw_shape_set(offset_x, offset_y, phase)
    -- Layout grid (1280x720) with offsets
    local cx1, cx2, cx3, cx4 = offset_x + 160, offset_x + 480, offset_x + 800, offset_x + 1120
    local cy1, cy2, cy3 = offset_y + 120, offset_y + 360, offset_y + 600

    -- Pixel (row 1, col 1)
    local px = wobble(cx1, 40, 1.9, 0.2 + phase)
    local py = wobble(cy1, 30, 1.3, 1.1 + phase)
    local spx, spy = scroll_cam:worldToScreen(px, py)
    with_transform(spx, spy, spin(1.6, 1.1 + phase), pulse(1.0, 0.45, 2.3, 0.4 + phase), function()
        local r, g, b, a = modulate_color(255, 64, 64, 255, 0.55, 2.4, 0.1 + phase, 0.7)
        graphics.drawPixel({
            x = 40,
            y = 0,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)

    -- Line (row 1, col 2)
    local lx = wobble(cx2, 55, 1.15, 0.6 + phase)
    local ly = wobble(cy1, 35, 0.95, 1.4 + phase)
    local slx, sly = scroll_cam:worldToScreen(lx, ly)
    with_transform(slx, sly, spin(-1.2, 0.4 + phase), pulse(1.0, 0.35, 1.9, 0.3 + phase), function()
        local r, g, b, a = modulate_color(255, 220, 0, 255, 0.55, 1.9, 0.7 + phase, 0.6)
        graphics.drawLine({
            x1 = -90,
            y1 = -20,
            x2 = 90,
            y2 = 20,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)

    -- Circle filled (row 1, col 3)
    local cfx = wobble(cx3, 45, 0.85, 0.9 + phase)
    local cfy = wobble(cy1, 28, 1.45, 0.1 + phase)
    local scfx, scfy = scroll_cam:worldToScreen(cfx, cfy)
    with_transform(scfx, scfy, spin(0.95, 2.0 + phase), pulse(1.0, 0.5, 1.6, 0.9 + phase), function()
        local r, g, b, a = modulate_color(0, 200, 255, 255, 0.6, 1.5, 1.1 + phase, 0.65)
        graphics.drawCircleFilled({
            x = 0,
            y = 0,
            radius = 48,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)

    -- Circle outline (row 1, col 4)
    local cox = wobble(cx4, 48, 1.05, 1.7 + phase)
    local coy = wobble(cy1, 30, 0.9, 0.5 + phase)
    local scox, scoy = scroll_cam:worldToScreen(cox, coy)
    with_transform(scox, scoy, spin(-1.05, 1.4 + phase), pulse(1.0, 0.45, 2.1, 0.2 + phase), function()
        local r, g, b, a = modulate_color(0, 255, 0, 255, 0.6, 2.0, 0.4 + phase, 0.75)
        graphics.drawCircleOutline({
            x = 0,
            y = 0,
            radius = 46,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)

    -- Rectangle filled (row 2, col 1)
    local rfx = wobble(cx1, 42, 1.25, 1.2 + phase)
    local rfy = wobble(cy2, 28, 0.95, 0.6 + phase)
    local srfx, srfy = scroll_cam:worldToScreen(rfx, rfy)
    with_transform(srfx, srfy, spin(0.85, 2.6 + phase), pulse(1.0, 0.4, 1.5, 0.8 + phase), function()
        local r, g, b, a = modulate_color(255, 128, 0, 255, 0.55, 1.4, 0.9 + phase, 0.7)
        graphics.drawRectangleFilled({
            x = -80,
            y = -45,
            w = 160,
            h = 90,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)

    -- Rectangle outline (row 2, col 2)
    local rox = wobble(cx2, 38, 0.85, 0.2 + phase)
    local roy = wobble(cy2, 26, 1.35, 1.4 + phase)
    local srox, sroy = scroll_cam:worldToScreen(rox, roy)
    with_transform(srox, sroy, spin(-1.35, 2.2 + phase), pulse(1.0, 0.35, 1.7, 0.1 + phase), function()
        local r, g, b, a = modulate_color(240, 240, 240, 255, 0.4, 1.1, 0.2 + phase, 0.5)
        graphics.drawRectangleOutline({
            x = -80,
            y = -45,
            w = 160,
            h = 90,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)

    -- Rounded rectangle filled (row 2, col 3)
    local rrx = wobble(cx3, 46, 1.05, 0.5 + phase)
    local rry = wobble(cy2, 26, 0.9, 1.1 + phase)
    local srrx, srry = scroll_cam:worldToScreen(rrx, rry)
    with_transform(srrx, srry, spin(1.4, 3.1 + phase), pulse(1.0, 0.45, 1.8, 0.6 + phase), function()
        local r, g, b, a = modulate_color(160, 80, 255, 255, 0.6, 1.6, 1.4 + phase, 0.7)
        graphics.drawRectangleRoundedFilled({
            x = -90,
            y = -45,
            w = 180,
            h = 90,
            radius = 16,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)

    -- Rounded rectangle outline (row 2, col 4)
    local rrox = wobble(cx4, 40, 0.75, 1.8 + phase)
    local rroy = wobble(cy2, 30, 1.15, 0.4 + phase)
    local srrox, srroy = scroll_cam:worldToScreen(rrox, rroy)
    with_transform(srrox, srroy, spin(-1.55, 2.9 + phase), pulse(1.0, 0.45, 1.9, 0.9 + phase), function()
        local r, g, b, a = modulate_color(0, 255, 200, 255, 0.6, 1.9, 0.6 + phase, 0.7)
        graphics.drawRectangleRoundedOutline({
            x = -90,
            y = -45,
            w = 180,
            h = 90,
            radius = 20,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)

    -- Triangle filled (row 3, col 1)
    local tfx = wobble(cx1, 48, 1.35, 1.3 + phase)
    local tfy = wobble(cy3, 32, 0.85, 0.7 + phase)
    local stfx, stfy = scroll_cam:worldToScreen(tfx, tfy)
    with_transform(stfx, stfy, spin(1.75, 1.8 + phase), pulse(1.0, 0.45, 1.9, 0.3 + phase), function()
        local r, g, b, a = modulate_color(255, 0, 128, 255, 0.65, 2.0, 1.0 + phase, 0.8)
        graphics.drawTriangleFilled({
            x1 = -70,
            y1 = 60,
            x2 = 0,
            y2 = -70,
            x3 = 70,
            y3 = 60,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)

    -- Triangle outline (row 3, col 2)
    local tox = wobble(cx2, 42, 1.1, 0.9 + phase)
    local toy = wobble(cy3, 28, 1.05, 1.0 + phase)
    local stox, stoy = scroll_cam:worldToScreen(tox, toy)
    with_transform(stox, stoy, spin(-1.1, 0.7 + phase), pulse(1.0, 0.4, 1.6, 0.5 + phase), function()
        local r, g, b, a = modulate_color(0, 200, 120, 255, 0.55, 1.7, 0.2 + phase, 0.7)
        graphics.drawTriangleOutline({
            x1 = -70,
            y1 = 60,
            x2 = 0,
            y2 = -70,
            x3 = 70,
            y3 = 60,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)

    -- Polygon filled (row 3, col 3)
    local pfx = wobble(cx3, 38, 0.95, 0.1 + phase)
    local pfy = wobble(cy3, 26, 1.4, 1.9 + phase)
    local spfx, spfy = scroll_cam:worldToScreen(pfx, pfy)
    with_transform(spfx, spfy, spin(0.95, 2.4 + phase), pulse(1.0, 0.45, 2.1, 0.2 + phase), function()
        local poly_filled = {-60, -20, -20, -60, 40, -30, 60, 20, 10, 60, -50, 30}
        local r, g, b, a = modulate_color(255, 0, 0, 180, 0.7, 2.2, 1.1 + phase, 0.9)
        graphics.drawPolyFilled({
            points = poly_filled,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)

    -- Polygon outline (row 3, col 4)
    local pox = wobble(cx4, 40, 1.2, 1.6 + phase)
    local poy = wobble(cy3, 28, 0.95, 0.3 + phase)
    local spox, spoy = scroll_cam:worldToScreen(pox, poy)
    with_transform(spox, spoy, spin(-1.45, 1.5 + phase), pulse(1.0, 0.45, 1.9, 0.7 + phase), function()
        local poly_outline = {-60, -20, -20, -60, 40, -30, 60, 20, 10, 60, -50, 30}
        local r, g, b, a = modulate_color(0, 255, 255, 255, 0.6, 1.8, 0.8 + phase, 0.75)
        graphics.drawPolyOutline({
            points = poly_outline,
            r = r,
            g = g,
            b = b,
            a = a,
        })
    end)
end

function leo.load()
    elapsed = 0
    scroll_cam = camera.new()
    shape_cam = camera.new()
    font = font_api.new("resources/font/font.ttf", 32)
    scroll_cam:setBounds(0, 0, playfield_w, playfield_h)
    scroll_cam:setClamp(true)
    scroll_cam:setDeadzone(0, 0)
    scroll_cam:setSmoothTime(0.25)
    scroll_cam:setOffset(vw * 0.5, vh * 0.5)
    fps_timer = 0
    fps_frames = 0
    fps_value = 0
end

function leo.update(dt, input)
    elapsed = elapsed + dt
    last_dt = dt
    fps_timer = fps_timer + dt
    fps_frames = fps_frames + 1
    if fps_timer >= 0.5 then
        fps_value = fps_frames / fps_timer
        fps_timer = 0
        fps_frames = 0
    end

    if scroll_cam then
        local center_y = playfield_h * 0.5
        local sweep = (playfield_w - vw) * 0.5
        local cam_x = (playfield_w * 0.5) + math.sin(elapsed * 0.25) * sweep
        scroll_cam:setTarget(cam_x, center_y)
        scroll_cam:update(dt)
    end
end

function leo.draw()
    graphics.clear(18, 18, 22, 255)

    graphics.beginCamera(scroll_cam)

    -- Snowy pixel field across the whole playfield
    draw_snow(playfield_w, playfield_h, 540, 0)

    -- Backdrop stripes for depth
    draw_backdrop(0, 0, playfield_w, playfield_h, 0.0)

    graphics.endCamera()

    -- Multiple shape clusters across the playfield (screen-space via scroll_cam)
    draw_shape_set(0, 0, 0.0)
    draw_shape_set(960, 0, 1.7)
    draw_shape_set(1920, 0, 3.4)

    -- FPS overlay
    graphics.setColor(255, 255, 255, 255)
    if font then
        font:print(string.format("FPS: %.1f", fps_value), 20, 30, 32)
    end
end

function leo.shutdown()
end
