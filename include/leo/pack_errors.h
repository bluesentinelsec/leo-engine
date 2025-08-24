#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum leo_pack_result
    {
        LEO_PACK_OK = 0,
        LEO_PACK_E_IO,
        LEO_PACK_E_FORMAT,
        LEO_PACK_E_VERSION,
        LEO_PACK_E_MEM,
        LEO_PACK_E_CRC,
        LEO_PACK_E_NOTFOUND,
        LEO_PACK_E_COMPRESS,
        LEO_PACK_E_DECOMPRESS,
        LEO_PACK_E_OBFUSCATE,
        LEO_PACK_E_INVALID_ARG,
        LEO_PACK_E_NOT_IMPLEMENTED
    } leo_pack_result;

    const char *leo_pack_strerror(leo_pack_result r);

#ifdef __cplusplus
}
#endif
