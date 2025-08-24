#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Result codes for pack reader/writer and helpers. */
typedef enum
{
	LEO_PACK_OK = 0,

	/* Format / integrity */
	LEO_PACK_E_FORMAT, /* bad magic/version/structure */
	LEO_PACK_E_CRC, /* CRC mismatch */

	/* I/O & memory */
	LEO_PACK_E_IO, /* file I/O error */
	LEO_PACK_E_OOM, /* out of memory */
	LEO_PACK_E_NOSPACE, /* destination buffer too small */

	/* API / flow */
	LEO_PACK_E_ARG, /* invalid argument */
	LEO_PACK_E_STATE, /* wrong call order / invalid internal state */
	LEO_PACK_E_NOTFOUND, /* entry not found */

	/* Compression / obfuscation */
	LEO_PACK_E_BAD_PASSWORD, /* missing/wrong password for obfuscated data */
	LEO_PACK_E_COMPRESS, /* compression failed */
	LEO_PACK_E_DECOMPRESS /* decompression failed */
} leo_pack_result;

/* Convenience helpers */
const char* leo_pack_result_str(leo_pack_result r);

#ifdef __cplusplus
} /* extern "C" */
#endif
