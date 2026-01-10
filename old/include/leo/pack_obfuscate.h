#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    uint32_t leo_xor_seed_from_password(const char *password, uint64_t pack_salt);

    void leo_xor_stream_apply(uint32_t seed, void *data, size_t n);

#ifdef __cplusplus
} /* extern "C" */
#endif
