// tests/lua_game_test.cpp
#include <catch2/catch_all.hpp>

extern "C"
{
#include "leo/color.h"
#include "leo/engine.h"
#include "leo/lua_game.h"
}

struct LuaCallState
{
    int setup_called = 0;
    int update_called = 0;
    int render_called = 0;
    int shutdown_called = 0;
    
    leo_LuaGameContext *ctx_seen = nullptr;
    
    int64_t first_frame = -1;
    int64_t last_frame = -1;
    float last_dt = -1.0f;
    double last_time = -1.0;
    
    bool make_setup_fail = false;
};

static bool on_lua_setup_ok(leo_LuaGameContext *ctx)
{
    LuaCallState *st = (LuaCallState *)ctx->user_data;
    st->setup_called += 1;
    st->ctx_seen = ctx;
    
    REQUIRE(ctx->user_data == st);
    
    if (st->make_setup_fail)
        return false;
        
    return true;
}

static void on_lua_update_once_then_quit(leo_LuaGameContext *ctx)
{
    LuaCallState *st = (LuaCallState *)ctx->user_data;
    st->update_called += 1;
    
    if (st->first_frame < 0)
        st->first_frame = ctx->frame;
    st->last_frame = ctx->frame;
    st->last_dt = ctx->dt;
    st->last_time = ctx->time_sec;
    
    leo_LuaGameQuit(ctx);
    REQUIRE(ctx->request_quit == true);
}

static void on_lua_render_count(leo_LuaGameContext *ctx)
{
    LuaCallState *st = (LuaCallState *)ctx->user_data;
    st->render_called += 1;
    
    REQUIRE(st->ctx_seen == ctx);
}

static void on_lua_shutdown_mark(leo_LuaGameContext *ctx)
{
    LuaCallState *st = (LuaCallState *)ctx->user_data;
    st->shutdown_called += 1;
    REQUIRE(st->ctx_seen == ctx);
}

static leo_LuaGameConfig make_lua_cfg(LuaCallState *st)
{
    leo_LuaGameConfig cfg{};
    cfg.window_width = 320;
    cfg.window_height = 180;
    cfg.window_title = "lua game test";
    cfg.target_fps = 0;
    cfg.clear_color = LEO_BLACK;
    cfg.user_data = st;
    return cfg;
}

static leo_LuaGameCallbacks make_lua_cbs()
{
    leo_LuaGameCallbacks cbs{};
    cbs.on_setup = &on_lua_setup_ok;
    cbs.on_update = &on_lua_update_once_then_quit;
    cbs.on_render = &on_lua_render_count;
    cbs.on_shutdown = &on_lua_shutdown_mark;
    return cbs;
}

TEST_CASE("leo_LuaGameRun: basic lifecycle executes one frame and shuts down cleanly")
{
    LuaCallState st{};
    
    auto cfg = make_lua_cfg(&st);
    auto cbs = make_lua_cbs();
    
    int rc = leo_LuaGameRun(&cfg, &cbs);
    
    REQUIRE(rc == 0);
    REQUIRE(st.setup_called == 1);
    REQUIRE(st.update_called >= 1);
    REQUIRE(st.render_called >= 1);
    REQUIRE(st.shutdown_called == 1);
    REQUIRE(st.ctx_seen != nullptr);
    
    REQUIRE(st.first_frame >= 0);
    REQUIRE(st.last_frame >= st.first_frame);
    REQUIRE(st.last_dt >= 0.0f);
    REQUIRE(st.last_time >= 0.0);
}

TEST_CASE("leo_LuaGameRun: setup failure aborts before entering main loop")
{
    LuaCallState st{};
    st.make_setup_fail = true;
    
    auto cfg = make_lua_cfg(&st);
    auto cbs = make_lua_cbs();
    
    int rc = leo_LuaGameRun(&cfg, &cbs);
    
    REQUIRE(rc != 0);
    REQUIRE(st.setup_called == 1);
    REQUIRE(st.update_called == 0);
    REQUIRE(st.render_called == 0);
    REQUIRE(st.shutdown_called == 0);
}

TEST_CASE("leo_LuaGameRun: user_data is plumbed through ctx")
{
    LuaCallState st{};
    auto cfg = make_lua_cfg(&st);
    auto cbs = make_lua_cbs();
    
    int rc = leo_LuaGameRun(&cfg, &cbs);
    REQUIRE(rc == 0);
    
    REQUIRE(st.setup_called == 1);
}

TEST_CASE("leo_LuaGameRun: Lua interpreter initializes and works")
{
    LuaCallState st{};
    auto cfg = make_lua_cfg(&st);
    auto cbs = make_lua_cbs();
    
    // This test verifies that Lua state creation and basic execution works
    // The Lua test (2+2) happens inside leo_LuaGameRun before the game loop
    int rc = leo_LuaGameRun(&cfg, &cbs);
    REQUIRE(rc == 0);  // Should succeed if Lua works correctly
}
