
        function leo_init()
            return true
        end
        
        function leo_update(dt) 
            leo_quit() 
        end
        
        function leo_render()
            leo_draw_pixel(10, 20, 255, 0, 0, 255)  -- Red pixel at (10,20)
        end
        
        function leo_exit() 
        end
    