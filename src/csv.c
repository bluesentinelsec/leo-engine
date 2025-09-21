#include "leo/csv.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static leo_csv_opts default_opts(void)
{
    leo_csv_opts o;
    o.delimiter = ',';
    o.quote = '"';
    o.trim_ws = 1;
    o.allow_crlf = 1;
    return o;
}

static const char *rstrip_ws(const char *s, const char *end)
{
    while (end > s && (unsigned char)end[-1] <= 0x20)
        --end; /* ASCII <= space */
    return end;
}

static const char *lskip_ws(const char *s, const char *end)
{
    while (s < end && (unsigned char)*s <= 0x20)
        ++s;
    return s;
}

/* Advance past one line ending (CRLF or LF). Returns new cursor. */
static inline const char *advance_line(const char *q, const char *end, int allow_crlf)
{
    if (q >= end)
        return q;
    if (*q == '\r' && allow_crlf)
    {
        if (q + 1 < end && q[1] == '\n')
            return q + 2;
        return q + 1;
    }
    if (*q == '\n')
        return q + 1;
    return q;
}

leo_csv_result leo_csv_parse(const char *data, size_t len, const leo_csv_opts *opt_in, leo_csv_cell_cb on_cell,
                             void *user)
{
    if (!data || !on_cell)
        return LEO_CSV_E_ARG;
    leo_csv_opts opt = opt_in ? *opt_in : default_opts();

    const char *p = data;
    const char *end = data + len;
    size_t row = 0, col = 0;

    /* If the user asks to abort (non-zero from callback), we finish the CURRENT row,
       then stop before starting the next row. */
    int abort_after_row = 0;

    /* Scratch buffer for unescaping quoted fields; grow-on-demand. */
    size_t cap = 256;
    char *buf = (char *)malloc(cap);
    if (!buf)
        return LEO_CSV_E_OOM;

    while (p < end)
    {
        const char *cell_start = p;
        size_t out_len = 0;
        int ended_row = 0; /* set to 1 if we consumed an EOL in this iteration */

        if (*p == opt.quote)
        {
            /* -------- quoted field -------- */
            ++p; /* opening quote */
            while (p < end)
            {
                const char c = *p++;
                if (c == opt.quote)
                {
                    if (p < end && *p == opt.quote)
                    {
                        if (out_len + 1 > cap)
                        {
                            size_t ncap = cap * 2;
                            char *nbuf = (char *)realloc(buf, ncap);
                            if (!nbuf)
                            {
                                free(buf);
                                return LEO_CSV_E_OOM;
                            }
                            buf = nbuf;
                            cap = ncap;
                        }
                        buf[out_len++] = opt.quote;
                        ++p; /* escaped quote */
                        continue;
                    }
                    /* end quoted field */
                    break;
                }
                if (out_len + 1 > cap)
                {
                    size_t ncap = cap * 2;
                    char *nbuf = (char *)realloc(buf, ncap);
                    if (!nbuf)
                    {
                        free(buf);
                        return LEO_CSV_E_OOM;
                    }
                    buf = nbuf;
                    cap = ncap;
                }
                buf[out_len++] = c;
            }

            /* skip trailing spaces before delimiter/EOL */
            const char *q = p;
            while (q < end && (unsigned char)*q <= 0x20 && *q != opt.delimiter && *q != '\r' && *q != '\n')
                ++q;

            if (q < end && *q == opt.delimiter)
            {
                p = q + 1;
            }
            else if (q < end && (*q == '\r' || *q == '\n'))
            {
                p = advance_line(q, end, opt.allow_crlf);
                ended_row = 1;
            }
            else if (q >= end)
            {
                p = q;
            }
            else
            {
                free(buf);
                return LEO_CSV_E_FORMAT;
            }

            /* emit */
            if (on_cell(user, buf, out_len, row, col))
            {
                abort_after_row = 1; /* defer stop until row end */
            }
            ++col;
        }
        else
        {
            /* -------- unquoted field -------- */
            const char *q = p;
            while (q < end && *q != opt.delimiter && *q != '\r' && *q != '\n')
                ++q;

            const char *s = cell_start;
            const char *e = q;
            if (opt.trim_ws)
            {
                s = lskip_ws(s, e);
                e = rstrip_ws(s, e);
            }

            if (on_cell(user, s, (size_t)(e - s), row, col))
            {
                abort_after_row = 1; /* defer stop until row end */
            }
            ++col;

            if (q < end && *q == opt.delimiter)
            {
                p = q + 1;
            }
            else if (q < end && (*q == '\r' || *q == '\n'))
            {
                p = advance_line(q, end, opt.allow_crlf);
                ended_row = 1;
            }
            else
            {
                p = q; /* end of buffer */
            }
        }

        if (ended_row)
        {
            /* finish row; honor deferred abort */
            row += 1;
            col = 0;
            if (abort_after_row)
            {
                free(buf);
                return LEO_CSV_OK;
            }
        }
    }

    free(buf);
    return LEO_CSV_OK;
}

/* ---------- integer list helper ---------- */

typedef struct
{
    uint32_t *out;
    size_t cap, count;
} u32_accum;

static int on_cell_u32(void *u, const char *cell, size_t len, size_t row, size_t col)
{
    (void)row;
    (void)col;
    u32_accum *a = (u32_accum *)u;

    /* Trim ASCII spaces */
    const char *s = cell;
    const char *e = cell + len;
    while (s < e && (unsigned char)*s <= 0x20)
        ++s;
    while (e > s && (unsigned char)e[-1] <= 0x20)
        --e;
    if (s == e)
        return 0;

    /* First, try strict parse from the start (optional sign + digits). */
    const char *p = s;
    int sign = 1;
    if (p < e && *p == '+')
    {
        ++p;
    }
    else if (p < e && *p == '-')
    {
        sign = -1;
        ++p;
    }

    uint64_t v = 0;
    int any = 0;
    while (p < e && *p >= '0' && *p <= '9')
    {
        v = v * 10u + (uint64_t)(*p - '0');
        ++p;
        any = 1;
        if (v > 0xFFFFFFFFull)
            v = 0xFFFFFFFFull; /* clamp */
    }

    /* If no digits were found at the start (e.g., stray quotes or wrappers),
       scan inside the slice for the first [+|-]? [0-9]+ token and parse it. */
    if (!any)
    {
        const char *it = s;
        while (it < e)
        {
            /* skip non-sign, non-digit chars */
            while (it < e && !((*it >= '0' && *it <= '9') || *it == '+' || *it == '-'))
                ++it;
            if (it >= e)
                break;

            /* optional sign */
            int inner_sign = 1;
            if (*it == '+')
            {
                ++it;
            }
            else if (*it == '-')
            {
                inner_sign = -1;
                ++it;
            }

            /* must have at least one digit */
            if (it < e && *it >= '0' && *it <= '9')
            {
                uint64_t vv = 0;
                do
                {
                    vv = vv * 10u + (uint64_t)(*it - '0');
                    ++it;
                    if (vv > 0xFFFFFFFFull)
                        vv = 0xFFFFFFFFull; /* clamp */
                } while (it < e && *it >= '0' && *it <= '9');

                v = vv;
                sign = inner_sign;
                any = 1;
                break;
            }
            /* if it was a stray sign not followed by digit, continue scanning */
        }
        if (!any)
            return 0; /* still nothing numeric; ignore cell */
    }

    uint32_t outv = (sign < 0) ? (uint32_t)(-(int64_t)v) : (uint32_t)v;

    if (a->count == a->cap)
    {
        size_t ncap = a->cap ? a->cap * 2 : 256;
        uint32_t *n = (uint32_t *)realloc(a->out, ncap * sizeof(uint32_t));
        if (!n)
            return 1; /* abort on OOM */
        a->out = n;
        a->cap = ncap;
    }
    a->out[a->count++] = outv;
    return 0;
}

leo_csv_result leo_csv_parse_uint32_alloc(const char *data, size_t len, uint32_t **out, size_t *out_count,
                                          const leo_csv_opts *opt_in)
{
    if (!out)
        return LEO_CSV_E_ARG;
    *out = NULL;
    if (out_count)
        *out_count = 0;

    u32_accum acc;
    acc.out = NULL;
    acc.cap = 0;
    acc.count = 0;
    leo_csv_opts opt = opt_in ? *opt_in : default_opts();

    leo_csv_result r = leo_csv_parse(data, len, &opt, on_cell_u32, &acc);
    if (r != LEO_CSV_OK)
    {
        free(acc.out);
        return r;
    }

    *out = acc.out;
    if (out_count)
        *out_count = acc.count;
    return LEO_CSV_OK;
}

size_t leo_csv_count_values(const char *data, size_t len, const leo_csv_opts *opt_in)
{
    if (!data)
        return 0;
    leo_csv_opts opt = opt_in ? *opt_in : default_opts();

    size_t count = 0;
    const char *p = data;
    const char *end = data + len;
    int in_quote = 0;

    while (p < end)
    {
        char c = *p++;
        if (in_quote)
        {
            if (c == opt.quote)
            {
                if (p < end && *p == opt.quote)
                {
                    ++p; /* "" */
                }
                else
                    in_quote = 0;
            }
        }
        else
        {
            if (c == opt.quote)
                in_quote = 1;
            else if (c == opt.delimiter || c == '\n' || c == '\r')
                count++;
        }
    }
    if (len > 0 && (data[len - 1] != opt.delimiter && data[len - 1] != '\n' && data[len - 1] != '\r'))
        count++;

    return count;
}
