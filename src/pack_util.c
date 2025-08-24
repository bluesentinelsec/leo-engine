#include "leo/pack_util.h"
#include <stdint.h>
#include <stddef.h>

/* 64-bit FNV-1a */
uint64_t leo_fnv1a64(const void* data, size_t n)
{
	const uint8_t* p = (const uint8_t*)data;
	uint64_t h = 0xcbf29ce484222325ULL; /* FNV offset basis */
	const uint64_t prime = 0x100000001b3ULL; /* FNV prime */
	for (size_t i = 0; i < n; ++i)
	{
		h ^= (uint64_t)p[i];
		h *= prime;
	}
	return h;
}

/* Bitwise (table-less) CRC-32 (IEEE 802.3) — slower but tiny & portable */
uint32_t leo_crc32_ieee(const void* data, size_t n, uint32_t seed)
{
	const uint8_t* p = (const uint8_t*)data;
	uint32_t crc = ~seed; /* seed==0 gives standard initial all-ones */
	for (size_t i = 0; i < n; ++i)
	{
		crc ^= p[i];
		for (int b = 0; b < 8; ++b)
		{
			uint32_t mask = -(crc & 1u);
			crc = (crc >> 1) ^ (0xEDB88320u & mask);
		}
	}
	return ~crc;
}
