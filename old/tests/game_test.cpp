// ==========================================================
// File: tests/game_test.cpp
// Unit tests for leo/game.h high-level loop
// - Verifies callback order & counts
// - Verifies pause/quitting helpers
// - Verifies setup failure path
// - Verifies user_data plumbing & basic telemetry sanity
// ==========================================================

#include <catch2/catch_all.hpp>

extern "C"
{
#include "leo/actor.h"
#include "leo/color.h"
#include "leo/engine.h"
#include "leo/game.h"
}

struct CallState
{
    // Counters
    int setup_called = 0;
    int update_called = 0;
    int render_ui_called = 0;
    int shutdown_called = 0;

    // Captured ctx for assertions within callbacks
    leo_GameContext *ctx_seen = nullptr;

    // Telemetry snapshots
    int64_t first_frame = -1;
    int64_t last_frame = -1;
    float last_dt = -1.0f;
    double last_time = -1.0;

    // Config payload echo
    void *expected_user_data = nullptr;

    // Behavior controls for scenarios
    bool make_setup_fail = false;
    bool expect_start_paused = false;
};

// ---------- Callback shims ----------

static bool on_setup_ok(leo_GameContext *ctx)
{
    CallState *st = (CallState *)ctx->user_data; // We pass our state via cfg.user_data
    st->setup_called += 1;
    st->ctx_seen = ctx;

    // Basic invariants
    REQUIRE(ctx->actors != nullptr);
    REQUIRE(ctx->root != nullptr);

    // Ensure user_data plumbed through ctx
    REQUIRE(ctx->user_data == st);

    // If this scenario wants to fail, abort now
    if (st->make_setup_fail)
        return false;

    return true;
}

static void on_update_once_then_quit(leo_GameContext *ctx)
{
    CallState *st = (CallState *)ctx->user_data;
    st->update_called += 1;

    // Telemetry should be readable (not asserting strict values: headless time can be ~0)
    if (st->first_frame < 0)
        st->first_frame = ctx->frame;
    st->last_frame = ctx->frame;
    st->last_dt = ctx->dt;
    st->last_time = ctx->time_sec;

    // Pause state should reflect config at first tick
    if (st->update_called == 1)
    {
        bool paused_now = leo_GameIsPaused(ctx);
        if (st->expect_start_paused)
        {
            REQUIRE(paused_now == true);
            // Toggle off and verify
            leo_GameSetPaused(ctx, false);
            REQUIRE(leo_GameIsPaused(ctx) == false);
        }
        else
        {
            REQUIRE(paused_now == false);
            // Toggle on and verify
            leo_GameSetPaused(ctx, true);
            REQUIRE(leo_GameIsPaused(ctx) == true);
            // Toggle back off so we don't impact other checks
            leo_GameSetPaused(ctx, false);
        }
    }

    // Request to exit loop after first visible update
    leo_GameQuit(ctx);
    REQUIRE(ctx->request_quit == true);
}

static void on_render_ui_count(leo_GameContext *ctx)
{
    CallState *st = (CallState *)ctx->user_data;
    st->render_ui_called += 1;

    // We are between Begin/EndDrawing in implementation; here we just sanity check ctx presence
    REQUIRE(st->ctx_seen == ctx);
}

static void on_shutdown_mark(leo_GameContext *ctx)
{
    CallState *st = (CallState *)ctx->user_data;
    st->shutdown_called += 1;
    REQUIRE(st->ctx_seen == ctx);
}

// ---------- Helpers ----------

static leo_GameConfig make_cfg(CallState *st, bool start_paused)
{
    leo_GameConfig cfg{};
    cfg.window_width = 320;
    cfg.window_height = 180;
    cfg.window_title = "leo/game test";
    cfg.target_fps = 0;    // no cap; we will quit immediately anyway
    cfg.logical_width = 0; // disable logical scaling for the test
    cfg.logical_height = 0;
    cfg.presentation = LEO_LOGICAL_PRESENTATION_LETTERBOX;
    cfg.scale_mode = LEO_SCALE_LINEAR;
    cfg.clear_color = LEO_BLACK;
    cfg.start_paused = start_paused;
    cfg.user_data = st; // Pass state through ctx->user_data
    return cfg;
}

static leo_GameCallbacks make_cbs()
{
    leo_GameCallbacks cbs{};
    cbs.on_setup = &on_setup_ok;
    cbs.on_update = &on_update_once_then_quit;
    cbs.on_render_ui = &on_render_ui_count;
    cbs.on_shutdown = &on_shutdown_mark;
    return cbs;
}

// ---------- Tests ----------

TEST_CASE("leo_GameRun: basic lifecycle executes one frame and shuts down cleanly")
{
    CallState st{};
    st.expect_start_paused = false;

    auto cfg = make_cfg(&st, /*start_paused=*/false);
    auto cbs = make_cbs();

    int rc = leo_GameRun(&cfg, &cbs);

    REQUIRE(rc == 0);
    REQUIRE(st.setup_called == 1);
    REQUIRE(st.update_called >= 1);
    REQUIRE(st.render_ui_called >= 1);
    REQUIRE(st.shutdown_called == 1);
    REQUIRE(st.ctx_seen != nullptr);

    // Telemetry is best-effort: ensure frame recorded and non-negative dt/time
    REQUIRE(st.first_frame >= 0);
    REQUIRE(st.last_frame >= st.first_frame);
    REQUIRE(st.last_dt >= 0.0f);
    REQUIRE(st.last_time >= 0.0);
}

TEST_CASE("leo_GameRun: start_paused honored and can be toggled in update")
{
    CallState st{};
    st.expect_start_paused = true;

    auto cfg = make_cfg(&st, /*start_paused=*/true);
    auto cbs = make_cbs();

    int rc = leo_GameRun(&cfg, &cbs);

    REQUIRE(rc == 0);
    REQUIRE(st.setup_called == 1);
    REQUIRE(st.update_called >= 1);
    REQUIRE(st.render_ui_called >= 1);
    REQUIRE(st.shutdown_called == 1);
}

TEST_CASE("leo_GameRun: setup failure aborts before entering main loop")
{
    CallState st{};
    st.make_setup_fail = true;

    auto cfg = make_cfg(&st, /*start_paused=*/false);
    auto cbs = make_cbs();

    int rc = leo_GameRun(&cfg, &cbs);

    REQUIRE(rc != 0);
    REQUIRE(st.setup_called == 1);
    REQUIRE(st.update_called == 0);
    REQUIRE(st.render_ui_called == 0);
    REQUIRE(st.shutdown_called == 0); // shutdown shouldn't run if startup aborts
}

TEST_CASE("leo_GameRun: user_data is plumbed through ctx")
{
    CallState st{};
    auto cfg = make_cfg(&st, /*start_paused=*/false);
    auto cbs = make_cbs();

    int rc = leo_GameRun(&cfg, &cbs);
    REQUIRE(rc == 0);

    // Verified inside callbacks via REQUIRE(ctx->user_data == st)
    REQUIRE(st.setup_called == 1);
}
