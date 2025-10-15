-- Default Leo Engine Lua Game Script

function leo_init()
    print("Lua: Game initialized!")
    leo_quit()  -- Quit immediately for testing
    return true
end

function leo_update(dt, time, frame)
    -- Update game logic here
    -- dt = delta time, time = total time, frame = frame number
end

function leo_render()
    -- Render game here
    -- Drawing functions will be available once we add bindings
end

function leo_exit()
    print("Lua: Game exiting!")
end
