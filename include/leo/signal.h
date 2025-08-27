#pragma once
/*
 * leo/signal.h — lightweight, Godot-style signals for Leo Engine 2.0
 *
 * - Embed leo_SignalEmitter in your actor/subsystem structs.
 * - Connect callbacks receiving (owner, user_data, va_list).
 * - Emit synchronously via leo_signal_emit / leo_signal_emitv.
 *
 * Notes:
 *  - Dispatch order is LIFO (newest connection fires first).
 *  - This header contains no inline functions; definitions are in signal.c.
 */

#include "leo/export.h"
#include <stdarg.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Forward declarations / public types */
    typedef struct leo_SignalEmitter leo_SignalEmitter;
    typedef struct leo_Signal leo_Signal;
    typedef struct leo_Callback leo_Callback;

    /* Callback signature: owner is the struct embedding the emitter */
    typedef void (*leo_SignalCallback)(void *owner, void *user_data, va_list ap);

    /* Single linked-list node for a connected callback (publicly visible layout for POD; modify only through API) */
    struct leo_Callback
    {
        leo_SignalCallback fn;
        void *user_data;
        struct leo_Callback *next;
    };

    /* A named signal with its callback list */
    struct leo_Signal
    {
        char *name;
        struct leo_Callback *callbacks; /* newest-first */
        struct leo_Signal *next;
    };

    /* The thing you embed into your actor/subsystem */
    struct leo_SignalEmitter
    {
        void *owner; /* back-pointer to the struct that owns this emitter */
        struct leo_Signal *signals;
    };

    /* API — lifecycle */
    LEO_API void leo_signal_emitter_init(leo_SignalEmitter *se, void *owner);
    LEO_API void leo_signal_emitter_free(leo_SignalEmitter *se);

    /* API — optional definition helpers */
    LEO_API bool leo_signal_define(leo_SignalEmitter *se, const char *signal_name);
    LEO_API bool leo_signal_is_defined(leo_SignalEmitter *se, const char *signal_name);

    /* API — connect / disconnect */
    LEO_API void leo_signal_connect(leo_SignalEmitter *se, const char *signal_name, leo_SignalCallback callback,
                                    void *user_data);

    LEO_API void leo_signal_disconnect(leo_SignalEmitter *se, const char *signal_name, leo_SignalCallback callback,
                                       void *user_data);

    LEO_API void leo_signal_disconnect_all(leo_SignalEmitter *se, const char *signal_name);

    /* API — emit */
    LEO_API void leo_signal_emit(leo_SignalEmitter *se, const char *signal_name, ...);
    LEO_API void leo_signal_emitv(leo_SignalEmitter *se, const char *signal_name, va_list ap_in);

#ifdef __cplusplus
} /* extern "C" */
#endif
