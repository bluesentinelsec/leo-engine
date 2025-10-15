-- Default Leo Engine Lua Game Script

local time = 0

function leo_init()
    print("Lua: Game initialized with graphics bindings!")
    return true
end

function leo_update(dt)
    time = time + dt
    
    -- Quit after 3 seconds for demo
    if time > 3.0 then
        leo_quit()
    end
end

function leo_render()
    -- Clear to dark blue
    leo_clear_background(32, 32, 64, 255)
    
    -- Draw animated rectangle
    local rect_x = 50 + math.floor(math.sin(time * 2) * 30)
    leo_draw_rectangle(rect_x, 50, 60, 40, 255, 128, 0, 255)
    
    -- Draw pixel trail
    for i = 0, 20 do
        local x = 20 + i * 3
        local y = 20 + math.floor(math.sin(time * 3 + i * 0.2) * 10)
        local brightness = math.floor(128 + 127 * math.sin(time + i * 0.1))
        leo_draw_pixel(x, y, brightness, 255, brightness, 255)
    end
end

function leo_exit()
    print("Lua: Game exiting!")
end
