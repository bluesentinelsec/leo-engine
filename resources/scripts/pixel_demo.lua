-- Pixel Drawing Demo

local time = 0

function leo_init()
    print("Pixel demo started!")
    return true
end

function leo_update(dt)
    time = time + dt
    
    -- Quit after 2 seconds for demo
    if time > 2.0 then
        leo_quit()
    end
end

function leo_render()
    -- Draw some colorful pixels
    for i = 0, 50 do
        local x = 10 + i
        local y = 10 + math.floor(math.sin(time * 3 + i * 0.1) * 20)
        local r = math.floor(128 + 127 * math.sin(time + i * 0.1))
        local g = math.floor(128 + 127 * math.cos(time + i * 0.1))
        local b = 255
        
        leo_draw_pixel(x, y, r, g, b, 255)
    end
end

function leo_exit()
    print("Pixel demo finished!")
end
