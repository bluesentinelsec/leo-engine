-- Leo Engine File Writing Demo
-- Note: This demo shows the concept, but WriteFile isn't bound to Lua yet

function leo_init()
    print("File Writing Demo - WriteFile not yet bound to Lua")
    print("But the C API leo_WriteFile() is working!")
    leo_quit()
    return true
end

function leo_update(dt)
    -- Nothing to do
end

function leo_render()
    leo_clear_background(64, 64, 128, 255)
end

function leo_exit()
    print("Demo finished!")
end
