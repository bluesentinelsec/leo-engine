# Virtual File System (VFS)

## Overview

Leo Engine uses PhysFS to provide a unified virtual file system for game data.
Resources are mounted read-only at the VFS root (`/`). Writes go to a separate
PhysFS write directory (also mounted at `/` during write-dir operations).

Key properties:
- Fail-fast: all VFS errors throw `std::runtime_error`.
- SDL allocators: PhysFS and VFS buffers use `SDL_malloc`, `SDL_realloc`, and
  `SDL_free`.
- Read-only resources: callers should not write into the mounted resources.
- Write dir is initialized by `VFS` automatically.

## Configuration and Initialization

The VFS consumes an `engine::Config`:

```cpp
namespace engine {
    struct Config {
        const char* argv0;
        const char* resource_path; // Optional override
        const char* organization;  // e.g. "bluesentinelsec"
        const char* app_name;      // e.g. "leo-engine"
        void* (*malloc_fn)(size_t);   // Present but not used by VFS today
        void* (*realloc_fn)(void*, size_t);
        void  (*free_fn)(void*);
    };
}
```

Constructor behavior (`engine::VFS::VFS`):
1. If PhysFS is not initialized, the VFS sets PhysFS allocators to SDL and
   calls `PHYSFS_init(argv0)`.
2. If no write dir is set, VFS calls `PHYSFS_getPrefDir(organization, app_name)`
   and sets it as the write dir with `PHYSFS_setWriteDir`.
3. Mounts resources at `/`:
   - If `config.resource_path` is set, it is mounted.
   - Otherwise it tries `resources/`, then `resources.zip`.
4. If no resources mount succeeds, the constructor throws.

## Read API

### Read from mounted resources
```cpp
void ReadAll(const char* vfs_path, void** out_data, size_t* out_size);
```

- Reads from the PhysFS search path (mounted resources).
- Allocates with `SDL_malloc`; caller frees with `SDL_free`.
- Throws on any failure (open, length, read).

### Read from write dir only
```cpp
void ReadAllWriteDir(const char* vfs_path, void** out_data, size_t* out_size);
```

- Ensures the file resolves to the write directory (not resources).
- Uses the same allocation rules as `ReadAll`.
- Throws if the file is missing or not located in the write dir.

## Write API

```cpp
void WriteAll(const char* vfs_path, const void* data, size_t size);
```

- Writes to the PhysFS write dir only.
- Automatically creates parent directories via `PHYSFS_mkdir`.
- Throws on any failure (mkdir, open, write, close).
- `vfs_path` must be non-empty; `data` must be non-null for `size > 0`.

## Listing the Write Dir

### List immediate entries (PhysFS style)
```cpp
void ListWriteDir(const char* vfs_path, char*** out_entries);
void FreeList(char** entries) noexcept;
```

- Returns a null-terminated list of entry names in `vfs_path`.
- Output is filtered to entries that live in the write dir.
- Call `FreeList` to release the list.

### List all files with full relative paths
```cpp
void ListWriteDirFiles(char*** out_entries);
void FreeList(char** entries) noexcept;
```

- Recursively enumerates files under the write dir.
- Returns full relative paths like `saves/slot1.dat`.
- Directories are not returned, only files.
- Call `FreeList` to release the list.

## Error Handling

All VFS methods throw `std::runtime_error` on failure.
PhysFS-related failures include the PhysFS error string and the path when
available.

## Tutorial

### Setup
```cpp
engine::Config config = {
    .argv0 = argv[0],
    .resource_path = nullptr, // or set to a custom resource root
    .organization = "bluesentinelsec",
    .app_name = "leo-engine",
    .malloc_fn = SDL_malloc,
    .realloc_fn = SDL_realloc,
    .free_fn = SDL_free
};

engine::VFS vfs(config);
```

### Read a resource file
```cpp
void* data = nullptr;
size_t size = 0;
vfs.ReadAll("maps/map.json", &data, &size);
// ... use data/size ...
SDL_free(data);
```

### Write a save file
```cpp
const char* payload = "save-data";
vfs.WriteAll("saves/slot1.dat", payload, SDL_strlen(payload));
```

### Read back from the write dir
```cpp
void* save_data = nullptr;
size_t save_size = 0;
vfs.ReadAllWriteDir("saves/slot1.dat", &save_data, &save_size);
SDL_free(save_data);
```

### List the write dir
```cpp
char** entries = nullptr;
vfs.ListWriteDir("", &entries);
for (char** it = entries; *it; ++it) {
    SDL_Log("entry: %s", *it);
}
vfs.FreeList(entries);
```

### List all files with full paths
```cpp
char** files = nullptr;
vfs.ListWriteDirFiles(&files);
for (char** it = files; *it; ++it) {
    SDL_Log("file: %s", *it);
}
vfs.FreeList(files);
```

### Delete files and directories
```cpp
vfs.DeleteFile("saves/slot1.dat");
vfs.DeleteDirRecursive("recordings");
```

## Notes

- Paths use forward slashes (`/`) regardless of platform.
- The write dir location is platform-specific. Use `PHYSFS_getWriteDir()` to
  retrieve the resolved path for manual inspection.
- VFS does not support hot reloading or remounting at runtime.
