#include <catch2/catch_test_macros.hpp>

#ifdef __APPLE__
#include <filesystem>
#include <fstream>

#include "leo/io.h"
#include "leo/macos_path_helper.h"

TEST_CASE("macOS path helper integration", "[macos][integration]") {
    
    SECTION("path helper integration with VFS") {
        // Get the base path
        char* basePath = leo_GetResourceBasePath();
        REQUIRE(basePath != nullptr);
        
        // Create a temporary test directory structure
        std::filesystem::path testDir = std::filesystem::path(basePath) / "test_resources";
        std::filesystem::create_directories(testDir);
        
        // Create a test file in the directory
        std::filesystem::path testFile = testDir / "test.txt";
        std::ofstream file(testFile);
        file << "test content";
        file.close();
        
        // Clear mounts and test mounting the directory
        leo_ClearMounts();
        
        // Mount using relative path - should use our path helper
        bool mounted = leo_MountDirectory("test_resources", 100);
        REQUIRE(mounted == true);
        
        // Verify we can stat the file through the VFS
        leo_AssetInfo stat;
        bool found = leo_StatAsset("test.txt", &stat);
        REQUIRE(found == true);
        REQUIRE(stat.size > 0);
        
        // Clean up
        leo_ClearMounts();
        std::filesystem::remove_all(testDir);
        free(basePath);
    }
    
    SECTION("relative vs absolute path handling") {
        char* basePath = leo_GetResourceBasePath();
        REQUIRE(basePath != nullptr);
        
        leo_ClearMounts();
        
        // Test that relative paths use the base path
        // Create a test directory to verify the path construction works
        std::filesystem::path testDir = std::filesystem::path(basePath) / "test_relative";
        std::filesystem::create_directories(testDir);
        
        bool relativeResult = leo_MountDirectory("test_relative", 100);
        REQUIRE(relativeResult == true);
        
        // Clean up
        leo_ClearMounts();
        std::filesystem::remove_all(testDir);
        free(basePath);
    }
}

#endif // __APPLE__
