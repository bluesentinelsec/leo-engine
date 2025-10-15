
        function leo_init() return true end
        function leo_update(dt) leo_quit() end
        function leo_render()
            leo_clear_background(64, 128, 255, 255)  -- Blue background
        end
        function leo_exit() end
    