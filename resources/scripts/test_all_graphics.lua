
        function leo_init() return true end
        function leo_update(dt) leo_quit() end
        function leo_render()
            leo_clear_background(32, 32, 32, 255)
            leo_draw_pixel(10, 10, 255, 255, 255, 255)
            leo_draw_line(0, 0, 50, 50, 255, 0, 0, 255)
            leo_draw_circle(25, 25, 10, 0, 255, 0, 255)
            leo_draw_circle_filled(75, 25, 8, 0, 0, 255, 255)
            leo_draw_rectangle(10, 40, 30, 20, 255, 255, 0, 255)
            leo_draw_rectangle_lines(50, 40, 30, 20, 255, 0, 255, 255)
            leo_draw_triangle(10, 70, 30, 70, 20, 90, 128, 255, 128, 255)
            leo_draw_triangle_filled(50, 70, 70, 70, 60, 90, 255, 128, 128, 255)
        end
        function leo_exit() end
    