#include "leo/engine_config.h"
#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <catch2/catch_test_macros.hpp>
#include <physfs.h>

namespace
{

struct SDLGuard
{
    SDLGuard()
    {
        SDL_Init(0);
    }

    ~SDLGuard()
    {
        SDL_Quit();
    }
};

engine::Config MakeConfig(const char *resource_path = ".")
{
    return {.argv0 = "test",
            .resource_path = resource_path,
            .script_path = nullptr,
            .organization = "bluesentinelsec",
            .app_name = "leo-engine",
            .malloc_fn = SDL_malloc,
            .realloc_fn = SDL_realloc,
            .free_fn = SDL_free};
}

} // namespace

TEST_CASE("VFS mounts configured resource root", "[vfs]")
{
    SDLGuard sdl;
    engine::Config config = MakeConfig();

    {
        engine::VFS vfs(config);

        REQUIRE(config.resource_path != nullptr);
        REQUIRE(SDL_strcmp(config.resource_path, ".") == 0);
    }
}

TEST_CASE("VFS can read file from mounted resources", "[vfs]")
{
    SDLGuard sdl;
    engine::Config config = MakeConfig();

    {
        engine::VFS vfs(config);

        void *data = nullptr;
        size_t size = 0;
        vfs.ReadAll("resources/maps/map.json", &data, &size);
        REQUIRE(data != nullptr);
        REQUIRE(size > 0);
        SDL_free(data);
    }
}

TEST_CASE("VFS ReadAll throws when file is missing", "[vfs]")
{
    SDLGuard sdl;
    engine::Config config = MakeConfig();

    {
        engine::VFS vfs(config);
        void *data = nullptr;
        size_t size = 0;
        REQUIRE_THROWS_AS(vfs.ReadAll("missing.file", &data, &size), std::runtime_error);
    }
}

TEST_CASE("VFS can write, list, and read from write dir", "[vfs]")
{
    SDLGuard sdl;
    engine::Config config = MakeConfig();

    {
        engine::VFS vfs(config);

        const char *payload = "write-dir-test";
        size_t payload_size = SDL_strlen(payload);

        vfs.WriteAll("saves/vfs_write_test.txt", payload, payload_size);

        char **entries = nullptr;
        vfs.ListWriteDir("saves", &entries);
        REQUIRE(entries != nullptr);

        bool found = false;
        for (char **it = entries; *it; ++it)
        {
            if (SDL_strcmp(*it, "vfs_write_test.txt") == 0)
            {
                found = true;
                break;
            }
        }
        vfs.FreeList(entries);
        REQUIRE(found);

        void *data = nullptr;
        size_t size = 0;
        vfs.ReadAllWriteDir("saves/vfs_write_test.txt", &data, &size);
        REQUIRE(size == payload_size);
        REQUIRE(SDL_memcmp(data, payload, size) == 0);
        SDL_free(data);
    }
}

TEST_CASE("VFS can list write dir files with relative paths", "[vfs]")
{
    SDLGuard sdl;
    engine::Config config = MakeConfig();

    {
        engine::VFS vfs(config);

        const char *payload = "list-write-files";
        size_t payload_size = SDL_strlen(payload);

        vfs.WriteAll("saves/vfs_list_slot1.dat", payload, payload_size);
        vfs.WriteAll("config/vfs_list_settings.json", payload, payload_size);
        vfs.WriteAll("recordings/vfs_list_run1.demo", payload, payload_size);

        char **entries = nullptr;
        vfs.ListWriteDirFiles(&entries);
        REQUIRE(entries != nullptr);

        bool found_save = false;
        bool found_config = false;
        bool found_recording = false;
        for (char **it = entries; *it; ++it)
        {
            if (SDL_strcmp(*it, "saves/vfs_list_slot1.dat") == 0)
            {
                found_save = true;
            }
            else if (SDL_strcmp(*it, "config/vfs_list_settings.json") == 0)
            {
                found_config = true;
            }
            else if (SDL_strcmp(*it, "recordings/vfs_list_run1.demo") == 0)
            {
                found_recording = true;
            }
        }
        vfs.FreeList(entries);

        REQUIRE(found_save);
        REQUIRE(found_config);
        REQUIRE(found_recording);
    }
}

TEST_CASE("VFS can delete files and directories in write dir", "[vfs]")
{
    SDLGuard sdl;
    engine::Config config = MakeConfig();

    {
        engine::VFS vfs(config);

        const char *payload = "delete-test";
        size_t payload_size = SDL_strlen(payload);

        vfs.WriteAll("delete_test/one.txt", payload, payload_size);
        vfs.WriteAll("delete_test/subdir/two.txt", payload, payload_size);

        vfs.DeleteFile("delete_test/one.txt");

        void *data = nullptr;
        size_t size = 0;
        REQUIRE_THROWS_AS(vfs.ReadAllWriteDir("delete_test/one.txt", &data, &size), std::runtime_error);

        vfs.DeleteDirRecursive("delete_test/subdir");
        char **entries = nullptr;
        REQUIRE_THROWS_AS(vfs.ListWriteDir("delete_test/subdir", &entries), std::runtime_error);

        vfs.DeleteDirRecursive("delete_test");
        REQUIRE_THROWS_AS(vfs.ListWriteDir("delete_test", &entries), std::runtime_error);

        REQUIRE_THROWS_AS(vfs.DeleteFile("resources/maps/map.json"), std::runtime_error);
    }
}

TEST_CASE("VFS ListWriteDir throws when directory is missing", "[vfs]")
{
    SDLGuard sdl;
    engine::Config config = MakeConfig();

    {
        engine::VFS vfs(config);
        char **entries = nullptr;
        REQUIRE_THROWS_AS(vfs.ListWriteDir("missing-dir", &entries), std::runtime_error);
    }
}

TEST_CASE("VFS throws exception when resources not found", "[vfs]")
{
    SDLGuard sdl;
    engine::Config config = MakeConfig("nonexistent/");

    REQUIRE_THROWS_AS(engine::VFS(config), std::runtime_error);
}
