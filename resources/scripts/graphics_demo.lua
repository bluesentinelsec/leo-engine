-- Leo Engine Graphics Demo - All Drawing Functions

local time = 0

function leo.load()
    print("Graphics Demo: All drawing functions!")
    return true
end

function leo.update(dt)
    time = time + dt

    -- Quit after 5 seconds
    if time > 5.0 then
        leo.quit()
    end
end

function leo.draw()
    -- Animated background
    local bg = math.floor(32 + 16 * math.sin(time))
    leo.graphics.clear(bg, bg, bg + 32, 255)

    -- Animated pixel trail
    for i = 0, 30 do
        local x = 10 + i * 2
        local y = 10 + math.floor(math.sin(time * 2 + i * 0.1) * 5)
        local brightness = math.floor(128 + 127 * math.sin(time + i * 0.1))
        leo.graphics.drawPixel({
            x = x,
            y = y,
            r = brightness,
            g = 255,
            b = brightness,
            a = 255,
        })
    end

    -- Animated line
    local line_end_x = 100 + math.floor(math.sin(time * 3) * 50)
    leo.graphics.drawLine({
        x1 = 50,
        y1 = 50,
        x2 = line_end_x,
        y2 = 80,
        r = 255,
        g = 128,
        b = 0,
        a = 255,
    })

    -- Pulsing circle (outline)
    local circle_radius = 15 + math.sin(time * 4) * 5
    leo.graphics.drawCircleOutline({
        x = 150,
        y = 60,
        radius = circle_radius,
        r = 0,
        g = 255,
        b = 255,
        a = 255,
    })

    -- Rotating filled circle
    local circle_x = 200 + math.cos(time) * 20
    local circle_y = 60 + math.sin(time) * 10
    leo.graphics.drawCircleFilled({
        x = circle_x,
        y = circle_y,
        radius = 8,
        r = 255,
        g = 0,
        b = 128,
        a = 255,
    })

    -- Animated rectangle (filled)
    local rect_x = 50 + math.floor(math.sin(time * 1.5) * 30)
    leo.graphics.drawRectangleFilled({
        x = rect_x,
        y = 120,
        w = 40,
        h = 25,
        r = 255,
        g = 255,
        b = 0,
        a = 200,
    })

    -- Rectangle outline
    leo.graphics.drawRectangleOutline({
        x = 150,
        y = 120,
        w = 60,
        h = 30,
        r = 0,
        g = 255,
        b = 0,
        a = 255,
    })

    -- Triangle outline
    local tri_offset = math.floor(math.sin(time * 2) * 10)
    leo.graphics.drawTriangleOutline({
        x1 = 50,
        y1 = 180,
        x2 = 80 + tri_offset,
        y2 = 180,
        x3 = 65,
        y3 = 200,
        r = 255,
        g = 0,
        b = 255,
        a = 255,
    })

    -- Filled triangle
    leo.graphics.drawTriangleFilled({
        x1 = 150,
        y1 = 180,
        x2 = 180,
        y2 = 180,
        x3 = 165,
        y3 = 200,
        r = 128,
        g = 255,
        b = 128,
        a = 255,
    })
end

function leo.shutdown()
    print("Graphics Demo finished!")
end
