#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <physfs.h>
#include <stdexcept>

namespace engine
{

static void *PHYSFS_Alloc(PHYSFS_uint64 size)
{
    return SDL_malloc(static_cast<size_t>(size));
}

static void *PHYSFS_Realloc(void *ptr, PHYSFS_uint64 size)
{
    return SDL_realloc(ptr, static_cast<size_t>(size));
}

static void PHYSFS_Free(void *ptr)
{
    SDL_free(ptr);
}

static std::runtime_error MakePhysfsError(const char *action, const char *path)
{
    PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
    const char *errorMsg = PHYSFS_getErrorByCode(error);
    char *buffer = nullptr;
    if (path)
    {
        SDL_asprintf(&buffer, "%s '%s': %s", action, path, errorMsg);
    }
    else
    {
        SDL_asprintf(&buffer, "%s: %s", action, errorMsg);
    }
    std::runtime_error err(buffer ? buffer : "PhysFS error");
    SDL_free(buffer);
    return err;
}

static void ClosePhysfsFile(PHYSFS_File *file)
{
    if (file)
    {
        PHYSFS_close(file);
    }
}

void VFS::ConfigureAllocator()
{
    PHYSFS_Allocator allocator{};
    allocator.Malloc = PHYSFS_Alloc;
    allocator.Realloc = PHYSFS_Realloc;
    allocator.Free = PHYSFS_Free;

    if (!PHYSFS_setAllocator(&allocator))
    {
        throw std::runtime_error("Failed to set PhysFS allocator");
    }
}

bool VFS::TryMount(const char *path)
{
    SDL_Log("Attempting to mount: %s", path);

    if (PHYSFS_mount(path, "/", 1))
    {
        SDL_Log("Successfully mounted: %s", path);
        config.resource_path = path;
        return true;
    }

    return false;
}

void VFS::Mount(const char *path)
{
    if (!TryMount(path))
    {
        throw MakePhysfsError("Failed to mount", path);
    }
}

VFS::VFS(Config &cfg) : config(cfg), initialized_physfs(false)
{
    // Initialize PhysFS (skip if already initialized from previous test)
    if (!PHYSFS_isInit())
    {
        ConfigureAllocator();

        if (!PHYSFS_init(config.argv0))
        {
            throw MakePhysfsError("Failed to initialize PhysFS", nullptr);
        }

        initialized_physfs = true;

        // Set up write directory using organization and app name
        const char *prefDir = PHYSFS_getPrefDir(config.organization, config.app_name);
        if (prefDir)
        {
            PHYSFS_setWriteDir(prefDir);
        }
    }

    // If resource_path is already set, use it
    if (config.resource_path != nullptr)
    {
        Mount(config.resource_path);
        return;
    }

    // Try to mount resources in order of preference
    static const char *kDefaultResourcePaths[] = {"resources/", "resources.zip"};
    for (const char *path : kDefaultResourcePaths)
    {
        if (TryMount(path))
        {
            return;
        }
    }

    // Neither found - fail
    if (initialized_physfs)
    {
        PHYSFS_deinit();
    }
    throw std::runtime_error("Failed to mount resources: tried 'resources/' and 'resources.zip'");
}

VFS::~VFS()
{
    if (initialized_physfs)
    {
        PHYSFS_deinit();
    }
}

void VFS::ReadAll(const char *vfs_path, void **out_data, size_t *out_size)
{
    if (vfs_path == nullptr || out_data == nullptr || out_size == nullptr)
    {
        throw std::runtime_error("ReadAll requires non-null arguments");
    }

    PHYSFS_File *file = nullptr;
    void *buffer = nullptr;
    PHYSFS_sint64 length = 0;
    const char *physfs_action = nullptr;
    bool alloc_failed = false;

    file = PHYSFS_openRead(vfs_path);
    if (!file)
    {
        physfs_action = "Failed to open";
        goto error;
    }

    length = PHYSFS_fileLength(file);
    if (length < 0)
    {
        physfs_action = "Failed to get file length for";
        goto error;
    }

    if (length == 0)
    {
        *out_data = nullptr;
        *out_size = 0;
        goto cleanup;
    }

    buffer = SDL_malloc(static_cast<size_t>(length));
    if (!buffer)
    {
        alloc_failed = true;
        goto error;
    }

    if (PHYSFS_readBytes(file, buffer, length) != length)
    {
        physfs_action = "Failed to read";
        goto error;
    }

    *out_data = buffer;
    *out_size = static_cast<size_t>(length);
    goto cleanup;

error:
    if (buffer)
    {
        SDL_free(buffer);
    }
    ClosePhysfsFile(file);
    if (alloc_failed)
    {
        throw std::runtime_error("ReadAll failed to allocate buffer");
    }
    throw MakePhysfsError(physfs_action, vfs_path);

cleanup:
    ClosePhysfsFile(file);
}

} // namespace engine
