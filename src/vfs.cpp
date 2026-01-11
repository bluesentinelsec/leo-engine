#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <physfs.h>
#include <stdexcept>

namespace engine {

static void* PHYSFS_Alloc(PHYSFS_uint64 size) {
    return SDL_malloc(static_cast<size_t>(size));
}

static void* PHYSFS_Realloc(void* ptr, PHYSFS_uint64 size) {
    return SDL_realloc(ptr, static_cast<size_t>(size));
}

static void PHYSFS_Free(void* ptr) {
    SDL_free(ptr);
}

void VFS::ConfigureAllocator() {
    PHYSFS_Allocator allocator;
    allocator.Init = nullptr;
    allocator.Deinit = nullptr;
    allocator.Malloc = PHYSFS_Alloc;
    allocator.Realloc = PHYSFS_Realloc;
    allocator.Free = PHYSFS_Free;
    
    if (!PHYSFS_setAllocator(&allocator)) {
        throw std::runtime_error("Failed to set PhysFS allocator");
    }
}

bool VFS::TryMount(const char* path) {
    SDL_Log("Attempting to mount: %s", path);
    
    if (PHYSFS_mount(path, "/", 1)) {
        SDL_Log("Successfully mounted: %s", path);
        config.resource_path = path;
        return true;
    }
    
    return false;
}

void VFS::Mount(const char* path) {
    if (!TryMount(path)) {
        PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
        const char* errorMsg = PHYSFS_getErrorByCode(error);
        
        // Use SDL_asprintf for safe string formatting
        char* buffer = nullptr;
        SDL_asprintf(&buffer, "Failed to mount '%s': %s", path, errorMsg);
        std::runtime_error err(buffer ? buffer : "Failed to mount resource");
        SDL_free(buffer);
        throw err;
    }
}

VFS::VFS(Config& cfg) : config(cfg), initialized_physfs(false) {
    // Initialize PhysFS (skip if already initialized from previous test)
    if (!PHYSFS_isInit()) {
        ConfigureAllocator();
        
        if (!PHYSFS_init(config.argv0)) {
            PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
            const char* errorMsg = PHYSFS_getErrorByCode(error);
            
            // Use SDL_asprintf for safe string formatting
            char* buffer = nullptr;
            SDL_asprintf(&buffer, "Failed to initialize PhysFS: %s", errorMsg);
            std::runtime_error err(buffer ? buffer : "Failed to initialize PhysFS");
            SDL_free(buffer);
            throw err;
        }
        
        initialized_physfs = true;
        
        // Set up write directory using organization and app name
        const char* prefDir = PHYSFS_getPrefDir(config.organization, config.app_name);
        if (prefDir) {
            PHYSFS_setWriteDir(prefDir);
        }
    }
    
    // If resource_path is already set, use it
    if (config.resource_path != nullptr) {
        Mount(config.resource_path);
        return;
    }
    
    // Try to mount resources in order of preference
    if (TryMount("resources/")) return;
    if (TryMount("resources.zip")) return;
    if (TryMount("../resources/")) return;
    if (TryMount("../resources.zip")) return;
    
    // Neither found - fail
    if (initialized_physfs) {
        PHYSFS_deinit();
    }
    throw std::runtime_error(
        "Failed to mount resources: tried 'resources/', '../resources/', 'resources.zip', '../resources.zip'"
    );
}

VFS::~VFS() {
    if (initialized_physfs) {
        PHYSFS_deinit();
    }
}

} // namespace engine
