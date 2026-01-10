/* ==========================================================
 * File: src/pack_errors.c
 * ========================================================== */

#include "leo/pack_errors.h"

#include <stddef.h>

/* Keep this in sync with leo/pack_errors.h */
typedef struct
{
    leo_pack_result code;
    const char *name;
    const char *msg;
} _ErrRec;

static const _ErrRec kErrs[] = {
    {LEO_PACK_OK, "LEO_PACK_OK", "OK"},
    {LEO_PACK_E_ARG, "LEO_PACK_E_ARG", "Invalid argument"},
    {LEO_PACK_E_IO, "LEO_PACK_E_IO", "I/O error"},
    {LEO_PACK_E_OOM, "LEO_PACK_E_OOM", "Out of memory"},
    {LEO_PACK_E_FORMAT, "LEO_PACK_E_FORMAT", "Corrupt or unsupported pack format"},
    {LEO_PACK_E_NOTFOUND, "LEO_PACK_E_NOTFOUND", "Entry not found"},
    {LEO_PACK_E_NOSPACE, "LEO_PACK_E_NOSPACE", "Output buffer too small"},
    {LEO_PACK_E_COMPRESS, "LEO_PACK_E_COMPRESS", "Compression failed"},
    {LEO_PACK_E_DECOMPRESS, "LEO_PACK_E_DECOMPRESS", "Decompression failed"},
    {LEO_PACK_E_BAD_PASSWORD, "LEO_PACK_E_BAD_PASSWORD", "Bad or missing password for obfuscated entry"},
    {LEO_PACK_E_STATE, "LEO_PACK_E_STATE", "Invalid state or duplicate entry"},
};

static const _ErrRec *_find(leo_pack_result r)
{
    for (size_t i = 0; i < sizeof(kErrs) / sizeof(kErrs[0]); ++i)
    {
        if (kErrs[i].code == r)
            return &kErrs[i];
    }
    return NULL;
}

const char *leo_pack_result_str(leo_pack_result r)
{
    const _ErrRec *e = _find(r);
    return e ? e->name : "LEO_PACK_E_UNKNOWN";
}

const char *leo_pack_errmsg(leo_pack_result r)
{
    const _ErrRec *e = _find(r);
    return e ? e->msg : "Unknown pack error";
}
