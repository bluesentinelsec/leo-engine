#pragma once
#include "leo/pack_errors.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct
{
	const char* password; /* optional, required if you set obfuscate for any file */
	int level; /* compression level (0..9) */
	size_t align; /* payload alignment (e.g., 16) */
} leo_pack_build_opts;

typedef struct leo_pack_writer leo_pack_writer;

leo_pack_result leo_pack_writer_begin(leo_pack_writer** out, const char* out_path, const leo_pack_build_opts* opts);

leo_pack_result leo_pack_writer_add(leo_pack_writer* w, const char* logical_name, const void* data, size_t size,
	int compress, int obfuscate);

leo_pack_result leo_pack_writer_end(leo_pack_writer* w);

#ifdef __cplusplus
} /* extern "C" */
#endif
