#pragma once
#include "leo/export.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        LEO_CSV_OK = 0,
        LEO_CSV_E_ARG,
        LEO_CSV_E_OOM,
        LEO_CSV_E_FORMAT
    } leo_csv_result;

    typedef struct
    {
        char delimiter; /* default: ',' */
        char quote;     /* default: '"' */
        int trim_ws;    /* nonzero => trim ASCII space/tabs around unquoted fields */
        int allow_crlf; /* nonzero => tolerate CRLF and LF line breaks (default 1) */
    } leo_csv_opts;

    /* Called for every parsed cell (row, col are 0-based).
     * 'cell' is a pointer into a scratch buffer valid until the next callback
     * invocation finishes. The string is NOT NUL-terminated; use 'len'. Return
     * nonzero from the callback to abort parsing early.
     */
    typedef int (*leo_csv_cell_cb)(void *user, const char *cell, size_t len, size_t row, size_t col);

    /* Streaming CSV parse with callback.
     * - If 'opts' is NULL, sensible defaults are used.
     * - 'data' may contain CRLF or LF. Handles quoted fields with "" escapes.
     */
    LEO_API leo_csv_result leo_csv_parse(const char *data, size_t len, const leo_csv_opts *opts,
                                         leo_csv_cell_cb on_cell, void *user);

    /* Convenience: parse a CSV of integers (decimal, optional +/-, ignores
     * whitespace and newlines). Accepts either:
     *  - true CSV with delimiter/quotes (it will still honor quotes if present), or
     *  - a simple list separated by commas/whitespace/newlines.
     * Allocates an array of uint32_t. Caller must free(*out) on success.
     */
    LEO_API leo_csv_result leo_csv_parse_uint32_alloc(const char *data, size_t len, uint32_t **out, size_t *out_count,
                                                      const leo_csv_opts *opts);

    /* A tiny helper to quickly count values in a CSV integer list without
     * allocating output. */
    LEO_API size_t leo_csv_count_values(const char *data, size_t len, const leo_csv_opts *opts);

#ifdef __cplusplus
} /* extern "C" */
#endif
