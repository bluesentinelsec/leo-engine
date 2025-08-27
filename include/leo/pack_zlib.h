/* ==========================================================
 * File: include/leo/pack_zlib.h
 * Zlib wrapper (RFC 1950) using sdefl/sinfl "z" helpers
 * ========================================================== */
#pragma once
#include "pack_errors.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Returns a conservative upper bound for zlib-compressed output. */
    static inline size_t leo_zlib_bound(size_t in_sz)
    {
        /* generous upper bound used elsewhere in your codebase */
        return in_sz + (in_sz / 10) + 64;
    }

    /* Compress to a zlib-wrapped deflate stream.
     * - in/out: out_sz is capacity on input, actual bytes written on success
     * - level: 0..8 (per sdefl); values are clamped to valid range
     * - returns LEO_PACK_OK or *_E_ARG/_E_NOSPACE/_E_COMPRESS
     */
    leo_pack_result leo_compress_zlib(const void *in, size_t in_sz, void *out, size_t *out_sz, int level);

    /* Decompress a zlib-wrapped deflate stream.
     * - in/out: out_sz is capacity on input, actual bytes written on success
     * - returns LEO_PACK_OK or *_E_ARG/_E_NOSPACE/_E_DECOMPRESS
     */
    leo_pack_result leo_decompress_zlib(const void *in, size_t in_sz, void *out, size_t *out_sz);

#ifdef __cplusplus
} /* extern "C" */
#endif
