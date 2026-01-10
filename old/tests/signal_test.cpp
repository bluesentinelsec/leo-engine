/* ==========================================================
 * File: test/signal_test.cpp
 * ========================================================== */

#include "leo/signal.h"
#include <catch2/catch_all.hpp>
#include <cstdarg>
#include <string>
#include <vector>

/* Simple struct to act as an owner */
struct Dummy
{
    leo_SignalEmitter emitter;
    int value;
};

/* Global/static state to capture callback events */
struct CallRecord
{
    std::string signal;
    int int_arg;
    double dbl_arg;
    void *user_data;
};

static std::vector<CallRecord> g_calls;

static void clear_calls()
{
    g_calls.clear();
}

/* Example callbacks */
static void on_int_signal(void *owner, void *user_data, va_list ap)
{
    int arg = va_arg(ap, int);
    g_calls.push_back({"int", arg, 0.0, user_data});
    (void)owner;
}

static void on_mixed_signal(void *owner, void *user_data, va_list ap)
{
    int i = va_arg(ap, int);
    double d = va_arg(ap, double);
    g_calls.push_back({"mixed", i, d, user_data});
    (void)owner;
}

TEST_CASE("signal system basic lifecycle", "[signal]")
{
    Dummy d{};
    leo_signal_emitter_init(&d.emitter, &d);

    REQUIRE_FALSE(leo_signal_is_defined(&d.emitter, "foo"));
    REQUIRE(leo_signal_define(&d.emitter, "foo"));
    REQUIRE(leo_signal_is_defined(&d.emitter, "foo"));

    leo_signal_emitter_free(&d.emitter);
}

TEST_CASE("connect and emit int signal", "[signal]")
{
    Dummy d{};
    leo_signal_emitter_init(&d.emitter, &d);
    clear_calls();

    leo_signal_connect(&d.emitter, "damage", on_int_signal, (void *)0x1234);
    leo_signal_emit(&d.emitter, "damage", 42);

    REQUIRE(g_calls.size() == 1);
    REQUIRE(g_calls[0].signal == "int");
    REQUIRE(g_calls[0].int_arg == 42);
    REQUIRE(g_calls[0].user_data == (void *)0x1234);

    leo_signal_emitter_free(&d.emitter);
}

TEST_CASE("connect and emit mixed args", "[signal]")
{
    Dummy d{};
    leo_signal_emitter_init(&d.emitter, &d);
    clear_calls();

    leo_signal_connect(&d.emitter, "move", on_mixed_signal, nullptr);
    leo_signal_emit(&d.emitter, "move", 7, 3.14);

    REQUIRE(g_calls.size() == 1);
    REQUIRE(g_calls[0].signal == "mixed");
    REQUIRE(g_calls[0].int_arg == 7);
    REQUIRE(g_calls[0].dbl_arg == Catch::Approx(3.14));

    leo_signal_emitter_free(&d.emitter);
}

TEST_CASE("disconnect works", "[signal]")
{
    Dummy d{};
    leo_signal_emitter_init(&d.emitter, &d);
    clear_calls();

    leo_signal_connect(&d.emitter, "foo", on_int_signal, nullptr);
    leo_signal_disconnect(&d.emitter, "foo", on_int_signal, nullptr);
    leo_signal_emit(&d.emitter, "foo", 99);

    REQUIRE(g_calls.empty());

    leo_signal_emitter_free(&d.emitter);
}

TEST_CASE("disconnect_all works", "[signal]")
{
    Dummy d{};
    leo_signal_emitter_init(&d.emitter, &d);
    clear_calls();

    leo_signal_connect(&d.emitter, "bar", on_int_signal, (void *)1);
    leo_signal_connect(&d.emitter, "bar", on_int_signal, (void *)2);
    leo_signal_disconnect_all(&d.emitter, "bar");
    leo_signal_emit(&d.emitter, "bar", 111);

    REQUIRE(g_calls.empty());

    leo_signal_emitter_free(&d.emitter);
}

TEST_CASE("multiple callbacks fire in LIFO order", "[signal]")
{
    Dummy d{};
    leo_signal_emitter_init(&d.emitter, &d);
    clear_calls();

    leo_signal_connect(&d.emitter, "baz", on_int_signal, (void *)1);
    leo_signal_connect(&d.emitter, "baz", on_int_signal, (void *)2);
    leo_signal_emit(&d.emitter, "baz", 5);

    REQUIRE(g_calls.size() == 2);
    // Newest connection (user_data=2) should fire first
    REQUIRE(g_calls[0].user_data == (void *)2);
    REQUIRE(g_calls[1].user_data == (void *)1);

    leo_signal_emitter_free(&d.emitter);
}
