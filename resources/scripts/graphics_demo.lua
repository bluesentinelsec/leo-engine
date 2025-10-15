-- Leo Engine Graphics Demo - All Drawing Functions

local time = 0

function leo_init()
    print("Graphics Demo: All drawing functions!")
    return true
end

function leo_update(dt)
    time = time + dt
    
    -- Quit after 5 seconds
    if time > 5.0 then
        leo_quit()
    end
end

function leo_render()
    -- Animated background
    local bg = math.floor(32 + 16 * math.sin(time))
    leo_clear_background(bg, bg, bg + 32, 255)
    
    -- Animated pixel trail
    for i = 0, 30 do
        local x = 10 + i * 2
        local y = 10 + math.floor(math.sin(time * 2 + i * 0.1) * 5)
        local brightness = math.floor(128 + 127 * math.sin(time + i * 0.1))
        leo_draw_pixel(x, y, brightness, 255, brightness, 255)
    end
    
    -- Animated line
    local line_end_x = 100 + math.floor(math.sin(time * 3) * 50)
    leo_draw_line(50, 50, line_end_x, 80, 255, 128, 0, 255)
    
    -- Pulsing circle (outline)
    local circle_radius = 15 + math.sin(time * 4) * 5
    leo_draw_circle(150, 60, circle_radius, 0, 255, 255, 255)
    
    -- Rotating filled circle
    local circle_x = 200 + math.cos(time) * 20
    local circle_y = 60 + math.sin(time) * 10
    leo_draw_circle_filled(circle_x, circle_y, 8, 255, 0, 128, 255)
    
    -- Animated rectangle (filled)
    local rect_x = 50 + math.floor(math.sin(time * 1.5) * 30)
    leo_draw_rectangle(rect_x, 120, 40, 25, 255, 255, 0, 200)
    
    -- Rectangle outline
    leo_draw_rectangle_lines(150, 120, 60, 30, 0, 255, 0, 255)
    
    -- Triangle outline
    local tri_offset = math.floor(math.sin(time * 2) * 10)
    leo_draw_triangle(50, 180, 80 + tri_offset, 180, 65, 200, 255, 0, 255, 255)
    
    -- Filled triangle
    leo_draw_triangle_filled(150, 180, 180, 180, 165, 200, 128, 255, 128, 255)
end

function leo_exit()
    print("Graphics Demo finished!")
end
