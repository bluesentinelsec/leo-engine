#pragma once
#include "leo/pack_errors.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int level;
} leo_deflate_opts;

/* Returns LEO_PACK_OK or *_E_COMPRESS/_E_DECOMPRESS. out_sz is in/out capacity/size. */
leo_pack_result leo_compress_deflate(const void* in, size_t in_sz, void* out, size_t* out_sz,
	const leo_deflate_opts* opt);

leo_pack_result leo_decompress_deflate(const void* in, size_t in_sz, void* out, size_t* out_sz);

#ifdef __cplusplus
} /* extern "C" */
#endif
