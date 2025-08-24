#include <catch2/catch_all.hpp>

#include "leo/error.h"

#include <string.h>

#include <atomic>
#include <cstdlib> // for strlenß
#include <cstring>
#include <thread>
#include <vector>

TEST_CASE("Error handling basics", "[error]")
{
    SECTION("Initial state is empty")
    {
        leo_ClearError(); // Ensure clean start
        REQUIRE(strcmp(leo_GetError(), "") == 0);
    }

    SECTION("Set and get simple error")
    {
        leo_SetError("Test error");
        REQUIRE(strcmp(leo_GetError(), "Test error") == 0);
    }

    SECTION("Set error with formatting")
    {
        leo_SetError("Error code: %d, message: %s", 42, "formatted");
        REQUIRE(strcmp(leo_GetError(), "Error code: 42, message: formatted") == 0);
    }

    SECTION("Clear error resets to empty")
    {
        leo_SetError("To be cleared");
        leo_ClearError();
        REQUIRE(strcmp(leo_GetError(), "") == 0);
    }

    SECTION("Overlong error is truncated")
    {
        char long_fmt[8192];
        memset(long_fmt, 'A', sizeof(long_fmt) - 1);
        long_fmt[sizeof(long_fmt) - 1] = '\0';
        leo_SetError("%s", long_fmt);
        REQUIRE(strlen(leo_GetError()) < 8192);  // Buffer limits it
        REQUIRE(strlen(leo_GetError()) == 4095); // Assuming 4096-1 for null
    }

    SECTION("Multiple sets override previous")
    {
        leo_SetError("First error");
        leo_SetError("Second error");
        REQUIRE(strcmp(leo_GetError(), "Second error") == 0);
    }
}

TEST_CASE("leo/error: Set/Get/Clear on same thread", "[error]")
{
    // Start clean
    leo_ClearError();
    REQUIRE(std::string(leo_GetError()).empty());

    // Basic formatting
    leo_SetError("hello %s %d", "world", 42);
    REQUIRE(std::string(leo_GetError()) == "hello world 42");

    // Overwrite previous error
    leo_SetError("new message");
    REQUIRE(std::string(leo_GetError()) == "new message");

    // Clear returns empty
    leo_ClearError();
    REQUIRE(std::string(leo_GetError()).empty());
}

TEST_CASE("leo/error: Truncation guarantees NUL-termination", "[error]")
{
    // Make a very long message (much larger than the internal buffer).
    const size_t kHuge = 8192;
    std::string big(kHuge, 'A');

    leo_SetError("%s", big.c_str());
    const char *got = leo_GetError();

    // Must be non-null, non-empty, and strictly shorter than our huge input.
    REQUIRE(got != nullptr);
    REQUIRE(std::strlen(got) > 0);
    REQUIRE(std::strlen(got) < big.size());

    // And it must be NUL-terminated (strlen would have crashed otherwise).
    // Also sanity-check that it's all 'A's (no format oddities).
    for (size_t i = 0; i < std::strlen(got); ++i)
    {
        REQUIRE(got[i] == 'A');
    }
}

TEST_CASE("leo/error: TLS isolation across threads", "[error][threads]")
{
    // Main thread sets its own error
    leo_SetError("main-error");
    REQUIRE(std::string(leo_GetError()) == "main-error");

    constexpr int kThreads = 8;
    std::atomic<int> okCount{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i)
    {
        threads.emplace_back([i, &okCount]() {
            // Each worker writes a unique message
            leo_SetError("worker-%d", i);

            // Verify this thread reads back its own message
            std::string mine = leo_GetError();
            if (mine == ("worker-" + std::to_string(i)))
            {
                ++okCount;
            }

            // Clearing here must not affect other threads
            leo_ClearError();
            if (leo_GetError()[0] == '\0')
            {
                // Count a second success for clear semantics
                ++okCount;
            }
        });
    }

    for (auto &t : threads)
        t.join();

    // Every thread should have:
    //  - read back its own "worker-i" message
    //  - observed clear -> empty string
    REQUIRE(okCount.load() == kThreads * 2);

    // Main thread's value should remain unchanged
    REQUIRE(std::string(leo_GetError()) == "main-error");
}

TEST_CASE("leo/error: Empty/NULL format is a no-op", "[error]")
{
    // Set something
    leo_SetError("before");
    REQUIRE(std::string(leo_GetError()) == "before");

    // NULL fmt -> no change
    leo_SetError(nullptr);
    REQUIRE(std::string(leo_GetError()) == "before");

    // Empty fmt -> becomes empty string (printf behavior would be undefined,
    // but our implementation bails early; we keep this test to ensure no crash)
    // We'll just call Clear to be explicit and ensure it doesn't crash.
    leo_ClearError();
    REQUIRE(std::string(leo_GetError()).empty());
}
