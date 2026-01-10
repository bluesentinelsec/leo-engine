#include "leo/pack_compress.h"
#include "leo/pack_errors.h"

#include <SDL3/SDL_stdinc.h>
#include <limits.h>

/* Pull in the single-header implementations exactly once here. */
#define SDEFL_IMPLEMENTATION
#define SINFL_IMPLEMENTATION
#include <mmx/sdefl.h> /* sdefl_bound, sdeflate, zsdeflate; levels 0..8 */
#include <mmx/sinfl.h> /* sinflate, zsinflate                      */

/* Internal: clamp to sdefl’s valid level range (0..8). */
static int clamp_sdefl_level(int lvl)
{
    if (lvl < SDEFL_LVL_MIN)
        return SDEFL_LVL_MIN;
    if (lvl > SDEFL_LVL_MAX)
        return SDEFL_LVL_MAX;
    return lvl;
}

/* ----------------------------
   leo_compress_deflate()
   ---------------------------- */
leo_pack_result leo_compress_deflate(const void *in, size_t in_sz, void *out, size_t *out_sz,
                                     const leo_deflate_opts *opt)
{
    if (!in || !out || !out_sz)
        return LEO_PACK_E_ARG;

    /* sdefl/sinfl use int lengths; bail if size doesn't fit. */
    if (in_sz > (size_t)INT_MAX)
        return LEO_PACK_E_ARG;
    if (*out_sz > (size_t)INT_MAX)
        *out_sz = (size_t)INT_MAX;

    const int lvl = clamp_sdefl_level(opt ? opt->level : SDEFL_LVL_DEF);

    /* Conservative worst-case for zlib-wrapped output.
   Matches the writer’s allocation strategy and preserves NOSPACE semantics. */
    size_t worst = in_sz + (in_sz / 10) + 64; /* generous upper bound */
    if (*out_sz < worst)
        return LEO_PACK_E_NOSPACE;

    struct sdefl ctx;
    SDL_memset(&ctx, 0, sizeof(ctx));
    int wrote = zsdeflate(&ctx, out, in, (int)in_sz, lvl);
    if (wrote <= 0)
        return LEO_PACK_E_COMPRESS;

    *out_sz = (size_t)wrote;
    return LEO_PACK_OK;
}

/* ----------------------------
   leo_decompress_deflate()
   ---------------------------- */
leo_pack_result leo_decompress_deflate(const void *in, size_t in_sz, void *out, size_t *out_sz)
{
    if (!in || !out || !out_sz)
        return LEO_PACK_E_ARG;

    if (in_sz > (size_t)INT_MAX)
        return LEO_PACK_E_ARG;
    if (*out_sz > (size_t)INT_MAX)
        *out_sz = (size_t)INT_MAX;

    int produced = zsinflate(out, (int)*out_sz, in, (int)in_sz); /* zlib stream */

    if (produced < 0)
        return LEO_PACK_E_DECOMPRESS; /* stream error */
    if ((size_t)produced > *out_sz)
        return LEO_PACK_E_NOSPACE; /* should not happen */

    *out_sz = (size_t)produced;
    return LEO_PACK_OK;
}
