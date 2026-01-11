# Virtual File System (VFS) Design

## Overview

Leo Engine uses PhysFS to provide a unified virtual file system that abstracts access to game resources. The VFS supports mounting directories during development and ZIP archives for production builds.

## Requirements

### Default Behavior
- Attempt to mount `resources/` (directory) first, then `resources.zip` (archive)
- Mount at root `/` - single mount point only
- If neither mount succeeds, throw a fatal exception with clear error messaging
- Use INFO console logs to indicate what we're trying to mount (SDL3 logging functions)

### CLI Override
- `--resource-dir <path>` or `-r <path>` to override the default resource location
- Supports both directories and archives
- Accepts relative paths (resolved from current working directory) and absolute paths

### Error Handling
- Fail-fast philosophy: mount failures are fatal
- Exception message must include:
  - The path that was attempted
  - The PhysFS error message explaining why it failed

### Memory Management
- Configure PhysFS to use SDL3 memory allocators via `PHYSFS_Allocator`
- Use `SDL_malloc`, `SDL_realloc`, `SDL_free`

### PhysFS Configuration
- Use `PHYSFS_setSaneConfig()` with:
  - Organization: `"bluesentinelsec"`
  - App name: `"leo-engine"`
  - Archive extension: `"zip"`
  - Include CD-ROMs: `0` (false)
  - Archives first: `0` (false - prefer directories)

### Engine Configuration
- Create a centralized `EngineConfig` struct in the `engine` namespace
- Store in `EngineConfig`:
  - `argv[0]` - for subsystems that need it
  - Mounted resource path
  - PhysFS configuration defaults (organization, app name)
- `EngineConfig` will be extended as more subsystems are added
- `EngineConfig` is owned by `main.cpp` and passed to subsystems (no singleton)

## Proposed Implementation

### Source Files

```
include/
  leo/engine_config.h       # EngineConfig struct definition
  leo/vfs.h          # VFS class definition

src/
  vfs.cpp        # VFS implementation
  
tests/
  test_vfs.cpp          # VFS unit tests
```

### Classes and Structs

```cpp
// leo/engine_config.h
namespace engine {
    struct Config {
        const char* argv0;                    // argv[0] from main
        const char* resource_path;            // Path to mounted resource directory/archive
        const char* organization;             // "bluesentinelsec"
        const char* app_name;                 // "leo-engine"
        
        // Memory allocation functions (default to SDL3)
        void* (*malloc_fn)(size_t);           // Default: SDL_malloc
        void* (*realloc_fn)(void*, size_t);   // Default: SDL_realloc
        void  (*free_fn)(void*);              // Default: SDL_free
        
        // Future: audio settings, window config, etc.
    };
}

// leo/vfs.h
namespace engine {
    class VFS {
    public:
        // Initialize PhysFS with SDL3 allocators and mount resources
        // Throws exception on failure
        VFS(Config& config);
        
        // Destructor calls PHYSFS_deinit()
        ~VFS();
        
        // Delete copy/move to enforce single instance
        VFS(const VFS&) = delete;
        VFS& operator=(const VFS&) = delete;
        
    private:
        // Helper: Try to mount a path (directory or archive)
        // Throws exception on failure with detailed error message
        void Mount(const char* path);
        
        // Helper: Set up SDL3 memory allocators for PhysFS
        void ConfigureAllocator();
    };
}
```

### Test-Driven Development Approach

Tests use real files in `resources/` and `resources.zip` present in the project root.

1. **Successful directory mount**
   - Given: `resources/` directory exists
   - When: VFS initializes with default settings
   - Then: PhysFS mounts successfully, `config.resource_path` is set to `"resources/"`

2. **Successful archive mount**
   - Given: `resources.zip` exists, no `resources/` directory
   - When: VFS initializes with default settings
   - Then: PhysFS mounts the archive, `config.resource_path` is set to `"resources.zip"`

3. **Mount failure throws exception**
   - Given: Neither `resources/` nor `resources.zip` exists
   - When: VFS initializes
   - Then: Exception is thrown with path and PhysFS error message

4. **File reading through VFS**
   - Given: A file exists in the mounted resource path
   - When: File is read through PhysFS
   - Then: File contents are accessible

**Deferred tests:**
- CLI override (will be tested when CLI integration is added)
- Memory allocator configuration (assume PhysFS library works correctly)

## Design Decisions

### 1. Mount Point
Mount at root `/` with single mount point. First try `resources/` (directory), then `resources.zip` (archive).

### 2. Search Path Priority
Directory takes priority over archive. If both exist, mount the directory.

### 3. Write Directory
Write access will be needed for save files/logs, but implementation is deferred. PhysFS handles this separately from read-only mounts.

### 4. Archive Format
ZIP only. PhysFS may support other formats at build time, which is acceptable.

### 5. Initialization Timing
VFS initializes after SDL3 and CLI argument parsing. Integration with `main.cpp` is deferred.

### 6. Resource Path Validation
No validation. If the directory/archive exists, mount it.

### 7. Hot Reloading
Not supported. No unmounting/remounting at runtime.

### 8. Error Recovery
Try `resources/` then `resources.zip`. Throw fatal error if neither is found.

### 9. Relative vs Absolute Paths
Both supported. Relative paths resolved from current working directory (shell handles this).

### 10. EngineConfig Lifetime
No singleton. `EngineConfig` owned by `main.cpp` and passed to subsystems.

## Integration Notes (Deferred)

VFS initialization will happen in `main.cpp` after:
1. SDL3 initialization (for memory allocators)
2. CLI argument parsing (to get `--resource-dir` if provided)

Example future integration:
```cpp
int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    
    CLI::App app{"Leo Engine"};
    std::string resource_dir;
    app.add_option("-r,--resource-dir", resource_dir, "Resource directory or archive");
    app.parse(argc, argv);
    
    engine::Config config = {
        .argv0 = argv[0],
        .organization = "bluesentinelsec",
        .app_name = "leo-engine",
        .malloc_fn = SDL_malloc,
        .realloc_fn = SDL_realloc,
        .free_fn = SDL_free
    };
    
    engine::VFS vfs(config);
    
    // ... rest of engine initialization
    
    // VFS destructor calls PHYSFS_deinit()
    SDL_Quit();
}
```

## Implementation Notes

- PhysFS requires `PHYSFS_init(argv[0])` to determine the base directory
- Store `argv[0]` in `EngineConfig` for subsystems that need it
- PhysFS automatically handles platform-specific path separators
- All PhysFS functions return error codes that can be queried via `PHYSFS_getLastErrorCode()` and `PHYSFS_getErrorByCode()`
- Memory allocator must be set before `PHYSFS_init()` is called
- Use SDL3 functions for memory allocation and standard library replacements throughout VFS implementation