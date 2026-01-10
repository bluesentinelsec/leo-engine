/* ==========================================================
 * File: tests/actor_test.cpp
 * ========================================================== */

#include <catch2/catch_all.hpp>

extern "C"
{
#include <leo/actor.h>
#include <leo/signal.h>
}

#include <cstdarg>
#include <cstring>
#include <string>
#include <vector>

/* ---------------- Helpers & fixtures ---------------- */

struct Counters
{
    int init_calls = 0;
    int update_calls = 0;
    int render_calls = 0;
    int exit_calls = 0;
};

static void v_on_update(leo_Actor *self, float dt)
{
    (void)dt;
    auto *c = (Counters *)leo_actor_userdata(self);
    if (c)
        c->update_calls++;
}
static void v_on_render(leo_Actor *self)
{
    auto *c = (Counters *)leo_actor_userdata(self);
    if (c)
        c->render_calls++;
}
static void v_on_exit(leo_Actor *self)
{
    auto *c = (Counters *)leo_actor_userdata(self);
    if (c)
        c->exit_calls++;
}
static bool v_on_init_true(leo_Actor *self)
{
    auto *c = (Counters *)leo_actor_userdata(self);
    if (c)
        c->init_calls++;
    return true;
}
static bool v_on_init_false(leo_Actor *self)
{
    auto *c = (Counters *)leo_actor_userdata(self);
    if (c)
        c->init_calls++;
    return false;
}

static const leo_ActorVTable VT_OK{v_on_init_true, v_on_update, v_on_render, v_on_exit};

static const leo_ActorVTable VT_FAIL_INIT{v_on_init_false, v_on_update, v_on_render, v_on_exit};

/* Signal capture */
struct SigHit
{
    std::string name;
    leo_ActorUID who = 0;
};
static std::vector<SigHit> g_sigs;
static void clear_sigs()
{
    g_sigs.clear();
}

static void on_spawned(void *owner, void *user, va_list)
{
    (void)user;
    auto *a = (leo_Actor *)owner;
    g_sigs.push_back({"spawned", leo_actor_uid(a)});
}
static void on_killed(void *owner, void *user, va_list)
{
    (void)user;
    auto *a = (leo_Actor *)owner;
    g_sigs.push_back({"killed", leo_actor_uid(a)});
}
static void on_exiting(void *owner, void *user, va_list)
{
    (void)user;
    auto *a = (leo_Actor *)owner;
    g_sigs.push_back({"exiting", leo_actor_uid(a)});
}

/* Helper: spawn an actor with counters */
static leo_Actor *spawn(leo_Actor *parent, const char *name, Counters *counters, const leo_ActorVTable *vt = &VT_OK,
                        bool paused = false, uint64_t groups = 0)
{
    leo_ActorDesc d{};
    d.name = name;
    d.vtable = vt;
    d.user_data = counters;
    d.start_paused = paused;
    d.groups = groups;
    return leo_actor_spawn(parent, &d);
}

/* For z-order test: a function-pointer-compatible render hook
   that pushes the actor's name into a shared vector. */
static std::vector<std::string> *g_render_sink = nullptr;
static void vt_record_render(leo_Actor *self)
{
    if (!g_render_sink)
        return;
    const char *n = leo_actor_name(self);
    g_render_sink->push_back(n ? n : "");
}

/* ---------------- Tests ---------------- */

TEST_CASE("actor system basic lifecycle", "[actor]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    REQUIRE(sys != nullptr);

    auto *root = leo_actor_system_root(sys);
    REQUIRE(root != nullptr);
    REQUIRE(leo_actor_name(root) != nullptr);
    REQUIRE(std::string(leo_actor_name(root)) == "root");

    leo_actor_system_destroy(sys);
}

TEST_CASE("spawn/update/render/exit and signals", "[actor]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    auto *root = leo_actor_system_root(sys);

    Counters c{};
    clear_sigs();

    auto *a = spawn(root, "A", &c);
    REQUIRE(a != nullptr);

    // Connect to signals AFTER spawn (spawned has already fired internally),
    // so we only expect killed/exiting later.
    leo_signal_connect(leo_actor_emitter(a), "killed", on_killed, nullptr);
    leo_signal_connect(leo_actor_emitter(a), "exiting", on_exiting, nullptr);

    // on_init already ran once
    CHECK(c.init_calls == 1);

    // One update + one render
    leo_actor_system_update(sys, 0.016f);
    leo_actor_system_render(sys);

    CHECK(c.update_calls == 1);
    CHECK(c.render_calls == 1);

    // Kill schedules destruction; still present before update drain
    const auto uid = leo_actor_uid(a);
    leo_actor_kill(a);
    CHECK(leo_actor_find_by_uid(sys, uid) == a);

    // After update, on_exit called, actor removed, signals delivered
    leo_actor_system_update(sys, 0.016f);

    CHECK(c.exit_calls == 1);
    CHECK(leo_actor_find_by_uid(sys, uid) == nullptr);

    // We connected to killed/exiting only; both must be present, in that order.
    REQUIRE(g_sigs.size() == 2);
    CHECK(g_sigs[0].name == "killed");
    CHECK(g_sigs[0].who == uid);
    CHECK(g_sigs[1].name == "exiting");
    CHECK(g_sigs[1].who == uid);

    leo_actor_system_destroy(sys);
}

TEST_CASE("pause: local, parent cascade, and system pause", "[actor][pause]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    auto *root = leo_actor_system_root(sys);

    Counters pC{}, cC{};
    auto *parent = spawn(root, "P", &pC);
    auto *child = spawn(parent, "C", &cC);

    // baseline
    leo_actor_system_update(sys, 0.016f);
    CHECK(pC.update_calls == 1);
    CHECK(cC.update_calls == 1);

    // local pause on parent cascades to child
    leo_actor_set_paused(parent, true);
    leo_actor_system_update(sys, 0.016f);
    CHECK(pC.update_calls == 1); // unchanged
    CHECK(cC.update_calls == 1); // unchanged

    // unpause parent; pause only child
    leo_actor_set_paused(parent, false);
    leo_actor_set_paused(child, true);
    leo_actor_system_update(sys, 0.016f);
    CHECK(pC.update_calls == 2);
    CHECK(cC.update_calls == 1);

    // system pause stops both
    leo_actor_set_paused(child, false);
    leo_actor_system_set_paused(sys, true);
    leo_actor_system_update(sys, 0.016f);
    CHECK(pC.update_calls == 2);
    CHECK(cC.update_calls == 1);

    // Render should still run while paused (engine-side behavior)
    int prev_render_parent = pC.render_calls;
    int prev_render_child = cC.render_calls;
    leo_actor_system_render(sys);
    CHECK(pC.render_calls == prev_render_parent + 1);
    CHECK(cC.render_calls == prev_render_child + 1);

    leo_actor_system_destroy(sys);
}

TEST_CASE("groups: create/find/add/remove/enumerate", "[actor][groups]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    auto *root = leo_actor_system_root(sys);

    int gEnemies = leo_actor_group_get_or_create(sys, "enemies");
    int gBullets = leo_actor_group_get_or_create(sys, "bullets");
    REQUIRE(gEnemies >= 0);
    REQUIRE(gBullets >= 0);
    REQUIRE(leo_actor_group_find(sys, "enemies") == gEnemies);

    Counters e1C{}, e2C{}, b1C{};
    auto *e1 = spawn(root, "E1", &e1C);
    auto *e2 = spawn(root, "E2", &e2C);
    auto *b1 = spawn(root, "B1", &b1C);

    leo_actor_add_to_group(e1, gEnemies);
    leo_actor_add_to_group(e2, gEnemies);
    leo_actor_add_to_group(b1, gBullets);

    CHECK(leo_actor_in_group(e1, gEnemies));
    CHECK_FALSE(leo_actor_in_group(e1, gBullets));

    std::vector<std::string> names;
    leo_actor_for_each_in_group(
        sys, gEnemies,
        [](leo_Actor *a, void *user) {
            auto *out = (std::vector<std::string> *)user;
            const char *n = leo_actor_name(a);
            out->push_back(n ? n : "");
        },
        &names);

    REQUIRE(names.size() == 2);
    CHECK((names[0] == "E1" || names[0] == "E2"));
    CHECK((names[1] == "E1" || names[1] == "E2"));

    // remove & re-enumerate
    names.clear();
    leo_actor_remove_from_group(e2, gEnemies);
    leo_actor_for_each_in_group(
        sys, gEnemies,
        [](leo_Actor *a, void *user) {
            auto *out = (std::vector<std::string> *)user;
            const char *n = leo_actor_name(a);
            out->push_back(n ? n : "");
        },
        &names);
    REQUIRE(names.size() == 1);
    CHECK(names[0] == "E1");

    leo_actor_system_destroy(sys);
}

TEST_CASE("find by name: child vs recursive", "[actor][lookup]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    auto *root = leo_actor_system_root(sys);

    Counters lC{}, aC{}, bC{}, cC{};
    auto *level = spawn(root, "Level", &lC);
    auto *a = spawn(level, "A", &aC);
    (void)a;
    auto *b = spawn(level, "B", &bC);
    auto *c = spawn(b, "C", &cC); // grandchild of level
    (void)c;

    // child lookup only among direct children
    REQUIRE(leo_actor_find_child_by_name(level, "A") != nullptr);
    REQUIRE(leo_actor_find_child_by_name(level, "B") != nullptr);
    REQUIRE(leo_actor_find_child_by_name(level, "C") == nullptr);

    // recursive finds descendants (including immediate children)
    REQUIRE(leo_actor_find_recursive_by_name(level, "C") != nullptr);

    leo_actor_system_destroy(sys);
}

TEST_CASE("find by UID", "[actor][lookup]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    auto *root = leo_actor_system_root(sys);

    Counters c{};
    auto *a = spawn(root, "Hero", &c);
    REQUIRE(a != nullptr);

    auto uid = leo_actor_uid(a);
    REQUIRE(uid != 0);

    auto *found = leo_actor_find_by_uid(sys, uid);
    REQUIRE(found == a);

    leo_actor_system_destroy(sys);
}

TEST_CASE("z-order: render order follows (z,seq) stable sort; set_z reorders", "[actor][z]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    auto *root = leo_actor_system_root(sys);

    // We'll collect names of actors as they render.
    std::vector<std::string> order;
    g_render_sink = &order;

    // Use a vtable that records renders to 'order'
    leo_ActorVTable vt = VT_OK;
    vt.on_render = vt_record_render;

    Counters c1{}, c2{}, c3{};
    leo_ActorDesc d1{"A", &vt, &c1, 0, false};
    leo_ActorDesc d2{"B", &vt, &c2, 0, false};
    leo_ActorDesc d3{"C", &vt, &c3, 0, false};

    auto *A = leo_actor_spawn(root, &d1);
    auto *B = leo_actor_spawn(root, &d2);
    auto *C = leo_actor_spawn(root, &d3);
    REQUIRE(A);
    REQUIRE(B);
    REQUIRE(C);

    // default z=0, order should be A,B,C (by seq/insertion)
    order.clear();
    leo_actor_system_render(sys);
    REQUIRE(order.size() >= 3);
    // the last three entries correspond to A,B,C (root may also render elsewhere)
    std::vector<std::string> tail(order.end() - 3, order.end());
    CHECK(tail[0] == "A");
    CHECK(tail[1] == "B");
    CHECK(tail[2] == "C");

    // Raise B above others
    leo_actor_set_z(B, 10);
    order.clear();
    leo_actor_system_render(sys);
    REQUIRE(order.size() >= 3);
    tail.assign(order.end() - 3, order.end());
    CHECK(tail[0] == "A"); // z=0
    CHECK(tail[1] == "C"); // z=0, seq after A
    CHECK(tail[2] == "B"); // z=10

    // Set C to z=10 as well; tie on z breaks by seq (B before C)
    leo_actor_set_z(C, 10);
    order.clear();
    leo_actor_system_render(sys);
    REQUIRE(order.size() >= 3);
    tail.assign(order.end() - 3, order.end());
    CHECK(tail[0] == "A"); // z=0
    CHECK(tail[1] == "B"); // z=10, earlier seq
    CHECK(tail[2] == "C"); // z=10, later seq

    g_render_sink = nullptr;
    leo_actor_system_destroy(sys);
}

TEST_CASE("user data get/set", "[actor]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    auto *root = leo_actor_system_root(sys);

    Counters c{};
    auto *a = spawn(root, "UD", &c);
    REQUIRE(a);

    int value = 42;
    leo_actor_set_userdata(a, &value);
    REQUIRE(leo_actor_userdata(a) == &value);

    leo_actor_system_destroy(sys);
}

TEST_CASE("on_init returning false cancels actor (deferred kill)", "[actor]")
{
    leo_ActorSystem *sys = leo_actor_system_create();
    auto *root = leo_actor_system_root(sys);

    Counters badC{};
    leo_ActorDesc badDesc{};
    badDesc.name = "Bad";
    badDesc.vtable = &VT_FAIL_INIT;
    badDesc.user_data = &badC;

    auto *bad = leo_actor_spawn(root, &badDesc);
    // Spawn returns NULL on init failure
    REQUIRE(bad == nullptr);

    // Actor was inserted then immediately scheduled for kill.
    // It should still be findable by name BEFORE update (queued).
    auto *pre = leo_actor_find_recursive_by_name(root, "Bad");
    REQUIRE(pre != nullptr);

    // After update, kill drain removes it and runs on_exit.
    leo_actor_system_update(sys, 0.016f);
    auto *post = leo_actor_find_recursive_by_name(root, "Bad");
    REQUIRE(post == nullptr);

    // init was called once; exit is also called once due to deferred destruction.
    CHECK(badC.init_calls == 1);
    CHECK(badC.exit_calls == 1);

    leo_actor_system_destroy(sys);
}
