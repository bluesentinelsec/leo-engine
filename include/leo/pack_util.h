#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t leo_fnv1a64(const void* data, size_t n);

uint32_t leo_crc32_ieee(const void* data, size_t n, uint32_t seed);

static inline uint64_t leo_align_up(uint64_t v, uint64_t a)
{
	return (v + (a - 1)) & ~(a - 1);
}

#ifdef __cplusplus
} /* extern "C" */
#endif
