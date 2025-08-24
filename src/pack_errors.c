#include "leo/pack_errors.h"

const char* leo_pack_result_str(leo_pack_result r)
{
	switch (r)
	{
	case LEO_PACK_OK: return "OK";

	case LEO_PACK_E_FORMAT: return "FORMAT_ERROR";
	case LEO_PACK_E_CRC: return "CRC_MISMATCH";

	case LEO_PACK_E_IO: return "IO_ERROR";
	case LEO_PACK_E_OOM: return "OUT_OF_MEMORY";
	case LEO_PACK_E_NOSPACE: return "NO_SPACE";

	case LEO_PACK_E_ARG: return "INVALID_ARGUMENT";
	case LEO_PACK_E_STATE: return "INVALID_STATE";
	case LEO_PACK_E_NOTFOUND: return "NOT_FOUND";

	case LEO_PACK_E_BAD_PASSWORD: return "BAD_PASSWORD";
	case LEO_PACK_E_COMPRESS: return "COMPRESS_FAILED";
	case LEO_PACK_E_DECOMPRESS: return "DECOMPRESS_FAILED";

	default: return "UNKNOWN";
	}
}
