#include "leo/vfs.h"
#include <SDL3/SDL.h>
#include <physfs.h>
#include <stdexcept>
#include <filesystem>

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

static std::runtime_error MakeError(const char *action, const char *path)
{
    char *buffer = nullptr;
    if (path)
    {
        SDL_asprintf(&buffer, "%s '%s'", action, path);
    }
    else
    {
        SDL_asprintf(&buffer, "%s", action);
    }
    std::runtime_error err(buffer ? buffer : "Error");
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

static const char *GetWriteDirOrThrow()
{
    const char *write_dir = PHYSFS_getWriteDir();
    if (!write_dir)
    {
        throw std::runtime_error("Write directory is not set");
    }
    return write_dir;
}

static bool EnsureWriteDirMounted(const char *write_dir)
{
    const char *mount_point = PHYSFS_getMountPoint(write_dir);
    if (mount_point)
    {
        if (SDL_strcmp(mount_point, "/") != 0)
        {
            throw MakeError("Write directory mounted at unexpected mount point", mount_point);
        }
        return false;
    }

    if (!PHYSFS_mount(write_dir, "/", 0))
    {
        throw MakePhysfsError("Failed to mount write directory", write_dir);
    }

    return true;
}

class ScopedWriteDirMount
{
  public:
    explicit ScopedWriteDirMount(const char *write_dir) : write_dir(write_dir), mounted(false)
    {
        mounted = EnsureWriteDirMounted(write_dir);
    }

    ~ScopedWriteDirMount()
    {
        if (mounted)
        {
            PHYSFS_unmount(write_dir);
        }
    }

  private:
    const char *write_dir;
    bool mounted;
};

static char *CopyString(const char *src)
{
    size_t length = SDL_strlen(src);
    char *copy = static_cast<char *>(SDL_malloc(length + 1));
    if (!copy)
    {
        return nullptr;
    }
    SDL_memcpy(copy, src, length + 1);
    return copy;
}

static char *BuildChildPath(const char *dir, const char *name)
{
    char *path = nullptr;
    if (!dir || dir[0] == '\0')
    {
        SDL_asprintf(&path, "%s", name);
    }
    else if (dir[SDL_strlen(dir) - 1] == '/')
    {
        SDL_asprintf(&path, "%s%s", dir, name);
    }
    else
    {
        SDL_asprintf(&path, "%s/%s", dir, name);
    }

    return path;
}

static bool IsPathInWriteDir(const char *path, const char *write_dir)
{
    const char *real_dir = PHYSFS_getRealDir(path);
    if (!real_dir)
    {
        return false;
    }
    return SDL_strcmp(real_dir, write_dir) == 0;
}

static void EnsureWriteDirContains(const char *vfs_path, const char *write_dir, bool allow_empty)
{
    if (allow_empty && vfs_path[0] == '\0')
    {
        return;
    }

    if (!IsPathInWriteDir(vfs_path, write_dir))
    {
        throw MakeError("Write directory does not contain", vfs_path);
    }
}

static char **FilterWriteDirEntries(const char *dir, char **entries, const char *write_dir)
{
    size_t count = 0;

    for (char **it = entries; *it; ++it)
    {
        char *path = BuildChildPath(dir, *it);
        if (!path)
        {
            return nullptr;
        }
        bool in_write_dir = IsPathInWriteDir(path, write_dir);
        SDL_free(path);
        if (in_write_dir)
        {
            count++;
        }
    }

    char **filtered = static_cast<char **>(SDL_malloc(sizeof(char *) * (count + 1)));
    if (!filtered)
    {
        return nullptr;
    }

    size_t index = 0;
    for (char **it = entries; *it; ++it)
    {
        char *path = BuildChildPath(dir, *it);
        if (!path)
        {
            goto error;
        }
        bool in_write_dir = IsPathInWriteDir(path, write_dir);
        SDL_free(path);
        if (!in_write_dir)
        {
            continue;
        }

        filtered[index] = CopyString(*it);
        if (!filtered[index])
        {
            goto error;
        }
        index++;
    }

    filtered[index] = nullptr;
    return filtered;

error:
    for (size_t i = 0; i < index; ++i)
    {
        SDL_free(filtered[i]);
    }
    SDL_free(filtered);
    return nullptr;
}

static void FreeStringList(char **entries)
{
    if (!entries)
    {
        return;
    }
    for (char **it = entries; *it; ++it)
    {
        SDL_free(*it);
    }
    SDL_free(entries);
}

static bool EnsureListCapacity(char ***entries, size_t *capacity, size_t needed)
{
    if (*capacity >= needed)
    {
        return true;
    }

    size_t new_capacity = (*capacity == 0) ? 8 : *capacity;
    while (new_capacity < needed)
    {
        new_capacity *= 2;
    }

    char **resized = static_cast<char **>(SDL_realloc(*entries, sizeof(char *) * new_capacity));
    if (!resized)
    {
        return false;
    }

    *entries = resized;
    *capacity = new_capacity;
    return true;
}

static bool AppendListEntry(char ***entries, size_t *count, size_t *capacity, char *entry)
{
    if (!EnsureListCapacity(entries, capacity, *count + 2))
    {
        return false;
    }

    (*entries)[*count] = entry;
    (*count)++;
    (*entries)[*count] = nullptr;
    return true;
}

static void ListWriteDirFilesRecursive(const char *dir, const char *write_dir, char ***entries, size_t *count,
                                       size_t *capacity)
{
    char **list = PHYSFS_enumerateFiles(dir);
    if (!list)
    {
        throw MakePhysfsError("Failed to enumerate write directory", dir);
    }

    for (char **it = list; *it; ++it)
    {
        char *path = BuildChildPath(dir, *it);
        if (!path)
        {
            PHYSFS_freeList(list);
            throw std::runtime_error("ListWriteDirFiles failed to allocate path");
        }

        if (!IsPathInWriteDir(path, write_dir))
        {
            SDL_free(path);
            continue;
        }

        PHYSFS_Stat stat;
        if (!PHYSFS_stat(path, &stat))
        {
            std::runtime_error err = MakePhysfsError("Failed to stat", path);
            SDL_free(path);
            PHYSFS_freeList(list);
            throw err;
        }

        if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY)
        {
            try
            {
                ListWriteDirFilesRecursive(path, write_dir, entries, count, capacity);
            }
            catch (...)
            {
                SDL_free(path);
                PHYSFS_freeList(list);
                throw;
            }
            SDL_free(path);
            continue;
        }

        if (!AppendListEntry(entries, count, capacity, path))
        {
            SDL_free(path);
            PHYSFS_freeList(list);
            throw std::runtime_error("ListWriteDirFiles failed to allocate list");
        }
    }

    PHYSFS_freeList(list);
}

static void DeleteDirRecursiveInternal(const char *dir, const char *write_dir)
{
    char **list = PHYSFS_enumerateFiles(dir);
    if (!list)
    {
        throw MakePhysfsError("Failed to enumerate write directory", dir);
    }

    for (char **it = list; *it; ++it)
    {
        char *path = BuildChildPath(dir, *it);
        if (!path)
        {
            PHYSFS_freeList(list);
            throw std::runtime_error("DeleteDirRecursive failed to allocate path");
        }

        if (!IsPathInWriteDir(path, write_dir))
        {
            SDL_free(path);
            continue;
        }

        PHYSFS_Stat stat;
        if (!PHYSFS_stat(path, &stat))
        {
            std::runtime_error err = MakePhysfsError("Failed to stat", path);
            SDL_free(path);
            PHYSFS_freeList(list);
            throw err;
        }

        if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY)
        {
            try
            {
                DeleteDirRecursiveInternal(path, write_dir);
            }
            catch (...)
            {
                SDL_free(path);
                PHYSFS_freeList(list);
                throw;
            }
            SDL_free(path);
            continue;
        }

        if (!PHYSFS_delete(path))
        {
            std::runtime_error err = MakePhysfsError("Failed to delete file", path);
            SDL_free(path);
            PHYSFS_freeList(list);
            throw err;
        }

        SDL_free(path);
    }

    PHYSFS_freeList(list);

    if (!PHYSFS_delete(dir))
    {
        throw MakePhysfsError("Failed to delete directory", dir);
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
    }

    if (!PHYSFS_getWriteDir())
    {
        // Set up write directory using organization and app name
        const char *prefDir = PHYSFS_getPrefDir(config.organization, config.app_name);
        if (!prefDir)
        {
            if (initialized_physfs)
            {
                PHYSFS_deinit();
            }
            throw std::runtime_error("Failed to get write directory");
        }
        if (!PHYSFS_setWriteDir(prefDir))
        {
            if (initialized_physfs)
            {
                PHYSFS_deinit();
            }
            throw MakePhysfsError("Failed to set write directory", prefDir);
        }
    }

    // If resource_path is already set, use it
    if (config.resource_path != nullptr)
    {
        Mount(config.resource_path);
        return;
    }

    // Try to mount resources in order of preference
    std::error_code ec;
    bool has_resources_dir = std::filesystem::is_directory("resources", ec);
    bool has_resources_zip = std::filesystem::exists("resources.zip", ec);

    if (has_resources_dir && TryMount("."))
    {
        return;
    }

    if (has_resources_zip && TryMount("resources.zip"))
    {
        return;
    }

    if (TryMount("resources/"))
    {
        return;
    }

    // Neither found - fail
    if (initialized_physfs)
    {
        PHYSFS_deinit();
    }
    throw std::runtime_error("Failed to mount resources: tried '.', 'resources.zip', and 'resources/'");
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

void VFS::ReadAllWriteDir(const char *vfs_path, void **out_data, size_t *out_size)
{
    if (vfs_path == nullptr || out_data == nullptr || out_size == nullptr)
    {
        throw std::runtime_error("ReadAllWriteDir requires non-null arguments");
    }

    const char *write_dir = GetWriteDirOrThrow();
    ScopedWriteDirMount mount(write_dir);
    EnsureWriteDirContains(vfs_path, write_dir, false);
    ReadAll(vfs_path, out_data, out_size);
}

void VFS::WriteAll(const char *vfs_path, const void *data, size_t size)
{
    if (vfs_path == nullptr || vfs_path[0] == '\0')
    {
        throw std::runtime_error("WriteAll requires a non-empty path");
    }
    if (size > 0 && data == nullptr)
    {
        throw std::runtime_error("WriteAll requires data for non-zero size");
    }

    GetWriteDirOrThrow();

    PHYSFS_File *file = nullptr;
    char *dir_path = nullptr;
    const char *physfs_action = nullptr;
    bool alloc_failed = false;

    const char *slash = SDL_strrchr(vfs_path, '/');
    if (slash && slash != vfs_path)
    {
        size_t dir_len = static_cast<size_t>(slash - vfs_path);
        dir_path = static_cast<char *>(SDL_malloc(dir_len + 1));
        if (!dir_path)
        {
            alloc_failed = true;
            goto error;
        }
        SDL_memcpy(dir_path, vfs_path, dir_len);
        dir_path[dir_len] = '\0';

        if (!PHYSFS_mkdir(dir_path))
        {
            PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
            if (error != PHYSFS_ERR_DUPLICATE)
            {
                physfs_action = "Failed to create directory";
                goto error;
            }
        }
    }

    file = PHYSFS_openWrite(vfs_path);
    if (!file)
    {
        physfs_action = "Failed to open for write";
        goto error;
    }

    if (size > 0)
    {
        if (PHYSFS_writeBytes(file, data, static_cast<PHYSFS_uint64>(size)) != static_cast<PHYSFS_sint64>(size))
        {
            physfs_action = "Failed to write";
            goto error;
        }
    }

    if (!PHYSFS_close(file))
    {
        file = nullptr;
        physfs_action = "Failed to close";
        goto error;
    }
    file = nullptr;

    SDL_free(dir_path);
    return;

error:
    ClosePhysfsFile(file);
    if (dir_path)
    {
        SDL_free(dir_path);
    }
    if (alloc_failed)
    {
        throw std::runtime_error("WriteAll failed to allocate buffer");
    }
    throw MakePhysfsError(physfs_action, vfs_path);
}

void VFS::ListWriteDir(const char *vfs_path, char ***out_entries)
{
    if (vfs_path == nullptr || out_entries == nullptr)
    {
        throw std::runtime_error("ListWriteDir requires non-null arguments");
    }

    const char *write_dir = GetWriteDirOrThrow();
    ScopedWriteDirMount mount(write_dir);
    char **entries = nullptr;
    char **filtered = nullptr;

    EnsureWriteDirContains(vfs_path, write_dir, true);

    entries = PHYSFS_enumerateFiles(vfs_path);
    if (!entries)
    {
        throw MakePhysfsError("Failed to enumerate write directory", vfs_path);
    }

    filtered = FilterWriteDirEntries(vfs_path, entries, write_dir);
    PHYSFS_freeList(entries);
    if (!filtered)
    {
        throw std::runtime_error("ListWriteDir failed to allocate list");
    }
    *out_entries = filtered;
}

void VFS::ListWriteDirFiles(char ***out_entries)
{
    if (out_entries == nullptr)
    {
        throw std::runtime_error("ListWriteDirFiles requires non-null arguments");
    }

    const char *write_dir = GetWriteDirOrThrow();
    char **entries = nullptr;
    size_t count = 0;
    size_t capacity = 0;

    ScopedWriteDirMount mount(write_dir);

    entries = static_cast<char **>(SDL_malloc(sizeof(char *)));
    if (!entries)
    {
        throw std::runtime_error("ListWriteDirFiles failed to allocate list");
    }
    entries[0] = nullptr;
    capacity = 1;

    try
    {
        ListWriteDirFilesRecursive("", write_dir, &entries, &count, &capacity);
    }
    catch (...)
    {
        FreeStringList(entries);
        throw;
    }

    *out_entries = entries;
}

void VFS::DeleteFile(const char *vfs_path)
{
    if (vfs_path == nullptr || vfs_path[0] == '\0')
    {
        throw std::runtime_error("DeleteFile requires a non-empty path");
    }

    const char *write_dir = GetWriteDirOrThrow();
    PHYSFS_Stat stat;

    ScopedWriteDirMount mount(write_dir);
    EnsureWriteDirContains(vfs_path, write_dir, false);

    if (!PHYSFS_stat(vfs_path, &stat))
    {
        throw MakePhysfsError("Failed to stat", vfs_path);
    }

    if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY)
    {
        throw MakeError("DeleteFile requires a file", vfs_path);
    }

    if (!PHYSFS_delete(vfs_path))
    {
        throw MakePhysfsError("Failed to delete file", vfs_path);
    }
}

void VFS::DeleteDirRecursive(const char *vfs_path)
{
    if (vfs_path == nullptr || vfs_path[0] == '\0')
    {
        throw std::runtime_error("DeleteDirRecursive requires a non-empty path");
    }

    const char *write_dir = GetWriteDirOrThrow();
    PHYSFS_Stat stat;

    ScopedWriteDirMount mount(write_dir);
    EnsureWriteDirContains(vfs_path, write_dir, false);

    if (!PHYSFS_stat(vfs_path, &stat))
    {
        throw MakePhysfsError("Failed to stat", vfs_path);
    }

    if (stat.filetype != PHYSFS_FILETYPE_DIRECTORY)
    {
        throw MakeError("DeleteDirRecursive requires a directory", vfs_path);
    }

    DeleteDirRecursiveInternal(vfs_path, write_dir);
}

void VFS::FreeList(char **entries) noexcept
{
    if (entries)
    {
        PHYSFS_freeList(entries);
    }
}

} // namespace engine
