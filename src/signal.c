#include "leo/signal.h"

#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Internal helpers (not exposed)                                             */
/* -------------------------------------------------------------------------- */

static char *leo__strdup(const char *s)
{
    if (!s)
        return NULL;
#if defined(_MSC_VER)
    return _strdup(s);
#else
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p)
        memcpy(p, s, n);
    return p;
#endif
}

static leo_Signal *leo__signal_find(leo_SignalEmitter *se, const char *name)
{
    if (!se || !name)
        return NULL;
    for (leo_Signal *s = se->signals; s; s = s->next)
    {
        if (strcmp(s->name, name) == 0)
            return s;
    }
    return NULL;
}

static leo_Signal *leo__signal_find_or_create(leo_SignalEmitter *se, const char *name)
{
    if (!se || !name)
        return NULL;
    leo_Signal *s = leo__signal_find(se, name);
    if (s)
        return s;

    s = (leo_Signal *)malloc(sizeof *s);
    if (!s)
        return NULL;

    s->name = leo__strdup(name);
    if (!s->name)
    {
        free(s);
        return NULL;
    }

    s->callbacks = NULL;
    s->next = se->signals;
    se->signals = s;
    return s;
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

void leo_signal_emitter_init(leo_SignalEmitter *se, void *owner)
{
    if (!se)
        return;
    se->owner = owner;
    se->signals = NULL;
}

void leo_signal_emitter_free(leo_SignalEmitter *se)
{
    if (!se)
        return;
    leo_Signal *sig = se->signals;
    while (sig)
    {
        leo_Signal *next_sig = sig->next;
        leo_Callback *cb = sig->callbacks;
        while (cb)
        {
            leo_Callback *next_cb = cb->next;
            free(cb);
            cb = next_cb;
        }
        free(sig->name);
        free(sig);
        sig = next_sig;
    }
    se->signals = NULL;
    se->owner = NULL;
}

bool leo_signal_define(leo_SignalEmitter *se, const char *signal_name)
{
    return leo__signal_find_or_create(se, signal_name) != NULL;
}

bool leo_signal_is_defined(leo_SignalEmitter *se, const char *signal_name)
{
    return leo__signal_find(se, signal_name) != NULL;
}

void leo_signal_connect(leo_SignalEmitter *se, const char *signal_name, leo_SignalCallback callback, void *user_data)
{
    if (!se || !signal_name || !callback)
        return;
    leo_Signal *sig = leo__signal_find_or_create(se, signal_name);
    if (!sig)
        return;

    leo_Callback *node = (leo_Callback *)malloc(sizeof *node);
    if (!node)
        return;

    node->fn = callback;
    node->user_data = user_data;
    node->next = sig->callbacks; /* LIFO dispatch: newest-first */
    sig->callbacks = node;
}

void leo_signal_disconnect(leo_SignalEmitter *se, const char *signal_name, leo_SignalCallback callback, void *user_data)
{
    if (!se || !signal_name || !callback)
        return;
    leo_Signal *sig = leo__signal_find(se, signal_name);
    if (!sig)
        return;

    leo_Callback **prev = &sig->callbacks;
    leo_Callback *cb = sig->callbacks;
    while (cb)
    {
        if (cb->fn == callback && cb->user_data == user_data)
        {
            *prev = cb->next;
            free(cb);
            return;
        }
        prev = &cb->next;
        cb = cb->next;
    }
}

void leo_signal_disconnect_all(leo_SignalEmitter *se, const char *signal_name)
{
    if (!se || !signal_name)
        return;
    leo_Signal *sig = leo__signal_find(se, signal_name);
    if (!sig)
        return;

    leo_Callback *cb = sig->callbacks;
    while (cb)
    {
        leo_Callback *next = cb->next;
        free(cb);
        cb = next;
    }
    sig->callbacks = NULL;
}

void leo_signal_emit(leo_SignalEmitter *se, const char *signal_name, ...)
{
    if (!se || !signal_name)
        return;
    leo_Signal *sig = leo__signal_find(se, signal_name);
    if (!sig || !sig->callbacks)
        return;

    va_list ap;
    va_start(ap, signal_name);

    for (leo_Callback *cb = sig->callbacks; cb; cb = cb->next)
    {
        va_list ap_copy;
        va_copy(ap_copy, ap);
        cb->fn(se->owner, cb->user_data, ap_copy);
        va_end(ap_copy);
    }

    va_end(ap);
}

void leo_signal_emitv(leo_SignalEmitter *se, const char *signal_name, va_list ap_in)
{
    if (!se || !signal_name)
        return;
    leo_Signal *sig = leo__signal_find(se, signal_name);
    if (!sig || !sig->callbacks)
        return;

    for (leo_Callback *cb = sig->callbacks; cb; cb = cb->next)
    {
        va_list ap_copy;
        va_copy(ap_copy, ap_in);
        cb->fn(se->owner, cb->user_data, ap_copy);
        va_end(ap_copy);
    }
}
