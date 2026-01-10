/* Enable POSIX large-file APIs on glibc (Linux) */
#if !defined(_WIN32)
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE 1
#endif
#endif

#include "leo/pack_writer.h"
#include "leo/pack_compress.h"
#include "leo/pack_errors.h"
#include "leo/pack_format.h"
#include "leo/pack_obfuscate.h"
#include "leo/pack_util.h"

#include <SDL3/SDL_stdinc.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

/* ---- Local helpers ------------------------------------------------------- */

#ifndef LEO_PACK_LOCAL_ALLOC
#define LEO_PACK_LOCAL_ALLOC(sz) SDL_malloc(sz)
#define LEO_PACK_LOCAL_FREE(p) SDL_free(p)
#endif

#ifndef LEO_PACK_MIN
#define LEO_PACK_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

typedef struct
{
    char *name;             /* heap, null-terminated */
    leo_pack_entry_v1 meta; /* filled as we write data */
} writer_entry;

struct leo_pack_writer
{
    FILE *f;
    leo_pack_build_opts opt;
    uint64_t data_cursor; /* absolute offset where next payload will be written */
    uint64_t toc_offset;  /* where we’ll write the table */
    uint64_t toc_size;    /* computed at the end */
    uint64_t data_offset; /* absolute start of payload region (after header) */
    uint64_t pack_salt;   /* random salt used with password->seed */
    uint32_t xor_seed;    /* cached if password provided, else 0 */
    int any_obfuscation;  /* header pack_flags bit */
    /* entries */
    writer_entry *entries;
    int count;
    int capacity;
};

/* Generate a pseudo-random 64-bit salt (non-cryptographic, good enough) */
static uint64_t gen_salt64(void)
{
    uint64_t t = (uint64_t)time(NULL);
    uint64_t x = 0x9E3779B97F4A7C15ull ^ t;
    /* xorshift-ish */
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    x *= 0xBF58476D1CE4E5B9ull;
    x ^= x >> 31;
    return x;
}

static leo_pack_result ensure_capacity(leo_pack_writer *w, int extra)
{
    if (w->count + extra <= w->capacity)
        return LEO_PACK_OK;
    int nc = w->capacity ? w->capacity * 2 : 16;
    while (nc < w->count + extra)
        nc *= 2;
    writer_entry *ne = (writer_entry *)SDL_realloc(w->entries, (size_t)nc * sizeof(writer_entry));
    if (!ne)
        return LEO_PACK_E_OOM;
    w->entries = ne;
    w->capacity = nc;
    return LEO_PACK_OK;
}

static uint64_t file_tell64(FILE *f)
{
#if defined(_WIN32) && !defined(__MINGW32__)
    return _ftelli64(f);
#else
    long long p = ftello(f);
    if (p < 0)
        return (uint64_t)0;
    return (uint64_t)p;
#endif
}

static int file_seek64(FILE *f, uint64_t off)
{
#if defined(_WIN32) && !defined(__MINGW32__)
    return _fseeki64(f, (long long)off, SEEK_SET);
#else
    return fseeko(f, (off_t)off, SEEK_SET);
#endif
}

/* ---- API ----------------------------------------------------------------- */

leo_pack_result leo_pack_writer_begin(leo_pack_writer **out, const char *out_path, const leo_pack_build_opts *opts)
{
    if (!out || !out_path)
        return LEO_PACK_E_ARG;

    leo_pack_writer *w = (leo_pack_writer *)SDL_calloc(1, sizeof(*w));
    if (!w)
        return LEO_PACK_E_OOM;

    if (opts)
    {
        w->opt = *opts;
    }
    else
    {
        SDL_memset(&w->opt, 0, sizeof(w->opt));
    }
    if (w->opt.align == 0)
        w->opt.align = 1;

    w->f = fopen(out_path, "wb+");
    if (!w->f)
    {
        SDL_free(w);
        return LEO_PACK_E_IO;
    }

    /* Reserve header space (write a zeroed header now; fill it at end). */
    leo_pack_header_v1 hdr;
    SDL_memset(&hdr, 0, sizeof(hdr));
    SDL_memcpy(hdr.magic, LEO_PACK_MAGIC, 8);
    hdr.version = LEO_PACK_V1;
    hdr.pack_salt = gen_salt64();
    /* pack_flags, toc_offset/size, data_offset set later */
    /* header_crc32 computed at end */

    if (fwrite(&hdr, 1, sizeof(hdr), w->f) != sizeof(hdr))
    {
        fclose(w->f);
        SDL_free(w);
        return LEO_PACK_E_IO;
    }

    w->data_offset = (uint64_t)sizeof(hdr);
    w->data_cursor = w->data_offset;

    if (w->opt.password && w->opt.password[0])
    {
        w->xor_seed = leo_xor_seed_from_password(w->opt.password, hdr.pack_salt);
    }
    else
    {
        w->xor_seed = 0;
    }

    *out = w;
    return LEO_PACK_OK;
}

leo_pack_result leo_pack_writer_add(leo_pack_writer *w, const char *logical_name, const void *data, size_t size,
                                    int compress, int obfuscate)
{
    if (!w || !w->f || !logical_name)
        return LEO_PACK_E_ARG;
    if (size > 0 && !data)
        return LEO_PACK_E_ARG;
    if (obfuscate && (w->xor_seed == 0))
        return LEO_PACK_E_BAD_PASSWORD;

    /* Normalize logical name: convert '\\' -> '/', strip leading './' */
    char stack_norm[512];
    const char *name = logical_name;
    size_t name_len = SDL_strlen(logical_name);
    char *heap_norm = NULL;
    if (name_len + 1 <= sizeof(stack_norm))
    {
        for (size_t i = 0; i < name_len; ++i)
        {
            char c = logical_name[i];
            stack_norm[i] = (c == '\\') ? '/' : c;
        }
        stack_norm[name_len] = '\0';
        /* strip a single leading "./" */
        if (stack_norm[0] == '.' && stack_norm[1] == '/')
            name = stack_norm + 2;
        else
            name = stack_norm;
    }
    else
    {
        heap_norm = (char *)LEO_PACK_LOCAL_ALLOC(name_len + 1);
        if (!heap_norm)
            return LEO_PACK_E_OOM;
        for (size_t i = 0; i < name_len; ++i)
        {
            char c = logical_name[i];
            heap_norm[i] = (c == '\\') ? '/' : c;
        }
        heap_norm[name_len] = '\0';
        if (heap_norm[0] == '.' && heap_norm[1] == '/')
            name = heap_norm + 2;
        else
            name = heap_norm;
    }

    /* Reject duplicates (case-sensitive; keep it simple) */
    for (int i = 0; i < w->count; ++i)
    {
        if (SDL_strcmp(w->entries[i].name, name) == 0)
        {
            if (heap_norm)
                LEO_PACK_LOCAL_FREE(heap_norm);
            return LEO_PACK_E_STATE;
        }
    }

    /* ---- Zero-length entry: no payload write, just metadata ---- */
    if (size == 0)
    {
        leo_pack_result r = ensure_capacity(w, 1);
        if (r != LEO_PACK_OK)
        {
            if (heap_norm)
                LEO_PACK_LOCAL_FREE(heap_norm);
            return r;
        }

        writer_entry *e = &w->entries[w->count++];
        size_t norm_len = SDL_strlen(name);
        e->name = (char *)LEO_PACK_LOCAL_ALLOC(norm_len + 1);
        if (!e->name)
        {
            if (heap_norm)
                LEO_PACK_LOCAL_FREE(heap_norm);
            return LEO_PACK_E_OOM;
        }
        SDL_memcpy(e->name, name, norm_len + 1);

        SDL_memset(&e->meta, 0, sizeof(e->meta));
        e->meta.flags = 0;
        if (obfuscate)
        {
            e->meta.flags |= LEO_PE_OBFUSCATED;
            w->any_obfuscation = 1;
        }
        e->meta.name_len = (uint16_t)norm_len;
        e->meta.offset = w->data_cursor; /* no payload written */
        e->meta.size_uncompressed = 0;
        e->meta.size_stored = 0;
        e->meta.crc32_uncompressed = 0; /* OK for empty */

        if (heap_norm)
            LEO_PACK_LOCAL_FREE(heap_norm);
        return LEO_PACK_OK;
    }

    leo_pack_result r = ensure_capacity(w, 1);
    if (r != LEO_PACK_OK)
        return r;

    /* Build payload in memory: maybe compress, maybe obfuscate */
    const uint8_t *in = (const uint8_t *)data;
    size_t in_sz = size;

    /* Try compression if requested */
    uint8_t *comp_buf = NULL;
    size_t comp_sz = 0;
    int use_compression = 0;

    if (compress)
    {
        /* Use a conservative worst-case size; avoids relying on sdefl_bound
           from another translation unit (which can be brittle). */
        size_t worst = in_sz + (in_sz / 10) + 64;

        comp_buf = (uint8_t *)LEO_PACK_LOCAL_ALLOC(worst);
        if (!comp_buf)
            return LEO_PACK_E_OOM;

        comp_sz = worst;
        leo_deflate_opts dopts;
        dopts.level = w->opt.level;
        r = leo_compress_deflate(in, in_sz, comp_buf, &comp_sz, &dopts);
        if (r == LEO_PACK_OK && comp_sz < in_sz)
        {
            /* accept compressed */
            in = comp_buf;
            in_sz = comp_sz;
            use_compression = 1;
        }
        else
        {
            /* keep original (compression not effective) */
            comp_sz = 0; /* ignore */
        }
    }

    /* Make a mutable copy if we need to obfuscate (so we don’t touch user’s data)
     */
    uint8_t *mut = NULL;
    if (obfuscate)
    {
        mut = (uint8_t *)LEO_PACK_LOCAL_ALLOC(in_sz);
        if (!mut)
        {
            if (comp_buf)
                LEO_PACK_LOCAL_FREE(comp_buf);
            return LEO_PACK_E_OOM;
        }
        SDL_memcpy(mut, in, in_sz);
        leo_xor_stream_apply(w->xor_seed, mut, in_sz);
        in = mut; /* switch to obfuscated bytes */
        w->any_obfuscation = 1;
    }

    /* Align payload start if requested */
    uint64_t aligned = leo_align_up(w->data_cursor, (uint64_t)w->opt.align);
    if (aligned != w->data_cursor)
    {
        uint64_t pad = aligned - w->data_cursor;
        static const uint8_t zeros[16] = {0};
        while (pad)
        {
            size_t chunk = (size_t)LEO_PACK_MIN(pad, (uint64_t)sizeof(zeros));
            if (fwrite(zeros, 1, chunk, w->f) != chunk)
            {
                if (mut)
                    LEO_PACK_LOCAL_FREE(mut);
                if (comp_buf)
                    LEO_PACK_LOCAL_FREE(comp_buf);
                return LEO_PACK_E_IO;
            }
            pad -= chunk;
        }
        w->data_cursor = aligned;
    }

    /* Write the payload */
    uint64_t payload_ofs = w->data_cursor;
    if (fwrite(in, 1, in_sz, w->f) != in_sz)
    {
        if (mut)
            LEO_PACK_LOCAL_FREE(mut);
        if (comp_buf)
            LEO_PACK_LOCAL_FREE(comp_buf);
        return LEO_PACK_E_IO;
    }
    w->data_cursor += (uint64_t)in_sz;

    /* Record entry */
    writer_entry *e = &w->entries[w->count++];
    size_t norm_len = SDL_strlen(name);
    e->name = (char *)LEO_PACK_LOCAL_ALLOC(norm_len + 1);
    if (!e->name)
    {
        if (heap_norm)
            LEO_PACK_LOCAL_FREE(heap_norm);
        if (mut)
            LEO_PACK_LOCAL_FREE(mut);
        if (comp_buf)
            LEO_PACK_LOCAL_FREE(comp_buf);
        return LEO_PACK_E_OOM;
    }
    SDL_memcpy(e->name, name, norm_len + 1);

    SDL_memset(&e->meta, 0, sizeof(e->meta));
    e->meta.flags = 0;
    if (use_compression)
        e->meta.flags |= LEO_PE_COMPRESSED;
    if (obfuscate)
        e->meta.flags |= LEO_PE_OBFUSCATED;
    e->meta.name_len = (uint16_t)norm_len; /* for convenience */
    e->meta.offset = payload_ofs;
    e->meta.size_uncompressed = (uint64_t)size;
    e->meta.size_stored = (uint64_t)in_sz;
    e->meta.crc32_uncompressed = leo_crc32_ieee(data, size, 0);

    if (heap_norm)
        LEO_PACK_LOCAL_FREE(heap_norm);
    if (mut)
        LEO_PACK_LOCAL_FREE(mut);
    if (comp_buf)
        LEO_PACK_LOCAL_FREE(comp_buf);

    return LEO_PACK_OK;
}

static leo_pack_result write_toc_and_header(leo_pack_writer *w)
{
    /* Compute TOC offset (aligned if needed). Table is a sequence of:
       [uint16 name_len][name bytes][leo_pack_entry_v1] ... */
    uint64_t toc_off = w->data_cursor;
    /* No special alignment needed beyond payload alignment already done */

    if (file_seek64(w->f, toc_off) != 0)
        return LEO_PACK_E_IO;

    uint64_t toc_sz = 0;
    for (int i = 0; i < w->count; ++i)
    {
        const writer_entry *e = &w->entries[i];

        uint16_t nlen = (uint16_t)SDL_strlen(e->name);
        if (fwrite(&nlen, 1, sizeof(nlen), w->f) != sizeof(nlen))
            return LEO_PACK_E_IO;
        toc_sz += sizeof(nlen);

        if (nlen)
        {
            if (fwrite(e->name, 1, nlen, w->f) != nlen)
                return LEO_PACK_E_IO;
            toc_sz += nlen;
        }

        if (fwrite(&e->meta, 1, sizeof(e->meta), w->f) != sizeof(e->meta))
            return LEO_PACK_E_IO;
        toc_sz += sizeof(e->meta);
    }

    /* Finalize header and write it at the start */
    leo_pack_header_v1 hdr;
    SDL_memset(&hdr, 0, sizeof(hdr));
    SDL_memcpy(hdr.magic, LEO_PACK_MAGIC, 8);
    hdr.version = LEO_PACK_V1;
    hdr.pack_flags = (w->any_obfuscation ? LEO_PACK_FLAG_OBFUSCATED : 0);
    hdr.toc_offset = toc_off;
    hdr.toc_size = toc_sz;
    hdr.data_offset = w->data_offset;
    hdr.pack_salt = w->pack_salt ? w->pack_salt : gen_salt64(); /* fallback */
    /* Reserved zeros */

    /* header_crc32 is computed over bytes [0..0x4F] (i.e., header minus the crc
     * field) */
    uint8_t tmp[sizeof(hdr)];
    SDL_memcpy(tmp, &hdr, sizeof(hdr));
    /* zero the crc field before computing */
    SDL_memset(tmp + offsetof(leo_pack_header_v1, header_crc32), 0, sizeof(uint32_t));
    hdr.header_crc32 = leo_crc32_ieee(tmp, sizeof(hdr), 0);

    if (file_seek64(w->f, 0) != 0)
        return LEO_PACK_E_IO;
    if (fwrite(&hdr, 1, sizeof(hdr), w->f) != sizeof(hdr))
        return LEO_PACK_E_IO;

    return LEO_PACK_OK;
}

leo_pack_result leo_pack_writer_end(leo_pack_writer *w)
{
    if (!w)
        return LEO_PACK_E_ARG;
    if (!w->f)
    {
        SDL_free(w);
        return LEO_PACK_E_STATE;
    }

    /* Remember salt chosen in begin (used for seed); fetch it back by reading
     * header */
    if (file_seek64(w->f, 0) != 0)
    {
        fclose(w->f);
        SDL_free(w);
        return LEO_PACK_E_IO;
    }
    leo_pack_header_v1 begin_hdr;
    if (fread(&begin_hdr, 1, sizeof(begin_hdr), w->f) != sizeof(begin_hdr))
    {
        fclose(w->f);
        SDL_free(w);
        return LEO_PACK_E_IO;
    }
    w->pack_salt = begin_hdr.pack_salt ? begin_hdr.pack_salt : gen_salt64();

    /* Write TOC and final header */
    leo_pack_result r = write_toc_and_header(w);

    /* cleanup */
    for (int i = 0; i < w->count; ++i)
    {
        LEO_PACK_LOCAL_FREE(w->entries[i].name);
    }
    SDL_free(w->entries);

    int err = ferror(w->f);
    fclose(w->f);

    SDL_free(w);
    if (r != LEO_PACK_OK)
        return r;
    if (err)
        return LEO_PACK_E_IO;
    return LEO_PACK_OK;
}
