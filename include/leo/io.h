// include/leo/io.h
#pragma once
#include "leo/export.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        LEO_MOUNT_PACK = 1,
        LEO_MOUNT_DIR = 2
    } leo_MountType;

    typedef struct
    {
        leo_MountType type;
        int priority; // higher checked first
        void *_impl;  // leo_pack* if PACK, char* basePath if DIR
    } leo_Mount;

    // Lifecycle
    LEO_API void leo_ClearMounts(void);

    LEO_API bool leo_MountResourcePack(const char *packPath, const char *password, int priority);

    LEO_API bool leo_MountDirectory(const char *baseDir, int priority);

    // Query/IO (logical, forward-slash paths like "images/player.png")
    typedef struct
    {
        size_t size;   // uncompressed size if from pack
        int from_pack; // 1 if read from pack, else 0 (directory)
    } leo_AssetInfo;

    // Returns true if an asset exists anywhere in mounts; fills info if not NULL.
    LEO_API bool leo_StatAsset(const char *logicalName, leo_AssetInfo *out);

    // Read whole file into caller-supplied buffer; returns bytes written, or 0 on error.
    // If out_total is provided and buffer==NULL, returns the size required via out_total.
    LEO_API size_t leo_ReadAsset(const char *logicalName, void *buffer, size_t bufferCap, size_t *out_total);

    // Convenience "read-all + malloc". Caller frees.
    // On success returns non-NULL and fills out_size. On error returns NULL.
    LEO_API void *leo_LoadAsset(const char *logicalName, size_t *out_size);

    /* ---------------- Streaming API (optional but enables music/large files) --------------- */
    typedef struct leo_AssetStream leo_AssetStream; /* opaque */

    /* Open an asset for streaming. Returns NULL on error. Fills info if not NULL. */
    LEO_API leo_AssetStream *leo_OpenAsset(const char *logicalName, leo_AssetInfo *info);

    /* Read up to 'n' bytes into dst. Returns bytes read (0 = EOF or error). */
    LEO_API size_t leo_AssetRead(leo_AssetStream *s, void *dst, size_t n);

    /* Seek relative to start/current/end. Returns true on success. */
    typedef enum
    {
        LEO_SEEK_SET = 0,
        LEO_SEEK_CUR = 1,
        LEO_SEEK_END = 2
    } leo_SeekWhence;

    LEO_API bool leo_AssetSeek(leo_AssetStream *s, long long offset, leo_SeekWhence whence);

    /* Tell returns current offset (>=0) or -1 on error. */
    LEO_API long long leo_AssetTell(leo_AssetStream *s);

    /* Size returns asset size in bytes (0 on error). */
    LEO_API size_t leo_AssetSize(leo_AssetStream *s);

    /* True if the underlying source is a pack, false if directory. */
    LEO_API bool leo_AssetFromPack(leo_AssetStream *s);

    /* Close and free resources. Safe on NULL. */
    LEO_API void leo_CloseAsset(leo_AssetStream *s);

    /* Convenience: load text (adds a trailing '\0'). Caller frees. */
    LEO_API char *leo_LoadTextAsset(const char *logicalName, size_t *out_size_without_nul);

    /* ---------------- File Writing API --------------- */

    /* Write data to file in user data directory. Creates directories as needed. */
    LEO_API bool leo_WriteFile(const char *relativePath, const void *data, size_t size);

    /* Read data from file in user data directory. Caller frees returned data. */
    LEO_API void *leo_ReadFile(const char *relativePath, size_t *out_size);

    /* Read text file from user data directory (adds trailing '\0'). Caller frees. */
    LEO_API char *leo_ReadTextFile(const char *relativePath, size_t *out_size_without_nul);

    /* Get writable directory path for organization/application. Caller frees. */
    LEO_API char *leo_GetWriteDirectory(const char *org, const char *app);

#ifdef __cplusplus
}
#endif
