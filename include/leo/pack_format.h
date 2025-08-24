#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define LEO_PACK_MAGIC "LEOPACK\0"

    enum
    {
        LEO_PACK_V1 = 1
    };

    enum
    {
        LEO_PACK_FLAG_OBFUSCATED = 1u << 0
    };

    enum
    {
        LEO_PE_COMPRESSED = 1u << 0,
        LEO_PE_OBFUSCATED = 1u << 1
    };

    typedef struct
    {
        char magic[8];        /* "LEOPACK\0" */
        uint32_t version;     /* 1 */
        uint32_t pack_flags;  /* bit 0: any obfuscation present */
        uint64_t toc_offset;  /* absolute */
        uint64_t toc_size;    /* bytes */
        uint64_t data_offset; /* absolute start of payload region */
        uint64_t pack_salt;   /* random salt for XOR seed */
        uint32_t reserved[8]; /* future */
        /* CRC32 of the whole header with this crc field zeroed (implementation-defined header size). */
        uint32_t header_crc32; /* crc over bytes [0..0x4F] */
    } leo_pack_header_v1;

    /* Entry metadata (name is serialized as length + bytes right before this struct) */
    typedef struct
    {
        uint16_t flags;    /* PE_* */
        uint16_t name_len; /* for convenience on readback */
        uint64_t offset;   /* absolute payload offset */
        uint64_t size_uncompressed;
        uint64_t size_stored;
        uint32_t crc32_uncompressed;
    } leo_pack_entry_v1;

#ifdef __cplusplus
} /* extern "C" */
#endif
