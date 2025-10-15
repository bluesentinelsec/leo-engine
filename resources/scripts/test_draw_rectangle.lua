
        function leo_init()
            return true
        end
        
        function leo_update(dt) 
            leo_quit() 
        end
        
        function leo_render()
            leo_draw_rectangle(5, 10, 50, 30, 0, 255, 0, 255)  -- Green rect at (5,10) size 50x30
        end
        
        function leo_exit() 
        end
    