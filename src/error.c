/* ==========================================================
 * File: src/error.c  (thread-safe with SDL3 TLS only)
 * ========================================================== */

#include "leo/error.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_stdinc.h>
#include <stdarg.h>

#define ERROR_BUFFER_SIZE 4096

/* One TLS slot for all threads; each thread stores its own char* buffer in it. */
static SDL_TLSID s_tls_error_id; /* zero-initialized by the C runtime */

/* SDL will call this on thread exit for the value stored in this TLS slot. */
static void SDLCALL _leo_errorbuf_destructor(void *p)
{
    SDL_free(p);
}

/* Get (or lazily create) the calling thread's error buffer. */
static char *_leo_get_or_make_thread_buffer(void)
{
    /* NOTE: SDL3’s TLS API takes the ADDRESS of the SDL_TLSID. It will
       initialize the ID on first use, so we don’t need our own spinlocks. */
    char *buf = (char *)SDL_GetTLS(&s_tls_error_id);
    if (!buf)
    {
        buf = (char *)SDL_malloc(ERROR_BUFFER_SIZE);
        if (!buf)
            return NULL;
        buf[0] = '\0';

        /* Store it in TLS; SDL will call the destructor at thread exit. */
        if (!SDL_SetTLS(&s_tls_error_id, buf, _leo_errorbuf_destructor))
        {
            /* Failed to set TLS; clean up and bail. */
            SDL_free(buf);
            return NULL;
        }
    }
    return buf;
}

void leo_SetError(const char *fmt, ...)
{
    if (!fmt)
        return;

    char *buf = _leo_get_or_make_thread_buffer();
    if (!buf)
        return; /* best-effort: fail silently if OOM/TLS failure */

    va_list args;
    va_start(args, fmt);
    SDL_vsnprintf(buf, ERROR_BUFFER_SIZE, fmt, args);
    va_end(args);

    /* Ensure null-termination even if truncated */
    buf[ERROR_BUFFER_SIZE - 1] = '\0';
}

const char *leo_GetError(void)
{
    /* Return the thread’s own buffer; empty string if not yet created. */
    char *buf = (char *)SDL_GetTLS(&s_tls_error_id);
    return buf ? buf : "";
}

void leo_ClearError(void)
{
    char *buf = (char *)SDL_GetTLS(&s_tls_error_id);
    if (buf)
        buf[0] = '\0';
}
