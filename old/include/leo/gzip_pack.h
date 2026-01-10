/* ==========================================================
 * File: include/leo/pack_gzip.h
 * Gzip wrapper (RFC 1952) using raw sdefl/sinfl + CRC32
 * ========================================================== */
#pragma once
#include "leo/export.h"
#include "leo/pack_errors.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Returns a conservative upper bound for gzip-compressed output.
     * (raw deflate bound + 10-byte header + 8-byte trailer)
     */
    static inline size_t leo_gzip_bound(size_t in_sz)
    {
        return in_sz + (in_sz / 10) + 64 + 18;
    }

    /* Compress to a gzip-wrapped DEFLATE stream.
     * - in/out: out_sz is capacity on input, actual bytes written on success
     * - level: 0..8 (per sdefl); values are clamped to valid range
     * - returns LEO_PACK_OK or *_E_ARG/_E_NOSPACE/_E_COMPRESS
     */
    LEO_API leo_pack_result leo_compress_gzip(const void *in, size_t in_sz, void *out, size_t *out_sz, int level);

    /* Decompress a gzip-wrapped DEFLATE stream.
     * - Validates CRC32 and ISIZE; on mismatch returns LEO_PACK_E_DECOMPRESS.
     * - in/out: out_sz is capacity on input, actual bytes written on success
     * - returns LEO_PACK_OK or *_E_ARG/_E_NOSPACE/_E_DECOMPRESS
     */
    LEO_API leo_pack_result leo_decompress_gzip(const void *in, size_t in_sz, void *out, size_t *out_sz);

#ifdef __cplusplus
} /* extern "C" */
#endif
