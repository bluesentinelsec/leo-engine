#include "leo/json.h"
#include "leo/error.h"
#include "leo/io.h"            /* for leo_ReadAsset(...) */
#include <cJSON/cJSON.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
	cJSON* root;
} Doc;

static inline leo_JsonNode wrap(cJSON* p)
{
	leo_JsonNode n;
	n._p = (void*)p;
	return n;
}

static inline cJSON* un(leo_JsonNode n) { return (cJSON*)n._p; }

/* Optional: if you have custom allocators, initialize cJSON hooks once.
   Call this at engine init, or make this file-scope function and call it before first parse. */
static void _leo_json_init_hooks_once(void)
{
#if 0
    static int inited = 0;
    if (!inited) {
        cJSON_Hooks hooks = {
            .malloc_fn = leo_malloc,   /* or your engine allocator */
            .free_fn   = leo_free
        };
        cJSON_InitHooks(&hooks);
        inited = 1;
    }
#endif
}

leo_JsonDoc* leo_json_parse(const char* data, size_t len, const char** err_msg)
{
	(void)len; /* cJSON reads until first NUL */
	if (!data)
	{
		if (err_msg) *err_msg = "null buffer";
		return NULL;
	}

	/* _leo_json_init_hooks_once(); // enable if using custom allocators */
	cJSON* r = cJSON_Parse(data);
	if (!r)
	{
		if (err_msg) *err_msg = cJSON_GetErrorPtr(); /* points into data */
		return NULL;
	}
	Doc* d = (Doc*)malloc(sizeof(Doc));
	if (!d)
	{
		cJSON_Delete(r);
		if (err_msg) *err_msg = "OOM";
		return NULL;
	}
	d->root = r;
	return (leo_JsonDoc*)d;
}

leo_JsonDoc* leo_json_load(const char* logicalName, const char** err_msg)
{
	if (!logicalName || !*logicalName)
	{
		if (err_msg) *err_msg = "invalid path";
		return NULL;
	}

	/* 1) Probe size via VFS (same pattern as image loader) */
	size_t need = 0;
	(void)leo_ReadAsset(logicalName, NULL, 0, &need); /* probe: out_total -> need */
	if (need == 0)
	{
		if (err_msg) *err_msg = "asset not found";
		return NULL;
	}

	/* 2) Read into temporary buffer and NUL-terminate for cJSON */
	unsigned char* buf = (unsigned char*)malloc(need + 1);
	if (!buf)
	{
		if (err_msg) *err_msg = "OOM";
		return NULL;
	}
	size_t got = leo_ReadAsset(logicalName, buf, need, NULL);
	if (got != need)
	{
		free(buf);
		if (err_msg) *err_msg = "asset read failed";
		return NULL;
	}
	buf[need] = '\0';

	/* 3) Parse */
	const char* perr = NULL;
	leo_JsonDoc* doc = leo_json_parse((const char*)buf, need, &perr);
	free(buf);

	if (!doc)
	{
		if (err_msg) *err_msg = perr ? perr : "parse error";
		return NULL;
	}
	return doc;
}

void leo_json_free(leo_JsonDoc* doc)
{
	if (!doc) return;
	Doc* d = (Doc*)doc;
	cJSON_Delete(d->root);
	free(d);
}

leo_JsonNode leo_json_root(const leo_JsonDoc* doc)
{
	const Doc* d = (const Doc*)doc;
	return d ? wrap(d->root) : wrap(NULL);
}

/* Type checks */
bool leo_json_is_null(leo_JsonNode n) { return un(n) == NULL; }
bool leo_json_is_object(leo_JsonNode n) { return cJSON_IsObject(un(n)); }
bool leo_json_is_array(leo_JsonNode n) { return cJSON_IsArray(un(n)); }
bool leo_json_is_string(leo_JsonNode n) { return cJSON_IsString(un(n)); }
bool leo_json_is_number(leo_JsonNode n) { return cJSON_IsNumber(un(n)); }
bool leo_json_is_bool(leo_JsonNode n) { return cJSON_IsBool(un(n)); }

/* Object & array access */
leo_JsonNode leo_json_obj_get(leo_JsonNode obj, const char* key)
{
	cJSON* o = un(obj);
	if (!cJSON_IsObject(o) || !key) return wrap(NULL);
	return wrap(cJSON_GetObjectItemCaseSensitive(o, key));
}

size_t leo_json_arr_size(leo_JsonNode arr)
{
	cJSON* a = un(arr);
	return cJSON_IsArray(a) ? (size_t)cJSON_GetArraySize(a) : 0;
}

leo_JsonNode leo_json_arr_get(leo_JsonNode arr, size_t index)
{
	cJSON* a = un(arr);
	if (!cJSON_IsArray(a)) return wrap(NULL);
	return wrap(cJSON_GetArrayItem(a, (int)index));
}

/* Typed getters */
bool leo_json_get_string(leo_JsonNode obj, const char* key, const char** out)
{
	cJSON* s = un(leo_json_obj_get(obj, key));
	if (cJSON_IsString(s) && s->valuestring)
	{
		if (out) *out = s->valuestring;
		return true;
	}
	return false;
}

bool leo_json_get_int(leo_JsonNode obj, const char* key, int* out)
{
	cJSON* n = un(leo_json_obj_get(obj, key));
	if (cJSON_IsNumber(n))
	{
		if (out) *out = n->valueint;
		return true;
	}
	return false;
}

bool leo_json_get_double(leo_JsonNode obj, const char* key, double* out)
{
	cJSON* n = un(leo_json_obj_get(obj, key));
	if (cJSON_IsNumber(n))
	{
		if (out) *out = n->valuedouble;
		return true;
	}
	return false;
}

bool leo_json_get_bool(leo_JsonNode obj, const char* key, bool* out)
{
	cJSON* b = un(leo_json_obj_get(obj, key));
	if (cJSON_IsBool(b))
	{
		if (out) *out = cJSON_IsTrue(b);
		return true;
	}
	return false;
}

/* Direct as_* */
const char* leo_json_as_string(leo_JsonNode n)
{
	cJSON* s = un(n);
	return cJSON_IsString(s) ? s->valuestring : NULL;
}

int leo_json_as_int(leo_JsonNode n)
{
	cJSON* x = un(n);
	return cJSON_IsNumber(x) ? x->valueint : 0;
}

double leo_json_as_double(leo_JsonNode n)
{
	cJSON* x = un(n);
	return cJSON_IsNumber(x) ? x->valuedouble : 0.0;
}

bool leo_json_as_bool(leo_JsonNode n)
{
	cJSON* x = un(n);
	return cJSON_IsBool(x) ? cJSON_IsTrue(x) : false;
}
