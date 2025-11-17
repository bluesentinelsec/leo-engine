/* ==========================================================
 * File: src/io.c  (thread-safe VFS with SDL3 RWLock)
 * ========================================================== */

#include "leo/io.h"
#include "leo/error.h"
#include "leo/macos_path_helper.h"
#include "leo/pack_format.h"
#include "leo/pack_reader.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>

/* -----------------------------
   Internal mount table
----------------------------- */
typedef struct
{
    leo_MountType type;
    int priority;
    void *impl; /* leo_pack* for PACK, char* baseDir for DIR */
} _MountRec;

static _MountRec *s_mounts = NULL;
static int s_count = 0;
static int s_cap = 0;

/* Global RW lock protecting s_mounts/s_count/s_cap.
   Readers: Stat/Read/Open
   Writers: Mount/Unmount/Clear */
static SDL_RWLock *s_mountLock = NULL;

/* Lazy-creates the mount lock. Returns NULL on failure. */
static SDL_RWLock *_mount_lock(void)
{
    if (!s_mountLock)
    {
        s_mountLock = SDL_CreateRWLock();
    }
    return s_mountLock;
}

static void _free_mount(_MountRec *m)
{
    if (!m)
        return;
    if (m->type == LEO_MOUNT_PACK)
    {
        if (m->impl)
        {
            leo_pack_close((leo_pack *)m->impl);
            m->impl = NULL;
        }
    }
    else if (m->type == LEO_MOUNT_DIR)
    {
        if (m->impl)
        {
            SDL_free(m->impl); /* baseDir string */
            m->impl = NULL;
        }
    }
}

static void _reserve(int need)
{
    if (s_cap >= need)
        return;
    int ncap = s_cap ? s_cap * 2 : 8;
    if (ncap < need)
        ncap = need;
    _MountRec *n = (_MountRec *)SDL_realloc(s_mounts, (size_t)ncap * sizeof(_MountRec));
    if (!n)
        return; /* best-effort; callers should check after push */
    s_mounts = n;
    s_cap = ncap;
}

static void _sort_by_priority_desc(void)
{
    /* simple insertion sort; small N */
    for (int i = 1; i < s_count; ++i)
    {
        _MountRec key = s_mounts[i];
        int j = i - 1;
        while (j >= 0 && s_mounts[j].priority < key.priority)
        {
            s_mounts[j + 1] = s_mounts[j];
            --j;
        }
        s_mounts[j + 1] = key;
    }
}

/* -----------------------------
   Logical name validation
----------------------------- */
static int _is_bad_logical(const char *name)
{
    if (!name || !*name)
        return 1;
    if (name[0] == '/')
        return 1; /* absolute */
    /* Reject ".." segments anywhere */
    for (const char *p = name; *p; ++p)
    {
        if (p[0] == '.' && p[1] == '.')
            return 1;
        if (*p == '\\')
            return 1; /* backslashes not allowed */
    }
    return 0;
}

/* -----------------------------
   File helpers for DIR mounts
----------------------------- */
static int _join_path(char *out, size_t outcap, const char *base, const char *rel)
{
    if (!out || outcap == 0 || !base || !rel)
        return 0;
    size_t bl = SDL_strlen(base);
    size_t rl = SDL_strlen(rel);
    int need_slash = (bl > 0 && base[bl - 1] != '/');
    size_t need = bl + (need_slash ? 1 : 0) + rl + 1;
    if (need > outcap)
        return 0;
    SDL_memcpy(out, base, bl);
    size_t pos = bl;
    if (need_slash)
        out[pos++] = '/';
    SDL_memcpy(out + pos, rel, rl);
    pos += rl;
    out[pos] = '\0';
    return 1;
}

static int _file_size(FILE *f, size_t *out)
{
    if (!f)
        return 0;
    long cur = ftell(f);
    if (cur < 0)
        return 0;
    if (fseek(f, 0, SEEK_END) != 0)
        return 0;
    long end = ftell(f);
    if (end < 0)
    {
        (void)fseek(f, cur, SEEK_SET);
        return 0;
    }
    if (fseek(f, cur, SEEK_SET) != 0)
        return 0;
    if (out)
        *out = (size_t)end;
    return 1;
}

/* -----------------------------
   Public API
----------------------------- */
void leo_ClearMounts(void)
{
    SDL_RWLock *lk = _mount_lock();
    if (!lk)
    {
        /* Best-effort fallback: perform single-threaded clear if no lock. */
        for (int i = 0; i < s_count; ++i)
            _free_mount(&s_mounts[i]);
        SDL_free(s_mounts);
        s_mounts = NULL;
        s_count = 0;
        s_cap = 0;
        return;
    }

    SDL_LockRWLockForWriting(lk);
    for (int i = 0; i < s_count; ++i)
        _free_mount(&s_mounts[i]);
    SDL_free(s_mounts);
    s_mounts = NULL;
    s_count = 0;
    s_cap = 0;
    SDL_UnlockRWLock(lk);
}

bool leo_MountResourcePack(const char *packPath, const char *password, int priority)
{
    if (!packPath || !*packPath)
        return false;

    char fullPackPath[4096] = {0};
    bool isRelativePath = (packPath[0] != '/');

#ifdef __APPLE__
    if (isRelativePath)
    {
        // Get platform-appropriate base path for relative paths on macOS
        char *basePath = leo_GetResourceBasePath();
        if (!basePath)
        {
            return false;
        }

        int ret = snprintf(fullPackPath, sizeof(fullPackPath), "%s/%s", basePath, packPath);
        SDL_free(basePath);

        if (ret >= sizeof(fullPackPath))
        {
            return false;
        }
    }
    else
    {
        // Use absolute paths as-is
        if (strlen(packPath) >= sizeof(fullPackPath))
        {
            return false;
        }
        strcpy(fullPackPath, packPath);
    }
#else
    // Non-macOS: use packPath as-is
    if (strlen(packPath) >= sizeof(fullPackPath))
    {
        return false;
    }
    strcpy(fullPackPath, packPath);
#endif

    /* Try to open pack WITHOUT holding the write lock (can touch disk, may be slow). */
    leo_pack *p = NULL;
    if (leo_pack_open_file(&p, fullPackPath, password) == LEO_PACK_OK)
    {
        /* Quick policy check: if the pack has obfuscated entries, require a password. */
        if (!password || !password[0])
        {
            int n = leo_pack_count(p);
            for (int i = 0; i < n; ++i)
            {
                leo_pack_stat st;
                if (leo_pack_stat_index(p, i, &st) == LEO_PACK_OK)
                {
                    if (st.flags & LEO_PE_OBFUSCATED)
                    {
                        leo_pack_close(p);
                        return false;
                    }
                }
            }
        }

        SDL_RWLock *lk = _mount_lock();
        if (!lk)
        {
            leo_pack_close(p);
            return false;
        }

        /* Mutate mount list under exclusive lock. */
        SDL_LockRWLockForWriting(lk);

        _reserve(s_count + 1);
        if (s_count >= s_cap)
        {
            SDL_UnlockRWLock(lk);
            leo_pack_close(p);
            return false;
        }
        s_mounts[s_count].type = LEO_MOUNT_PACK;
        s_mounts[s_count].priority = priority;
        s_mounts[s_count].impl = (void *)p;
        ++s_count;
        _sort_by_priority_desc();

        SDL_UnlockRWLock(lk);
        return true;
    }

#ifdef __APPLE__
    // Only fallback to directory for relative paths on macOS
    if (isRelativePath)
    {
        char *basePath = leo_GetResourceBasePath();
        if (basePath)
        {
            char fallbackDirPath[4096] = {0};
            int ret = snprintf(fallbackDirPath, sizeof(fallbackDirPath), "%s/resources", basePath);
            SDL_free(basePath);

            if (ret < sizeof(fallbackDirPath))
            {
                return leo_MountDirectory(fallbackDirPath, priority);
            }
        }
    }
#endif

    return false;
}

bool leo_MountDirectory(const char *baseDir, int priority)
{
    if (!baseDir || !*baseDir)
        return false;

    char fullDirPath[4096] = {0};
    bool isRelativePath = (baseDir[0] != '/');

#ifdef __APPLE__
    if (isRelativePath)
    {
        // Get platform-appropriate base path for relative paths on macOS
        char *basePath = leo_GetResourceBasePath();
        if (!basePath)
        {
            return false;
        }

        int ret = snprintf(fullDirPath, sizeof(fullDirPath), "%s/%s", basePath, baseDir);
        SDL_free(basePath);

        if (ret >= sizeof(fullDirPath))
        {
            return false;
        }
    }
    else
    {
        // Use absolute paths as-is
        if (strlen(baseDir) >= sizeof(fullDirPath))
        {
            return false;
        }
        strcpy(fullDirPath, baseDir);
    }
#else
    // Non-macOS: use baseDir as-is
    if (strlen(baseDir) >= sizeof(fullDirPath))
    {
        return false;
    }
    strcpy(fullDirPath, baseDir);
#endif

    /* Prepare dup WITHOUT holding the write lock. */
    size_t n = SDL_strlen(fullDirPath);
    char *dup = (char *)SDL_malloc(n + 1);
    if (!dup)
        return false;
    SDL_memcpy(dup, fullDirPath, n + 1);

    SDL_RWLock *lk = _mount_lock();
    if (!lk)
    {
        SDL_free(dup);
        return false;
    }

    /* Mutate mount list under exclusive lock. */
    SDL_LockRWLockForWriting(lk);

    _reserve(s_count + 1);
    if (s_count >= s_cap)
    {
        SDL_UnlockRWLock(lk);
        SDL_free(dup);
        return false;
    }
    s_mounts[s_count].type = LEO_MOUNT_DIR;
    s_mounts[s_count].priority = priority;
    s_mounts[s_count].impl = dup;
    ++s_count;
    _sort_by_priority_desc();

    SDL_UnlockRWLock(lk);
    return true;
}

bool leo_StatAsset(const char *logicalName, leo_AssetInfo *out)
{
    if (_is_bad_logical(logicalName))
        return false;
    if (out)
    {
        out->size = 0;
        out->from_pack = 0;
    }

    SDL_RWLock *lk = _mount_lock();
    if (!lk)
        return false;

    SDL_LockRWLockForReading(lk);

    for (int i = 0; i < s_count; ++i)
    {
        _MountRec *m = &s_mounts[i];
        if (m->type == LEO_MOUNT_PACK)
        {
            int idx = -1;
            leo_pack_result r = leo_pack_find((leo_pack *)m->impl, logicalName, &idx);
            if (r == LEO_PACK_OK && idx >= 0)
            {
                if (out)
                {
                    leo_pack_stat st;
                    if (leo_pack_stat_index((leo_pack *)m->impl, idx, &st) == LEO_PACK_OK)
                    {
                        out->size = (size_t)st.size_uncompressed;
                        out->from_pack = 1;
                    }
                }
                SDL_UnlockRWLock(lk);
                return true;
            }
        }
        else if (m->type == LEO_MOUNT_DIR)
        {
            char full[4096] = {0};
            if (!_join_path(full, sizeof(full), (const char *)m->impl, logicalName))
                continue;
            FILE *f = fopen(full, "rb");
            if (!f)
                continue;
            if (out)
            {
                size_t sz = 0;
                if (_file_size(f, &sz))
                {
                    out->size = sz;
                    out->from_pack = 0;
                }
            }
            fclose(f);
            SDL_UnlockRWLock(lk);
            return true;
        }
    }

    SDL_UnlockRWLock(lk);
    return false;
}

size_t leo_ReadAsset(const char *logicalName, void *buffer, size_t bufferCap, size_t *out_total)
{
    if (_is_bad_logical(logicalName))
        return 0;

    SDL_RWLock *lk = _mount_lock();
    if (!lk)
        return 0;

    SDL_LockRWLockForReading(lk);

    for (int i = 0; i < s_count; ++i)
    {
        _MountRec *m = &s_mounts[i];
        if (m->type == LEO_MOUNT_PACK)
        {
            int idx = -1;
            if (leo_pack_find((leo_pack *)m->impl, logicalName, &idx) != LEO_PACK_OK || idx < 0)
                goto next_mount;

            if (!buffer)
            {
                if (out_total)
                {
                    leo_pack_stat st;
                    if (leo_pack_stat_index((leo_pack *)m->impl, idx, &st) == LEO_PACK_OK)
                        *out_total = (size_t)st.size_uncompressed;
                }
                SDL_UnlockRWLock(lk);
                return 0; /* probe only */
            }
            size_t outsz = 0;
            leo_pack_result rr = leo_pack_extract_index((leo_pack *)m->impl, idx, buffer, bufferCap, &outsz);
            SDL_UnlockRWLock(lk);
            if (rr == LEO_PACK_OK)
                return outsz;
            return 0;
        }
        else if (m->type == LEO_MOUNT_DIR)
        {
            char full[4096] = {0};
            if (!_join_path(full, sizeof(full), (const char *)m->impl, logicalName))
                goto next_mount;
            FILE *f = fopen(full, "rb");
            if (!f)
                goto next_mount;

            size_t sz = 0;
            (void)_file_size(f, &sz);
            if (!buffer)
            {
                if (out_total)
                    *out_total = sz;
                fclose(f);
                SDL_UnlockRWLock(lk);
                return 0; /* probe only */
            }
            if (bufferCap < sz)
            {
                fclose(f);
                SDL_UnlockRWLock(lk);
                return 0;
            }
            size_t n = fread(buffer, 1, sz, f);
            fclose(f);
            SDL_UnlockRWLock(lk);
            return n == sz ? n : 0;
        }
    next_mount:;
    }

    SDL_UnlockRWLock(lk);
    return 0;
}

void *leo_LoadAsset(const char *logicalName, size_t *out_size)
{
    if (out_size)
        *out_size = 0;
    size_t need = 0;
    if (leo_ReadAsset(logicalName, NULL, 0, &need) == 0 && need > 0)
    {
        void *mem = SDL_malloc(need);
        if (!mem)
            return NULL;
        size_t got = leo_ReadAsset(logicalName, mem, need, NULL);
        if (got != need)
        {
            SDL_free(mem);
            return NULL;
        }
        if (out_size)
            *out_size = need;
        return mem;
    }
    return NULL;
}

/* ---------------- Streaming API ---------------- */

struct leo_AssetStream
{
    int from_pack; /* 1 = pack, 0 = dir */
    size_t size;   /* total size */
    /* DIR case */
    FILE *f;
    /* PACK case */
    unsigned char *mem;
    size_t pos;
};

static size_t _filesize_of_path(const char *path)
{
    size_t sz = 0;
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    long cur = ftell(f);
    if (cur >= 0 && fseek(f, 0, SEEK_END) == 0)
    {
        long end = ftell(f);
        if (end >= 0)
            sz = (size_t)end;
        (void)fseek(f, cur, SEEK_SET);
    }
    fclose(f);
    return sz;
}

leo_AssetStream *leo_OpenAsset(const char *logicalName, leo_AssetInfo *info)
{
    if (info)
    {
        info->size = 0;
        info->from_pack = 0;
    }
    if (_is_bad_logical(logicalName))
        return NULL;

    SDL_RWLock *lk = _mount_lock();
    if (!lk)
        return NULL;

    SDL_LockRWLockForReading(lk);

    /* iterate mounts by priority */
    for (int i = 0; i < s_count; ++i)
    {
        _MountRec *m = &s_mounts[i];
        if (m->type == LEO_MOUNT_PACK)
        {
            int idx = -1;
            if (leo_pack_find((leo_pack *)m->impl, logicalName, &idx) != LEO_PACK_OK || idx < 0)
                continue;

            leo_pack_stat st;
            if (leo_pack_stat_index((leo_pack *)m->impl, idx, &st) != LEO_PACK_OK)
                continue;

            size_t need = (size_t)st.size_uncompressed; /* allow zero-sized */

            leo_AssetStream *s = (leo_AssetStream *)SDL_calloc(1, sizeof(*s));
            if (!s)
            {
                SDL_UnlockRWLock(lk);
                return NULL;
            }
            s->from_pack = 1;
            s->size = need;
            if (need > 0)
            {
                s->mem = (unsigned char *)SDL_malloc(need);
                if (!s->mem)
                {
                    SDL_free(s);
                    SDL_UnlockRWLock(lk);
                    return NULL;
                }
                size_t outsz = 0;
                if (leo_pack_extract_index((leo_pack *)m->impl, idx, s->mem, need, &outsz) != LEO_PACK_OK ||
                    outsz != need)
                {
                    SDL_free(s->mem);
                    SDL_free(s);
                    SDL_UnlockRWLock(lk);
                    return NULL;
                }
            }
            s->pos = 0;
            if (info)
            {
                info->size = s->size;
                info->from_pack = 1;
            }
            SDL_UnlockRWLock(lk);
            return s;
        }
        else if (m->type == LEO_MOUNT_DIR)
        {
            char full[4096] = {0};
            if (!_join_path(full, sizeof(full), (const char *)m->impl, logicalName))
                continue;
            FILE *f = fopen(full, "rb");
            if (!f)
                continue;

            leo_AssetStream *s = (leo_AssetStream *)SDL_calloc(1, sizeof(*s));
            if (!s)
            {
                fclose(f);
                SDL_UnlockRWLock(lk);
                return NULL;
            }
            s->from_pack = 0;
            s->f = f;
            s->size = _filesize_of_path(full);
            if (info)
            {
                info->size = s->size;
                info->from_pack = 0;
            }
            SDL_UnlockRWLock(lk);
            return s;
        }
    }

    SDL_UnlockRWLock(lk);
    return NULL;
}

size_t leo_AssetRead(leo_AssetStream *s, void *dst, size_t n)
{
    if (!s || !dst || n == 0)
        return 0;
    if (s->from_pack)
    {
        size_t rem = (s->pos < s->size) ? (s->size - s->pos) : 0;
        if (rem == 0)
            return 0;
        if (n > rem)
            n = rem;
        memcpy(dst, s->mem + s->pos, n);
        s->pos += n;
        return n;
    }
    else
    {
        return fread(dst, 1, n, s->f);
    }
}

bool leo_AssetSeek(leo_AssetStream *s, long long off, leo_SeekWhence whence)
{
    if (!s)
        return false;
    if (s->from_pack)
    {
        size_t base = 0;
        if (whence == LEO_SEEK_SET)
            base = 0;
        else if (whence == LEO_SEEK_CUR)
            base = s->pos;
        else if (whence == LEO_SEEK_END)
            base = s->size;
        else
            return false;

        long long t = (long long)base + off;
        if (t < 0)
            return false;
        size_t np = (size_t)t;
        if (np > s->size)
            return false;
        s->pos = np;
        return true;
    }
    else
    {
        int wh = (whence == LEO_SEEK_SET) ? SEEK_SET : (whence == LEO_SEEK_CUR) ? SEEK_CUR : SEEK_END;
        return fseek(s->f, (long)off, wh) == 0;
    }
}

long long leo_AssetTell(leo_AssetStream *s)
{
    if (!s)
        return -1;
    if (s->from_pack)
        return (long long)s->pos;
    long p = ftell(s->f);
    return (p < 0) ? -1 : (long long)p;
}

size_t leo_AssetSize(leo_AssetStream *s)
{
    if (!s)
        return 0;
    return s->size;
}

bool leo_AssetFromPack(leo_AssetStream *s)
{
    return s ? (s->from_pack != 0) : false;
}

void leo_CloseAsset(leo_AssetStream *s)
{
    if (!s)
        return;
    if (s->from_pack)
    {
        SDL_free(s->mem);
    }
    else if (s->f)
    {
        fclose(s->f);
    }
    SDL_free(s);
}

char *leo_LoadTextAsset(const char *logicalName, size_t *out_size_without_nul)
{
    size_t n = 0;
    void *p = leo_LoadAsset(logicalName, &n);
    if (!p)
    {
        if (out_size_without_nul)
            *out_size_without_nul = 0;
        return NULL;
    }
    char *s = (char *)SDL_malloc(n + 1);
    if (!s)
    {
        SDL_free(p);
        if (out_size_without_nul)
            *out_size_without_nul = 0;
        return NULL;
    }
    SDL_memcpy(s, p, n);
    s[n] = '\0';
    if (out_size_without_nul)
        *out_size_without_nul = n;
    SDL_free(p);
    return s;
}

/* ---------------- File Writing API --------------- */

char *leo_GetWriteDirectory(const char *org, const char *app)
{
    if (!org || !app)
        return NULL;

    return SDL_GetPrefPath(org, app);
}

static bool _create_directories_for_file(const char *filepath)
{
    if (!filepath)
        return false;

    // Find the last directory separator
    const char *last_sep = strrchr(filepath, '/');
    if (!last_sep)
        return true; // No directory to create

    // Create a copy of the directory path
    size_t dir_len = last_sep - filepath;
    char *dir_path = (char *)SDL_malloc(dir_len + 1);
    if (!dir_path)
        return false;

    SDL_strlcpy(dir_path, filepath, dir_len + 1);

    bool result = SDL_CreateDirectory(dir_path);
    SDL_free(dir_path);
    return result;
}

bool leo_WriteFile(const char *relativePath, const void *data, size_t size)
{
    if (!relativePath || !data)
        return false;

    // Get write directory
    char *write_dir = leo_GetWriteDirectory("Leo", "Engine");
    if (!write_dir)
        return false;

    // Build full path
    size_t full_path_len = SDL_strlen(write_dir) + SDL_strlen(relativePath) + 1;
    char *full_path = (char *)SDL_malloc(full_path_len);
    if (!full_path)
    {
        SDL_free(write_dir);
        return false;
    }

    SDL_strlcpy(full_path, write_dir, full_path_len);
    SDL_strlcat(full_path, relativePath, full_path_len);

    // Create directories if needed
    if (!_create_directories_for_file(full_path))
    {
        // Directory creation failed, but continue - file might still be writable
    }

    // Write file
    FILE *file = fopen(full_path, "wb");
    bool success = false;

    if (file)
    {
        if (size == 0 || fwrite(data, 1, size, file) == size)
        {
            success = true;
        }
        fclose(file);
    }

    SDL_free(full_path);
    SDL_free(write_dir);
    return success;
}

void *leo_ReadFile(const char *relativePath, size_t *out_size)
{
    if (out_size)
        *out_size = 0;

    if (!relativePath)
        return NULL;

    // Get write directory
    char *write_dir = leo_GetWriteDirectory("Leo", "Engine");
    if (!write_dir)
        return NULL;

    // Build full path
    size_t full_path_len = SDL_strlen(write_dir) + SDL_strlen(relativePath) + 1;
    char *full_path = (char *)SDL_malloc(full_path_len);
    if (!full_path)
    {
        SDL_free(write_dir);
        return NULL;
    }

    SDL_strlcpy(full_path, write_dir, full_path_len);
    SDL_strlcat(full_path, relativePath, full_path_len);

    // Open and read file
    FILE *file = fopen(full_path, "rb");
    void *data = NULL;

    if (file)
    {
        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (file_size >= 0)
        {
            data = SDL_malloc((size_t)file_size);
            if (data)
            {
                size_t bytes_read = fread(data, 1, (size_t)file_size, file);
                if (bytes_read == (size_t)file_size)
                {
                    if (out_size)
                        *out_size = (size_t)file_size;
                }
                else
                {
                    // Read failed
                    SDL_free(data);
                    data = NULL;
                }
            }
        }
        fclose(file);
    }

    SDL_free(full_path);
    SDL_free(write_dir);
    return data;
}

char *leo_ReadTextFile(const char *relativePath, size_t *out_size_without_nul)
{
    if (out_size_without_nul)
        *out_size_without_nul = 0;

    size_t size = 0;
    void *data = leo_ReadFile(relativePath, &size);
    if (!data)
        return NULL;

    // Add null terminator
    char *text = (char *)SDL_malloc(size + 1);
    if (!text)
    {
        SDL_free(data);
        return NULL;
    }

    SDL_memcpy(text, data, size);
    text[size] = '\0';

    if (out_size_without_nul)
        *out_size_without_nul = size;

    SDL_free(data);
    return text;
}
