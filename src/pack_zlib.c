/* ==========================================================
 * File: src/pack_zlib.c
 * ========================================================== */
#include "leo/pack_zlib.h"
#include "leo/pack_errors.h"

#include <limits.h>
#include <string.h>

/* Use mmx sdefl/sinfl "z" helpers (zlib-wrapped).
 * NOTE: Do NOT define SDEFL_IMPLEMENTATION/SINFL_IMPLEMENTATION here.
 * They are already defined/compiled in src/pack_compress.c.
 */
#include <mmx/sdefl.h> /* zsdeflate, SDEFL_LVL_MIN/MAX/DEF */
#include <mmx/sinfl.h> /* zsinflate */

static int clamp_level(int lvl)
{
    if (lvl < SDEFL_LVL_MIN)
        return SDEFL_LVL_MIN;
    if (lvl > SDEFL_LVL_MAX)
        return SDEFL_LVL_MAX;
    return lvl;
}

leo_pack_result leo_compress_zlib(const void *in, size_t in_sz, void *out, size_t *out_sz, int level)
{
    if (!in || !out || !out_sz)
        return LEO_PACK_E_ARG;

    /* sdefl uses int lengths; guard the range. */
    if (in_sz > (size_t)INT_MAX)
        return LEO_PACK_E_ARG;

    /* Preserve NOSPACE semantics with a conservative bound. */
    const size_t need = leo_zlib_bound(in_sz);
    if (*out_sz < need)
        return LEO_PACK_E_NOSPACE;

    struct sdefl ctx;
    memset(&ctx, 0, sizeof(ctx));

    const int lvl = clamp_level(level);
    /* zsdeflate writes a zlib stream into 'out' and returns bytes written. */
    int wrote = zsdeflate(&ctx, out, in, (int)in_sz, lvl);
    if (wrote <= 0)
        return LEO_PACK_E_COMPRESS;

    *out_sz = (size_t)wrote;
    return LEO_PACK_OK;
}

leo_pack_result leo_decompress_zlib(const void *in, size_t in_sz, void *out, size_t *out_sz)
{
    if (!in || !out || !out_sz)
        return LEO_PACK_E_ARG;

    if (in_sz > (size_t)INT_MAX)
        return LEO_PACK_E_ARG;
    if (*out_sz > (size_t)INT_MAX)
        *out_sz = (size_t)INT_MAX;

    /* zsinflate reads a zlib stream from 'in' into 'out', returns produced bytes or <0 on error. */
    int produced = zsinflate(out, (int)*out_sz, in, (int)in_sz);
    if (produced < 0)
        return LEO_PACK_E_DECOMPRESS;
    if ((size_t)produced > *out_sz)
        return LEO_PACK_E_NOSPACE; /* should not happen */

    *out_sz = (size_t)produced;
    return LEO_PACK_OK;
}
