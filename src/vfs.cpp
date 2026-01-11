#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <physfs.h>
#include <stdexcept>

namespace engine {

// Static pointer to config for allocator callbacks
static Config* g_allocator_config = nullptr;

static void* PHYSFS_Alloc(PHYSFS_uint64 size) {
    return g_allocator_config->malloc_fn(static_cast<size_t>(size));
}

static void* PHYSFS_Realloc(void* ptr, PHYSFS_uint64 size) {
    return g_allocator_config->realloc_fn(ptr, static_cast<size_t>(size));
}

static void PHYSFS_Free(void* ptr) {
    g_allocator_config->free_fn(ptr);
}

void VFS::ConfigureAllocator() {
    // Only set allocator if not already configured
    static bool allocator_configured = false;
    if (allocator_configured) {
        return;
    }
    
    g_allocator_config = &config;
    
    PHYSFS_Allocator allocator;
    allocator.Init = nullptr;
    allocator.Deinit = nullptr;
    allocator.Malloc = PHYSFS_Alloc;
    allocator.Realloc = PHYSFS_Realloc;
    allocator.Free = PHYSFS_Free;
    
    if (!PHYSFS_setAllocator(&allocator)) {
        throw std::runtime_error("Failed to set PhysFS allocator");
    }
    
    allocator_configured = true;
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

VFS::VFS(Config& cfg) : config(cfg) {
    ConfigureAllocator();
    
    // Initialize PhysFS (skip if already initialized from previous test)
    if (!PHYSFS_isInit()) {
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
    PHYSFS_deinit();
    throw std::runtime_error(
        "Failed to mount resources: tried 'resources/', '../resources/', 'resources.zip', '../resources.zip'"
    );
}

VFS::~VFS() {
    PHYSFS_deinit();
}

} // namespace engine
