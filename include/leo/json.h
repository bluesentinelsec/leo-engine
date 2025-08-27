#pragma once
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Borrowed-node model: Nodes are views into a document; invalid after doc is freed.
typedef struct leo_JsonDoc  leo_JsonDoc;
typedef struct leo_JsonNode { void* _p; } leo_JsonNode;  // opaque

// -------- Document lifecycle --------
/* Parse from memory (UTF-8). On failure returns NULL and (optionally) sets *err_msg to an internal static string. */
leo_JsonDoc*  leo_json_parse(const char* data, size_t len, const char** err_msg);
/* Load via Leo VFS (pack/filesystem). Convenience; calls your VFS under the hood. */
leo_JsonDoc*  leo_json_load(const char* path, const char** err_msg);
/* Destroy document and invalidate all nodes derived from it. */
void          leo_json_free(leo_JsonDoc* doc);

// -------- Root / navigation --------
leo_JsonNode  leo_json_root(const leo_JsonDoc* doc);
bool          leo_json_is_null(leo_JsonNode n);
bool          leo_json_is_object(leo_JsonNode n);
bool          leo_json_is_array(leo_JsonNode n);
bool          leo_json_is_string(leo_JsonNode n);
bool          leo_json_is_number(leo_JsonNode n);
bool          leo_json_is_bool(leo_JsonNode n);

// Object access (returns null node if missing)
leo_JsonNode  leo_json_obj_get(leo_JsonNode obj, const char* key);

// Array access
size_t        leo_json_arr_size(leo_JsonNode arr);
leo_JsonNode  leo_json_arr_get(leo_JsonNode arr, size_t index);

// -------- Typed getters with defaults (return true if present & correct type) --------
bool          leo_json_get_string(leo_JsonNode obj, const char* key, const char** out);
bool          leo_json_get_int(leo_JsonNode obj, const char* key, int* out);
bool          leo_json_get_double(leo_JsonNode obj, const char* key, double* out);
bool          leo_json_get_bool(leo_JsonNode obj, const char* key, bool* out);

// Direct value from a node (for array elements/inline reads)
const char*   leo_json_as_string(leo_JsonNode n);   // NULL if not string
int           leo_json_as_int(leo_JsonNode n);      // 0 if not number
double        leo_json_as_double(leo_JsonNode n);   // 0.0 if not number
bool          leo_json_as_bool(leo_JsonNode n);     // false if not bool

#ifdef __cplusplus
}
#endif
