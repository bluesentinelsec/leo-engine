#include "leo/base64.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char B64[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static unsigned char DECODE_TABLE[256];
static int DECODE_INIT = 0;

static void b64_init_table(void)
{
    if (DECODE_INIT)
        return;
    for (int i = 0; i < 256; ++i)
        DECODE_TABLE[i] = 0xFF; // invalid
    for (int i = 0; i < 64; ++i)
        DECODE_TABLE[(unsigned char)B64[i]] = (unsigned char)i;
    DECODE_TABLE[(unsigned char)'='] = 0xFE; // padding marker
    DECODE_INIT = 1;
}

static inline size_t min_size(size_t a, size_t b)
{
    return a < b ? a : b;
}

leo_b64_result leo_base64_encode(const void *src, size_t src_len, char *dst, size_t dst_cap, size_t *out_len)
{
    if ((!src && src_len) || !dst)
        return LEO_B64_E_ARG;

    const unsigned char *s = (const unsigned char *)src;
    size_t need = leo_base64_encoded_len(src_len);
    if (dst_cap < need)
        return LEO_B64_E_NOSPACE;

    size_t di = 0;
    size_t i = 0;
    while (i + 3 <= src_len)
    {
        uint32_t v = (uint32_t)s[i] << 16 | (uint32_t)s[i + 1] << 8 | (uint32_t)s[i + 2];
        dst[di++] = B64[(v >> 18) & 0x3F];
        dst[di++] = B64[(v >> 12) & 0x3F];
        dst[di++] = B64[(v >> 6) & 0x3F];
        dst[di++] = B64[v & 0x3F];
        i += 3;
    }
    if (i < src_len)
    {
        uint32_t v = (uint32_t)s[i] << 16;
        int rem = 1;
        if (i + 1 < src_len)
        {
            v |= (uint32_t)s[i + 1] << 8;
            rem = 2;
        }
        dst[di++] = B64[(v >> 18) & 0x3F];
        dst[di++] = B64[(v >> 12) & 0x3F];
        if (rem == 2)
        {
            dst[di++] = B64[(v >> 6) & 0x3F];
            dst[di++] = '=';
        }
        else
        {
            dst[di++] = '=';
            dst[di++] = '=';
        }
    }
    if (out_len)
        *out_len = di;
    return LEO_B64_OK;
}

leo_b64_result leo_base64_decode(const char *src, size_t src_len, void *dst, size_t dst_cap, size_t *out_len)
{
    if (!src || !dst)
        return LEO_B64_E_ARG;
    b64_init_table();

    unsigned char *d = (unsigned char *)dst;
    size_t di = 0;

    // We’ll collect 4 meaningful base64 chars at a time, skipping whitespace.
    unsigned char quad[4];
    int qn = 0;

    for (size_t i = 0; i < src_len; ++i)
    {
        unsigned char c = (unsigned char)src[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            continue; // ignore whitespace

        unsigned char val = DECODE_TABLE[c];
        if (val == 0xFF)
        {
            // invalid char
            return LEO_B64_E_FORMAT;
        }
        quad[qn++] = c;
        if (qn < 4)
            continue;

        // Decode quad
        unsigned char a = DECODE_TABLE[quad[0]];
        unsigned char b = DECODE_TABLE[quad[1]];
        unsigned char c2 = DECODE_TABLE[quad[2]];
        unsigned char d2 = DECODE_TABLE[quad[3]];

        if (a >= 64 || b >= 64)
            return LEO_B64_E_FORMAT;

        uint32_t v = ((uint32_t)a) << 18 | ((uint32_t)b) << 12;
        unsigned emit = 0;

        if (c2 == 0xFE && d2 == 0xFE)
        {
            // '==': one output byte
            emit = 1;
        }
        else if (c2 < 64 && d2 == 0xFE)
        {
            // last is '=': two output bytes
            v |= ((uint32_t)c2) << 6;
            emit = 2;
        }
        else if (c2 < 64 && d2 < 64)
        {
            // full triple
            v |= ((uint32_t)c2) << 6 | (uint32_t)d2;
            emit = 3;
        }
        else
        {
            return LEO_B64_E_FORMAT;
        }

        if (dst_cap < di + emit)
            return LEO_B64_E_NOSPACE;

        if (emit >= 1)
            d[di++] = (unsigned char)((v >> 16) & 0xFF);
        if (emit >= 2)
            d[di++] = (unsigned char)((v >> 8) & 0xFF);
        if (emit >= 3)
            d[di++] = (unsigned char)(v & 0xFF);

        qn = 0;
        if (emit < 3)
        {
            // If padding consumed, remaining input must be whitespace only.
            // We continue to allow trailing whitespace.
        }
    }

    // If there are leftover chars (<4) that are not whitespace-only input end → format error.
    if (qn != 0)
    {
        // Accept the case where the trailing chars are whitespace-only; but we skip those above,
        // so reaching here means we saw 1..3 non-ws chars then EOS → invalid base64.
        return LEO_B64_E_FORMAT;
    }

    if (out_len)
        *out_len = di;
    return LEO_B64_OK;
}

leo_b64_result leo_base64_encode_alloc(const void *src, size_t src_len, char **out, size_t *out_len)
{
    if (!out)
        return LEO_B64_E_ARG;
    size_t need = leo_base64_encoded_len(src_len);
    char *buf = (char *)malloc(need + 1); // +1 for NUL
    if (!buf)
        return LEO_B64_E_NOSPACE;
    size_t wrote = 0;
    leo_b64_result r = leo_base64_encode(src, src_len, buf, need, &wrote);
    if (r != LEO_B64_OK)
    {
        free(buf);
        return r;
    }
    buf[wrote] = '\0';
    *out = buf;
    if (out_len)
        *out_len = wrote;
    return LEO_B64_OK;
}

leo_b64_result leo_base64_decode_alloc(const char *src, size_t src_len, unsigned char **out, size_t *out_len)
{
    if (!src || !out)
        return LEO_B64_E_ARG;

    // Over-allocate by safe cap, then shrink by out_len.
    size_t cap = leo_base64_decoded_cap(src_len);
    unsigned char *buf = (unsigned char *)malloc(cap);
    if (!buf)
        return LEO_B64_E_NOSPACE;

    size_t wrote = 0;
    leo_b64_result r = leo_base64_decode(src, src_len, buf, cap, &wrote);
    if (r != LEO_B64_OK)
    {
        free(buf);
        return r;
    }

    // Optionally shrink with realloc (not required).
    // For simplicity, keep the full buffer.
    *out = buf;
    if (out_len)
        *out_len = wrote;
    return LEO_B64_OK;
}

size_t leo_base64_encoded_len(size_t n)
{
    return (n == 0) ? 0 : (((n + 2) / 3) * 4);
}

size_t leo_base64_decoded_cap(size_t n)
{
    return (n / 4 + 1) * 3;
}
