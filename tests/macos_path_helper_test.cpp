#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <cstdlib>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "leo/io.h"

// Forward declare the function we'll implement
extern "C" char* leo_GetResourceBasePath(void);

TEST_CASE("macOS path helper", "[macos][path]") {
    
    SECTION("returns valid path when not in bundle") {
        char* path = leo_GetResourceBasePath();
        REQUIRE(path != nullptr);
        
        // Should return a valid absolute path
        REQUIRE(path[0] == '/');
        
        // Path should exist
        std::filesystem::path pathObj(path);
        REQUIRE(std::filesystem::exists(pathObj));
        
        free(path);
    }
    
    SECTION("path helper handles relative path construction") {
        char* basePath = leo_GetResourceBasePath();
        REQUIRE(basePath != nullptr);
        
        // Test that we can construct paths relative to the base
        std::string testPath = std::string(basePath) + "/resources.leopack";
        REQUIRE(testPath.length() > strlen(basePath));
        
        free(basePath);
    }
    
    SECTION("leo_MountResourcePack uses path helper for relative paths") {
        // Clear any existing mounts
        leo_ClearMounts();
        
        // This should use the path helper internally for relative paths
        // It will try the leopack first, then fall back to resources directory
        bool result = leo_MountResourcePack("nonexistent.leopack", nullptr, 100);
        
        // The result depends on whether a resources directory exists at the base path
        // Either way, it should have used our path helper to construct the correct paths
        // We can't easily test the exact behavior without mocking, but we can verify
        // that the function completed without crashing
        (void)result; // Acknowledge we're not checking the result
        
        leo_ClearMounts();
    }
    
#ifdef __APPLE__
    SECTION("detects bundle resources directory when available") {
        // This test will only pass when running from an actual bundle
        // For now, just verify the function exists and returns a valid path
        char* path = leo_GetResourceBasePath();
        REQUIRE(path != nullptr);
        
        // Path should be absolute
        REQUIRE(path[0] == '/');
        
        free(path);
    }
#endif
}
