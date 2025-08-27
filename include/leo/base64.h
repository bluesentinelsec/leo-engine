#pragma once
#include "leo/export.h"

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Result codes for base64 operations.
typedef enum
{
	LEO_B64_OK = 0,
	LEO_B64_E_ARG, // invalid arguments (NULL, sizes, etc.)
	LEO_B64_E_NOSPACE, // destination buffer too small
	LEO_B64_E_FORMAT // invalid base64 input
} leo_b64_result;

/* Return the exact number of bytes required to base64-encode `n` bytes
 * (excluding the trailing '\0'). This always includes '=' padding to a
 * multiple of 4 output chars.
 */
LEO_API size_t leo_base64_encoded_len(size_t n);

/* Return the maximum number of bytes that could result from decoding a
 * base64 string of length `n` (ignoring whitespace). This is a safe cap
 * for destination buffer sizing; the actual decoded size may be smaller.
 */
LEO_API size_t leo_base64_decoded_cap(size_t n);


/* Encode raw bytes -> base64 characters.
 * - dst receives ASCII base64 data (no newline insertion) and is NOT NUL-terminated.
 * - If out_len is provided, it receives the number of bytes written to dst.
 * - Returns LEO_B64_E_NOSPACE if dst_cap is too small.
 */
LEO_API leo_b64_result leo_base64_encode(const void* src, size_t src_len,
	char* dst, size_t dst_cap, size_t* out_len);

/* Decode base64 (padded or unpadded). Ignores ASCII whitespace in src.
 * - dst receives raw bytes.
 * - If out_len is provided, it receives the number of bytes written to dst.
 * - Returns LEO_B64_E_FORMAT on invalid alphabet/padding sequence.
 * - Returns LEO_B64_E_NOSPACE if dst_cap is too small.
 */
LEO_API leo_b64_result leo_base64_decode(const char* src, size_t src_len,
	void* dst, size_t dst_cap, size_t* out_len);

/* Convenience helpers (malloc-based). Caller must free(*out) on success. */
LEO_API leo_b64_result leo_base64_encode_alloc(const void* src, size_t src_len,
	char** out, size_t* out_len);

LEO_API leo_b64_result leo_base64_decode_alloc(const char* src, size_t src_len,
	unsigned char** out, size_t* out_len);

#ifdef __cplusplus
}
#endif
