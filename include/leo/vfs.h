#ifndef LEO_VFS_H
#define LEO_VFS_H

#include "engine_config.h"

namespace engine
{

class VFS
{
  public:
    // Initialize PhysFS with SDL3 allocators and mount resources
    // Throws exception on failure
    VFS(Config &config);

    // Destructor calls PHYSFS_deinit()
    ~VFS();

    // Delete copy/move to enforce single instance
    VFS(const VFS &) = delete;
    VFS &operator=(const VFS &) = delete;

    // Read entire file into SDL-allocated buffer. Caller frees via SDL_free.
    void ReadAll(const char *vfs_path, void **out_data, size_t *out_size);

    // Read file from write directory only. Caller frees via SDL_free.
    void ReadAllWriteDir(const char *vfs_path, void **out_data, size_t *out_size);

    // Write entire buffer to write directory, creating parent dirs as needed.
    void WriteAll(const char *vfs_path, const void *data, size_t size);

    // List entries in the write directory (PhysFS-style list).
    void ListWriteDir(const char *vfs_path, char ***out_entries);

    // List files under the write directory, returning full relative paths.
    void ListWriteDirFiles(char ***out_entries);

    // Free a list returned by ListWriteDir or ListWriteDirFiles.
    void FreeList(char **entries) noexcept;

  private:
    Config &config;
    bool initialized_physfs; // Track if this instance initialized PhysFS

    // Helper: Try to mount a path, returns true on success, false on failure
    // Does not throw
    bool TryMount(const char *path);

    // Helper: Mount a path, throws exception on failure
    void Mount(const char *path);

    // Helper: Set up SDL3 memory allocators for PhysFS
    void ConfigureAllocator();
};

} // namespace engine

#endif // LEO_VFS_H
