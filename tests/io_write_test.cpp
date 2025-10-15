// tests/io_write_test.cpp
#include <catch2/catch_all.hpp>
#include <cstring>
#include <fstream>

extern "C"
{
#include "leo/error.h"
#include "leo/io.h"
}

TEST_CASE("leo_WriteFile: creates file with data", "[io_write]")
{
    const char *test_data = "Hello Leo Engine!";
    size_t data_size = strlen(test_data);

    bool result = leo_WriteFile("test.txt", test_data, data_size);
    REQUIRE(result == true);

    // Verify we can read the data back using leo_GetWriteDirectory
    char *write_dir = leo_GetWriteDirectory("Leo", "Engine");
    REQUIRE(write_dir != nullptr);

    std::string full_path = std::string(write_dir) + "test.txt";
    INFO("Trying to read file at: " << full_path);
    std::ifstream file(full_path, std::ios::binary);
    REQUIRE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    REQUIRE(content == test_data);

    file.close();
    free(write_dir);
}

TEST_CASE("leo_WriteFile: creates subdirectories automatically", "[io_write]")
{
    const char *test_data = "save_data";

    bool result = leo_WriteFile("saves/game1.dat", test_data, strlen(test_data));
    REQUIRE(result == true);
}

TEST_CASE("leo_GetWriteDirectory: returns valid path", "[io_write]")
{
    char *path = leo_GetWriteDirectory("Leo", "Engine");
    REQUIRE(path != nullptr);
    REQUIRE(strlen(path) > 0);
    free(path);
}

TEST_CASE("leo_WriteFile: handles empty data", "[io_write]")
{
    bool result = leo_WriteFile("empty.txt", "", 0);
    REQUIRE(result == true);
}

TEST_CASE("leo_WriteFile: fails with null parameters", "[io_write]")
{
    bool result = leo_WriteFile(nullptr, "data", 4);
    REQUIRE(result == false);

    result = leo_WriteFile("test.txt", nullptr, 4);
    REQUIRE(result == false);
}

TEST_CASE("leo_ReadFile: reads back written data", "[io_write]")
{
    const char *test_data = "Save game data: level=10, coins=500";
    size_t data_size = strlen(test_data);

    // Write the data
    bool write_result = leo_WriteFile("save.dat", test_data, data_size);
    REQUIRE(write_result == true);

    // Read it back
    size_t read_size = 0;
    void *read_data = leo_ReadFile("save.dat", &read_size);
    REQUIRE(read_data != nullptr);
    REQUIRE(read_size == data_size);

    // Verify content matches
    REQUIRE(memcmp(read_data, test_data, data_size) == 0);

    free(read_data);
}

TEST_CASE("leo_ReadFile: returns null for non-existent file", "[io_write]")
{
    size_t size = 0;
    void *data = leo_ReadFile("nonexistent.txt", &size);
    REQUIRE(data == nullptr);
    REQUIRE(size == 0);
}
