#pragma once
#include "leo/pack_errors.h"
#include <stddef.h>
#include <stdint.h>

typedef struct leo_pack leo_pack;
typedef struct
{
    const char *name; /* transient pointer valid until close */
    uint16_t flags;
    uint64_t size_uncompressed;
    uint64_t size_stored;
} leo_pack_stat;

leo_pack_result leo_pack_open_file(leo_pack **out, const char *path, const char *password);
void leo_pack_close(leo_pack *p);

int leo_pack_count(leo_pack *p);
leo_pack_result leo_pack_stat_index(leo_pack *p, int index, leo_pack_stat *out);
leo_pack_result leo_pack_find(leo_pack *p, const char *name, int *out_index);

leo_pack_result leo_pack_extract_index(leo_pack *p, int index, void *dst, size_t dst_cap, size_t *out_size);
