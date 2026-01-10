// tests/lua_bindings_test.cpp
#include <catch2/catch_all.hpp>
#include <fstream>

extern "C"
{
#include "leo/color.h"
#include "leo/engine.h"
#include "leo/error.h"
#include "leo/lua_game.h"
}

// Helper: reset engine state between tests
static void resetSDLState()
{
    leo_CloseWindow();
    leo_ClearError();
}

TEST_CASE("Lua bindings: draw_pixel function works", "[lua_bindings]")
{
    resetSDLState();

    // Create test script that calls draw_pixel
    const char *test_script = R"(
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
    )";

    // Write test script to temp file
    std::ofstream f("resources/scripts/test_draw_pixel.lua");
    f << test_script;
    f.close();

    leo_LuaGameConfig cfg{};
    cfg.script_path = "scripts/test_draw_pixel.lua";
    cfg.window_width = 100;
    cfg.window_height = 100;
    cfg.clear_color = LEO_BLACK;

    leo_LuaGameCallbacks cbs{};

    int rc = leo_LuaGameRun(&cfg, &cbs);
    REQUIRE(rc == 0); // Should succeed if draw_pixel binding works

    resetSDLState();
}

TEST_CASE("Lua bindings: draw_rectangle function works", "[lua_bindings]")
{
    resetSDLState();

    // Create test script that calls draw_rectangle
    const char *test_script = R"(
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
    )";

    // Write test script to temp file
    std::ofstream f("resources/scripts/test_draw_rectangle.lua");
    f << test_script;
    f.close();

    leo_LuaGameConfig cfg{};
    cfg.script_path = "scripts/test_draw_rectangle.lua";
    cfg.window_width = 100;
    cfg.window_height = 100;
    cfg.clear_color = LEO_BLACK;

    leo_LuaGameCallbacks cbs{};

    int rc = leo_LuaGameRun(&cfg, &cbs);
    REQUIRE(rc == 0); // Should succeed if draw_rectangle binding works

    resetSDLState();
}
TEST_CASE("Lua bindings: clear_background function works", "[lua_bindings]")
{
    resetSDLState();

    const char *test_script = R"(
        function leo_init() return true end
        function leo_update(dt) leo_quit() end
        function leo_render()
            leo_clear_background(64, 128, 255, 255)  -- Blue background
        end
        function leo_exit() end
    )";

    std::ofstream f("resources/scripts/test_clear_background.lua");
    f << test_script;
    f.close();

    leo_LuaGameConfig cfg{};
    cfg.script_path = "scripts/test_clear_background.lua";
    cfg.window_width = 100;
    cfg.window_height = 100;

    int rc = leo_LuaGameRun(&cfg, nullptr);
    REQUIRE(rc == 0);

    resetSDLState();
}
TEST_CASE("Lua bindings: all graphics functions work", "[lua_bindings]")
{
    resetSDLState();

    const char *test_script = R"(
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
    )";

    std::ofstream f("resources/scripts/test_all_graphics.lua");
    f << test_script;
    f.close();

    leo_LuaGameConfig cfg{};
    cfg.script_path = "scripts/test_all_graphics.lua";
    cfg.window_width = 100;
    cfg.window_height = 100;

    int rc = leo_LuaGameRun(&cfg, nullptr);
    REQUIRE(rc == 0);

    resetSDLState();
}
