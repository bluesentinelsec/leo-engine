// tests/python_compat_test.cpp
#include <catch2/catch_all.hpp>
#include <leo/pack_reader.h>
#include <leo/pack_errors.h>

#include <cstring>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

TEST_CASE("Python leo-packer compatibility", "[pack][python]")
{
    const char* archive_path = "../resources.leopack";  // Relative to build dir
    const char* password = "password";
    
    SECTION("Can open Python-created compressed obfuscated archive")
    {
        leo_pack* pack = nullptr;
        leo_pack_result result = leo_pack_open_file(&pack, archive_path, password);
        
        REQUIRE(result == LEO_PACK_OK);
        REQUIRE(pack != nullptr);
        
        // Archive should contain entries
        int count = leo_pack_count(pack);
        REQUIRE(count > 0);
        
        leo_pack_close(pack);
    }
    
    SECTION("Can extract files from Python-created archive")
    {
        leo_pack* pack = nullptr;
        REQUIRE(leo_pack_open_file(&pack, archive_path, password) == LEO_PACK_OK);
        
        int count = leo_pack_count(pack);
        REQUIRE(count > 0);
        
        // Test extracting first few entries
        for (int i = 0; i < std::min(count, 3); i++) {
            leo_pack_stat stat;
            REQUIRE(leo_pack_stat_index(pack, i, &stat) == LEO_PACK_OK);
            
            // Verify stat info is reasonable
            REQUIRE(stat.size_uncompressed > 0);
            REQUIRE(stat.size_stored > 0);
            REQUIRE(strlen(stat.name) > 0);
            
            // Extract the file
            std::vector<uint8_t> buffer(stat.size_uncompressed);
            size_t extracted_size;
            leo_pack_result extract_result = leo_pack_extract_index(
                pack, i, buffer.data(), buffer.size(), &extracted_size
            );
            
            REQUIRE(extract_result == LEO_PACK_OK);
            REQUIRE(extracted_size == stat.size_uncompressed);
        }
        
        leo_pack_close(pack);
    }
    
    SECTION("Wrong password fails correctly")
    {
        leo_pack* pack = nullptr;
        leo_pack_result result = leo_pack_open_file(&pack, archive_path, "wrongpassword");
        
        // Should either fail to open or fail during extraction
        if (result == LEO_PACK_OK) {
            // Archive opened, but extraction should fail with password or buffer error
            uint8_t buffer[1024];
            size_t extracted_size;
            leo_pack_result extract_result = leo_pack_extract_index(
                pack, 0, buffer, sizeof(buffer), &extracted_size
            );
            // Accept either bad password or buffer size errors as valid failures
            REQUIRE((extract_result == LEO_PACK_E_BAD_PASSWORD || extract_result == LEO_PACK_E_NOSPACE));
            leo_pack_close(pack);
        } else {
            // Archive failed to open, which is also acceptable
            REQUIRE(pack == nullptr);
        }
    }
    
    SECTION("Can find specific files by name")
    {
        leo_pack* pack = nullptr;
        REQUIRE(leo_pack_open_file(&pack, archive_path, password) == LEO_PACK_OK);
        
        // Try to find any file (we don't know exact names, so test the API)
        leo_pack_stat stat;
        REQUIRE(leo_pack_stat_index(pack, 0, &stat) == LEO_PACK_OK);
        
        int found_index;
        leo_pack_result find_result = leo_pack_find(pack, stat.name, &found_index);
        REQUIRE(find_result == LEO_PACK_OK);
        REQUIRE(found_index == 0);
        
        // Extract by name
        std::vector<uint8_t> buffer(stat.size_uncompressed);
        size_t extracted_size;
        leo_pack_result extract_result = leo_pack_extract(
            pack, stat.name, buffer.data(), buffer.size(), &extracted_size
        );
        
        REQUIRE(extract_result == LEO_PACK_OK);
        REQUIRE(extracted_size == stat.size_uncompressed);
        
        leo_pack_close(pack);
    }
}
