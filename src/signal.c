#include "leo/signal.h"
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>   /* only here, not exposed in header */

/* -------------------------------- Helpers -------------------------------- */

static char* leo__strdup(const char* s)
{
	if (!s) return NULL;
#if defined(_MSC_VER)
    return _strdup(s);
#else
	size_t n = strlen(s) + 1;
	char* p = (char*)malloc(n);
	if (p)
		memcpy(p, s, n);
	return p;
#endif
}

static void leo__lock(leo_SignalEmitter* se) { if (se && se->_lock) SDL_LockMutex((SDL_Mutex*)se->_lock); }
static void leo__unlock(leo_SignalEmitter* se) { if (se && se->_lock) SDL_UnlockMutex((SDL_Mutex*)se->_lock); }

static leo_Signal* leo__signal_find(leo_SignalEmitter* se, const char* name)
{
	if (!se || !name) return NULL;
	for (leo_Signal* s = se->signals; s; s = s->next)
		if (strcmp(s->name, name) == 0) return s;
	return NULL;
}

static leo_Signal* leo__signal_find_or_create(leo_SignalEmitter* se, const char* name)
{
	if (!se || !name) return NULL;
	leo_Signal* s = leo__signal_find(se, name);
	if (s) return s;

	s = (leo_Signal*)malloc(sizeof *s);
	if (!s) return NULL;
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

/* ------------------------------- Public API ------------------------------- */

void leo_signal_emitter_init(leo_SignalEmitter* se, void* owner)
{
	if (!se) return;
	se->owner = owner;
	se->signals = NULL;
	se->_lock = SDL_CreateMutex(); /* hidden from header */
}

void leo_signal_emitter_free(leo_SignalEmitter* se)
{
	if (!se) return;

	leo__lock(se);
	leo_Signal* sig = se->signals;
	while (sig)
	{
		leo_Signal* next_sig = sig->next;
		leo_Callback* cb = sig->callbacks;
		while (cb)
		{
			leo_Callback* next_cb = cb->next;
			free(cb);
			cb = next_cb;
		}
		free(sig->name);
		free(sig);
		sig = next_sig;
	}
	se->signals = NULL;
	se->owner = NULL;
	leo__unlock(se);

	if (se->_lock)
	{
		SDL_DestroyMutex((SDL_Mutex*)se->_lock);
		se->_lock = NULL;
	}
}

bool leo_signal_define(leo_SignalEmitter* se, const char* signal_name)
{
	if (!se || !signal_name) return false;
	bool ok = false;
	leo__lock(se);
	ok = (leo__signal_find_or_create(se, signal_name) != NULL);
	leo__unlock(se);
	return ok;
}

bool leo_signal_is_defined(leo_SignalEmitter* se, const char* signal_name)
{
	if (!se || !signal_name) return false;
	bool found;
	leo__lock(se);
	found = (leo__signal_find(se, signal_name) != NULL);
	leo__unlock(se);
	return found;
}

void leo_signal_connect(leo_SignalEmitter* se,
	const char* signal_name,
	leo_SignalCallback callback,
	void* user_data)
{
	if (!se || !signal_name || !callback) return;

	leo__lock(se);
	leo_Signal* sig = leo__signal_find_or_create(se, signal_name);
	if (sig)
	{
		leo_Callback* node = (leo_Callback*)malloc(sizeof *node);
		if (node)
		{
			node->fn = callback;
			node->user_data = user_data;
			node->next = sig->callbacks;
			sig->callbacks = node;
		}
	}
	leo__unlock(se);
}

void leo_signal_disconnect(leo_SignalEmitter* se,
	const char* signal_name,
	leo_SignalCallback callback,
	void* user_data)
{
	if (!se || !signal_name || !callback) return;

	leo__lock(se);
	leo_Signal* sig = leo__signal_find(se, signal_name);
	if (sig)
	{
		leo_Callback** prev = &sig->callbacks;
		for (leo_Callback* cb = sig->callbacks; cb; cb = cb->next)
		{
			if (cb->fn == callback && cb->user_data == user_data)
			{
				*prev = cb->next;
				free(cb);
				break;
			}
			prev = &cb->next;
		}
	}
	leo__unlock(se);
}

void leo_signal_disconnect_all(leo_SignalEmitter* se, const char* signal_name)
{
	if (!se || !signal_name) return;

	leo__lock(se);
	leo_Signal* sig = leo__signal_find(se, signal_name);
	if (sig)
	{
		leo_Callback* cb = sig->callbacks;
		while (cb)
		{
			leo_Callback* next = cb->next;
			free(cb);
			cb = next;
		}
		sig->callbacks = NULL;
	}
	leo__unlock(se);
}

void leo_signal_emit(leo_SignalEmitter* se, const char* signal_name, ...)
{
	if (!se || !signal_name) return;

	leo__lock(se);
	leo_Signal* sig = leo__signal_find(se, signal_name);
	leo_Callback* head = sig ? sig->callbacks : NULL;
	leo__unlock(se);

	if (!head) return;

	va_list ap;
	va_start(ap, signal_name);
	for (leo_Callback* cb = head; cb; cb = cb->next)
	{
		va_list ap_copy;
		va_copy(ap_copy, ap);
		cb->fn(se->owner, cb->user_data, ap_copy);
		va_end(ap_copy);
	}
	va_end(ap);
}

void leo_signal_emitv(leo_SignalEmitter* se, const char* signal_name, va_list ap_in)
{
	if (!se || !signal_name) return;

	leo__lock(se);
	leo_Signal* sig = leo__signal_find(se, signal_name);
	leo_Callback* head = sig ? sig->callbacks : NULL;
	leo__unlock(se);

	if (!head) return;

	for (leo_Callback* cb = head; cb; cb = cb->next)
	{
		va_list ap_copy;
		va_copy(ap_copy, ap_in);
		cb->fn(se->owner, cb->user_data, ap_copy);
		va_end(ap_copy);
	}
}
