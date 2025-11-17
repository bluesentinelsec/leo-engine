#if !defined(_WIN32)
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE 1
#endif
#endif

#include "leo/pack_reader.h"
#include "leo/pack_compress.h"
#include "leo/pack_errors.h"
#include "leo/pack_format.h"
#include "leo/pack_obfuscate.h"
#include "leo/pack_util.h"

#include <SDL3/SDL_stdinc.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

/* ---- Local types/state --------------------------------------------------- */

typedef struct
{
    char *name; /* heap, null-terminated */
    leo_pack_entry_v1 meta;
} pack_entry_rec;

struct leo_pack
{
    FILE *f;
    leo_pack_header_v1 hdr;
    uint32_t xor_seed; /* 0 if no/empty password */
    int count;
    pack_entry_rec *entries; /* array[count] */
};

#ifndef LEO_PACK_LOCAL_ALLOC
#define LEO_PACK_LOCAL_ALLOC(sz) SDL_malloc(sz)
#define LEO_PACK_LOCAL_FREE(p) SDL_free(p)
#endif

static int file_seek64(FILE *f, uint64_t off)
{
#if defined(_WIN32) && !defined(__MINGW32__)
    return _fseeki64(f, (long long)off, SEEK_SET);
#else
    return fseeko(f, (off_t)off, SEEK_SET);
#endif
}

/* ---- Header validation --------------------------------------------------- */

static leo_pack_result read_and_validate_header(FILE *f, leo_pack_header_v1 *out)
{
    if (file_seek64(f, 0) != 0)
        return LEO_PACK_E_IO;

    leo_pack_header_v1 hdr;
    if (fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr))
        return LEO_PACK_E_IO;

    if (memcmp(hdr.magic, LEO_PACK_MAGIC, 8) != 0)
        return LEO_PACK_E_FORMAT;
    if (hdr.version != LEO_PACK_V1)
        return LEO_PACK_E_FORMAT;

    /* Verify header CRC (over the entire struct with CRC field zeroed) */
    uint8_t tmp[sizeof(hdr)];
    SDL_memcpy(tmp, &hdr, sizeof(hdr));
    uint32_t expect = hdr.header_crc32;
    SDL_memset(tmp + offsetof(leo_pack_header_v1, header_crc32), 0, sizeof(uint32_t));
    uint32_t calc = leo_crc32_ieee(tmp, sizeof(hdr), 0);
    if (calc != expect)
        return LEO_PACK_E_FORMAT;

    *out = hdr;
    return LEO_PACK_OK;
}

/* ---- TOC parsing --------------------------------------------------------- */

static leo_pack_result load_toc(FILE *f, const leo_pack_header_v1 *hdr, pack_entry_rec **out_entries, int *out_count)
{
    if (hdr->toc_size == 0)
    {
        *out_entries = NULL;
        *out_count = 0;
        return LEO_PACK_OK;
    }
    if (file_seek64(f, hdr->toc_offset) != 0)
        return LEO_PACK_E_IO;

    /* We don’t know the count; parse until toc_size consumed. */
    uint64_t left = hdr->toc_size;
    int cap = 16;
    int cnt = 0;
    pack_entry_rec *arr = (pack_entry_rec *)LEO_PACK_LOCAL_ALLOC((size_t)cap * sizeof(pack_entry_rec));
    if (!arr)
        return LEO_PACK_E_OOM;

    while (left > 0)
    {
        /* read name_len (u16) */
        uint16_t nlen = 0;
        if (left < sizeof(nlen))
        {
            /* malformed */
            for (int i = 0; i < cnt; ++i)
                LEO_PACK_LOCAL_FREE(arr[i].name);
            LEO_PACK_LOCAL_FREE(arr);
            return LEO_PACK_E_FORMAT;
        }
        if (fread(&nlen, 1, sizeof(nlen), f) != sizeof(nlen))
        {
            for (int i = 0; i < cnt; ++i)
                LEO_PACK_LOCAL_FREE(arr[i].name);
            LEO_PACK_LOCAL_FREE(arr);
            return LEO_PACK_E_IO;
        }
        left -= sizeof(nlen);

        char *name = NULL;
        if (nlen)
        {
            if (left < nlen)
            {
                for (int i = 0; i < cnt; ++i)
                    LEO_PACK_LOCAL_FREE(arr[i].name);
                LEO_PACK_LOCAL_FREE(arr);
                return LEO_PACK_E_FORMAT;
            }
            name = (char *)LEO_PACK_LOCAL_ALLOC((size_t)nlen + 1);
            if (!name)
            {
                for (int i = 0; i < cnt; ++i)
                    LEO_PACK_LOCAL_FREE(arr[i].name);
                LEO_PACK_LOCAL_FREE(arr);
                return LEO_PACK_E_OOM;
            }
            if (fread(name, 1, nlen, f) != nlen)
            {
                LEO_PACK_LOCAL_FREE(name);
                for (int i = 0; i < cnt; ++i)
                    LEO_PACK_LOCAL_FREE(arr[i].name);
                LEO_PACK_LOCAL_FREE(arr);
                return LEO_PACK_E_IO;
            }
            name[nlen] = '\0';
            left -= nlen;
        }
        else
        {
            /* allow empty names, still allocate a 1-byte empty string for consistency */
            name = (char *)LEO_PACK_LOCAL_ALLOC(1);
            if (!name)
            {
                for (int i = 0; i < cnt; ++i)
                    LEO_PACK_LOCAL_FREE(arr[i].name);
                LEO_PACK_LOCAL_FREE(arr);
                return LEO_PACK_E_OOM;
            }
            name[0] = '\0';
        }

        leo_pack_entry_v1 meta;
        if (left < sizeof(meta))
        {
            LEO_PACK_LOCAL_FREE(name);
            for (int i = 0; i < cnt; ++i)
                LEO_PACK_LOCAL_FREE(arr[i].name);
            LEO_PACK_LOCAL_FREE(arr);
            return LEO_PACK_E_FORMAT;
        }
        if (fread(&meta, 1, sizeof(meta), f) != sizeof(meta))
        {
            LEO_PACK_LOCAL_FREE(name);
            for (int i = 0; i < cnt; ++i)
                LEO_PACK_LOCAL_FREE(arr[i].name);
            LEO_PACK_LOCAL_FREE(arr);
            return LEO_PACK_E_IO;
        }
        left -= sizeof(meta);

        if (cnt == cap)
        {
            int ncap = cap * 2;
            pack_entry_rec *narr = (pack_entry_rec *)SDL_realloc(arr, (size_t)ncap * sizeof(pack_entry_rec));
            if (!narr)
            {
                LEO_PACK_LOCAL_FREE(name);
                for (int i = 0; i < cnt; ++i)
                    LEO_PACK_LOCAL_FREE(arr[i].name);
                LEO_PACK_LOCAL_FREE(arr);
                return LEO_PACK_E_OOM;
            }
            arr = narr;
            cap = ncap;
        }

        arr[cnt].name = name;
        arr[cnt].meta = meta;
        ++cnt;
    }

    *out_entries = arr;
    *out_count = cnt;
    return LEO_PACK_OK;
}

static int file_size64(FILE *f, uint64_t *out_sz)
{
#if defined(_WIN32) && !defined(__MINGW32__)
    __int64 cur = _ftelli64(f);
    if (cur < 0)
        return -1;
    if (_fseeki64(f, 0, SEEK_END) != 0)
        return -1;
    __int64 end = _ftelli64(f);
    if (end < 0)
    {
        _fseeki64(f, cur, SEEK_SET);
        return -1;
    }
    if (_fseeki64(f, cur, SEEK_SET) != 0)
        return -1;
    *out_sz = (uint64_t)end;
    return 0;
#else
    off_t cur = ftello(f);
    if (cur < 0)
        return -1;
    if (fseeko(f, 0, SEEK_END) != 0)
        return -1;
    off_t end = ftello(f);
    if (end < 0)
    {
        fseeko(f, cur, SEEK_SET);
        return -1;
    }
    if (fseeko(f, cur, SEEK_SET) != 0)
        return -1;
    *out_sz = (uint64_t)end;
    return 0;
#endif
}

static int zlib_header_seems_valid(const uint8_t *buf, size_t n)
{
    if (n < 2)
        return 0;
    uint8_t cmf = buf[0], flg = buf[1];

    // CM = 8 (deflate)
    if ((cmf & 0x0F) != 8)
        return 0;
    // CINFO (window size) <= 7 (32K)
    if ((cmf >> 4) > 7)
        return 0;
    // FCHECK makes (CMF*256 + FLG) % 31 == 0
    if ((((uint16_t)cmf << 8) | flg) % 31 != 0)
        return 0;
    // We never use a preset dictionary
    if (flg & 0x20)
        return 0;

    return 1;
}

/* ---- Public API ---------------------------------------------------------- */

leo_pack_result leo_pack_open_file(leo_pack **out, const char *path, const char *password)
{
    if (!out || !path)
        return LEO_PACK_E_ARG;

    FILE *f = fopen(path, "rb");
    if (!f)
        return LEO_PACK_E_IO;

    leo_pack_header_v1 hdr;
    leo_pack_result r = read_and_validate_header(f, &hdr);
    if (r != LEO_PACK_OK)
    {
        fclose(f);
        return r;
    }

    /* Prepare pack object */
    leo_pack *p = (leo_pack *)SDL_calloc(1, sizeof(*p));
    if (!p)
    {
        fclose(f);
        return LEO_PACK_E_OOM;
    }

    p->f = f;
    p->hdr = hdr;

    /* If any obfuscation is present, require password for extraction of those entries */
    if ((hdr.pack_flags & LEO_PACK_FLAG_OBFUSCATED) != 0)
    {
        if (!password || !password[0])
        {
            /* allow open but later extracts of obfuscated entries will fail with BAD_PASSWORD */
            p->xor_seed = 0;
        }
        else
        {
            p->xor_seed = leo_xor_seed_from_password(password, hdr.pack_salt);
        }
    }

    r = load_toc(f, &hdr, &p->entries, &p->count);
    if (r != LEO_PACK_OK)
    {
        leo_pack_close(p);
        return r;
    }

    *out = p;
    return LEO_PACK_OK;
}

void leo_pack_close(leo_pack *p)
{
    if (!p)
        return;
    if (p->entries)
    {
        for (int i = 0; i < p->count; ++i)
        {
            LEO_PACK_LOCAL_FREE(p->entries[i].name);
        }
        LEO_PACK_LOCAL_FREE(p->entries);
    }
    if (p->f)
        fclose(p->f);
    SDL_free(p);
}

int leo_pack_count(leo_pack *p)
{
    return p ? p->count : 0;
}

leo_pack_result leo_pack_stat_index(leo_pack *p, int index, leo_pack_stat *out)
{
    if (!p || !out)
        return LEO_PACK_E_ARG;
    if (index < 0 || index >= p->count)
        return LEO_PACK_E_NOTFOUND;
    pack_entry_rec *e = &p->entries[index];
    out->name = e->name;
    out->flags = e->meta.flags;
    out->size_uncompressed = e->meta.size_uncompressed;
    out->size_stored = e->meta.size_stored;
    return LEO_PACK_OK;
}

leo_pack_result leo_pack_find(leo_pack *p, const char *name, int *out_index)
{
    if (!p || !name || !out_index)
        return LEO_PACK_E_ARG;
    for (int i = 0; i < p->count; ++i)
    {
        if (SDL_strcmp(p->entries[i].name, name) == 0)
        {
            *out_index = i;
            return LEO_PACK_OK;
        }
    }
    return LEO_PACK_E_NOTFOUND;
}

leo_pack_result leo_pack_extract_index(leo_pack *p, int index, void *dst, size_t dst_cap, size_t *out_size)
{
    if (!p || !dst)
        return LEO_PACK_E_ARG;
    if (index < 0 || index >= p->count)
        return LEO_PACK_E_NOTFOUND;

    const pack_entry_rec *e = &p->entries[index];

    uint64_t fsz = 0;
    if (file_size64(p->f, &fsz) != 0)
        return LEO_PACK_E_IO;

    uint64_t off = e->meta.offset;
    uint64_t sz = e->meta.size_stored;

    if (off > fsz || sz > (fsz - off))
    {
        // If obfuscated, map corruption to BAD_PASSWORD to keep threat model simple for callers.
        return (e->meta.flags & LEO_PE_OBFUSCATED) ? LEO_PACK_E_BAD_PASSWORD : LEO_PACK_E_FORMAT;
    }

    /* Read stored blob (with safety padding to tolerate small overreads in inflaters) */
    if (file_seek64(p->f, e->meta.offset) != 0)
        return LEO_PACK_E_IO;
    size_t tmp_sz = (size_t)e->meta.size_stored;
    size_t tmp_cap = tmp_sz + 64; /* input guard */
    uint8_t *tmp = (uint8_t *)LEO_PACK_LOCAL_ALLOC(tmp_cap);
    if (!tmp)
        return LEO_PACK_E_OOM;

    if (fread(tmp, 1, tmp_sz, p->f) != tmp_sz)
    {
        LEO_PACK_LOCAL_FREE(tmp);
        return LEO_PACK_E_IO;
    }
    SDL_memset(tmp + tmp_sz, 0, tmp_cap - tmp_sz);

    /* De-obfuscate if needed */
    if ((e->meta.flags & LEO_PE_OBFUSCATED) != 0)
    {
        if (p->xor_seed == 0)
        {
            LEO_PACK_LOCAL_FREE(tmp);
            return LEO_PACK_E_BAD_PASSWORD;
        }
        leo_xor_stream_apply(p->xor_seed, tmp, tmp_sz);
    }

    /* Decompress (if compressed), or just copy */
    leo_pack_result r = LEO_PACK_OK;
    size_t produced = (size_t)e->meta.size_uncompressed;

    if ((e->meta.flags & LEO_PE_COMPRESSED) != 0)
    {
        if (dst_cap < produced)
        {
            LEO_PACK_LOCAL_FREE(tmp);
            return LEO_PACK_E_NOSPACE;
        }
        size_t out_sz = produced;
        /* If obfuscated (or just to be defensive), reject obviously bad zlib headers
   before calling the inflater. This avoids crashing in third-party code. */
        if (!zlib_header_seems_valid(tmp, tmp_sz))
        {
            LEO_PACK_LOCAL_FREE(tmp);
            if ((e->meta.flags & LEO_PE_OBFUSCATED) != 0)
                return LEO_PACK_E_BAD_PASSWORD;
            else
                return LEO_PACK_E_DECOMPRESS;
        }
        r = leo_decompress_deflate(tmp, tmp_sz, dst, &out_sz);
        if (r != LEO_PACK_OK)
        {
            LEO_PACK_LOCAL_FREE(tmp);
            if ((e->meta.flags & LEO_PE_OBFUSCATED) != 0)
                return LEO_PACK_E_BAD_PASSWORD;
            return r;
        }
        produced = out_sz;
    }
    else
    {
        if (dst_cap < tmp_sz)
        {
            LEO_PACK_LOCAL_FREE(tmp);
            return LEO_PACK_E_NOSPACE;
        }
        SDL_memcpy(dst, tmp, tmp_sz);
        produced = tmp_sz;
    }

    /* Optional integrity check (uncompressed CRC) */
    uint32_t crc = leo_crc32_ieee(dst, produced, 0);
    if (crc != e->meta.crc32_uncompressed)
    {
        LEO_PACK_LOCAL_FREE(tmp);
        if ((e->meta.flags & LEO_PE_OBFUSCATED) != 0)
            return LEO_PACK_E_BAD_PASSWORD;
        return LEO_PACK_E_FORMAT; /* corrupted or wrong password */
    }

    if (out_size)
        *out_size = produced;
    LEO_PACK_LOCAL_FREE(tmp);
    return LEO_PACK_OK;
}

leo_pack_result leo_pack_extract(leo_pack *p, const char *name, void *dst, size_t dst_cap, size_t *out_size)
{
    if (!p || !name || !dst)
        return LEO_PACK_E_ARG;
    int idx = -1;
    leo_pack_result r = leo_pack_find(p, name, &idx);
    if (r != LEO_PACK_OK)
        return r;
    return leo_pack_extract_index(p, idx, dst, dst_cap, out_size);
}
