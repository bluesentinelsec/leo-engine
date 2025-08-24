Great—those are exactly the hot spots. Here’s a concrete integration plan (still design-only), tailored to the files you shared, plus what I’d add:

# The shape of the solution

## 1) Add a tiny VFS shim (new module)

Create a very small module that knows how to:

* optionally mount one `.leopack` archive (+ password)
* maintain a list of search directories
* read a logical path into memory by probing: **pack → dirs**
* (optional) expose stream-y helpers later

**New files**

* `include/leo/vfs.h`
* `src/vfs.c`

**Core API (minimal)**

```c
// vfs.h
typedef struct leo_VfsMount leo_VfsMount;

// Global “session” (simple, single pack is enough to start)
LEO_API bool  leo_vfs_mount_pack(const char* packPath, const char* password);  // replace/clear with NULL path
LEO_API void  leo_vfs_add_search_dir(const char* dir);                         // can be called many times
LEO_API void  leo_vfs_clear_search_dirs(void);

// Read-all helper (heap-owned buffer; caller frees with free()).
LEO_API bool  leo_vfs_read_all(const char* logical_path, void** out_data, size_t* out_size);

// Optional introspection (for diagnostics/tests)
LEO_API int   leo_vfs_file_exists(const char* logical_path);    // 1 if found in pack or any dir
LEO_API int   leo_vfs_is_from_pack(const char* logical_path);   // 1 if pack wins
```

**Behavior**

* Normalize `logical_path` to forward slashes, strip a leading `./`, mirror the writer’s normalization so names match inside the pack.
* First, try the mounted pack (if any) via `leo_pack_find + leo_pack_extract_index`.
* If not found, probe the search dirs in order (`dir/logical_path`) using normal filesystem I/O.
* Return a heap buffer (`malloc`) and size; callers free.

Threading: a single `static struct` + a small mutex around mount/search operations is enough (most engines load on the main thread). We can add locks later if you spin up background loaders.

## 2) Wire existing loaders to the VFS (touch these files)

### `src/image.c`

Current path:

* `leo_LoadTexture(fileName)` → `stbi_load(fileName,...)`
* `leo_LoadTextureFromMemory(...)` already exists (great!)

**Change**: In `leo_LoadTexture`, replace the direct `stbi_load(fileName,…)` with:

* `leo_vfs_read_all(fileName, &buf, &sz)` → then call existing `stbi_load_from_memory(buf, sz, …)`.
* Free `buf` after upload.

No other call sites need to change.

### `src/font.c`

Current path:

* `leo_LoadFont(fileName, pixelSize)` uses `fopen` to slurp the TTF.

**Change**: Replace the `fopen` block with:

* `leo_vfs_read_all(fileName, &buf, &sz)` → `_load_from_ttf_bytes(buf, (int)sz, pixelSize)`.

Everything else stays intact.

### `src/audio.c`

Current path:

* `leo_LoadSound(filePath)` uses `ma_sound_init_from_file(&g_engine, filePath, …)`.

**Change**: Use miniaudio’s in-memory path:

* Read the file through `leo_vfs_read_all(filePath, &buf, &sz)`.
* Initialize via **decoder-as-data-source** (pattern many people use):

  * `ma_decoder decoder; ma_decoder_init_memory(buf, sz, NULL, &decoder)`
  * `ma_sound* snd = malloc(sizeof(*snd));`
  * `ma_sound_init_from_data_source(&g_engine, &decoder, 0, NULL, snd)`
  * Keep `buf` around as long as the decoder needs it OR use `ma_decoder_init_memory_copy()` (if available in your version) so you can free `buf` right away.
  * When unloading: `ma_sound_uninit(snd); ma_decoder_uninit(&decoder); free(snd); free(buf_if_kept);`

If your current miniaudio version doesn’t have `ma_sound_init_from_data_source`, use `ma_decoder` + `ma_data_source` patterns supported by your version (or `ma_engine_play_sound` equivalents). We can pick the exact calls when we look at the miniaudio version you’ve vendored.

### (Optionally) central place to configure mounts early

You can expose simple app-level helpers in `src/engine.c` init path, e.g.:

* `leo_vfs_mount_pack(path, password)` called by the app after `leo_InitWindow()` (or before, doesn’t matter)
* `leo_vfs_add_search_dir("assets")` in your bootstrap

No engine-core coupling is required; keeping VFS as its own module is cleaner.

## 3) Search policy & path rules

* **Order**: Pack wins over directories (your stated preference).
* **Case-sensitivity**: Pack names are bytes; OS filesystem varies. Choose one:

  * Easiest: **case-sensitive** everywhere. Document it.
  * Optional later: case-insensitive lookups on Windows for dirs (build a lowercased cache when you add the dir).
* **Separators**: Normalize to `/` before lookups; writer already normalized.
* **Relative roots**: `leo_vfs_add_search_dir()` gives you control; typical is `"assets"` and maybe `"."`.
* **Security**: When probing dirs, reject `..` segments in logical paths (don’t allow escaping the asset roots).

## 4) Error mapping

* If pack lookup fails for an obfuscated entry due to **wrong password** (`LEO_PACK_E_BAD_PASSWORD`), I recommend:

  * **Do not** silently fall back to directories. Return a clear error to the loader (so app logs “bad password” instead of mysteriously loading the wrong asset from disk). This keeps your threat model consistent.
* If the entry is **absent** in pack, then fall back to dirs.

We can make this policy a small flag on mount if you want different behavior in dev vs prod.

## 5) Tests you’ll want (new)

* **Happy path**: With a mounted pack containing `foo.png`, `leo_LoadTexture("foo.png")` succeeds without touching disk.
* **Fallback**: With pack mounted but `bar.ttf` only on disk, `leo_LoadFont("bar.ttf")` succeeds via dir.
* **Wrong password**: Pack contains `boom.ogg` obfuscated; with wrong password, `leo_LoadSound("boom.ogg")` returns failure (no dir fallback).
* **No pack mounted**: All loaders use dirs and continue to pass.
* **Path normalization**: On Windows, `"textures\\hero.png"` works when the pack used `/` names.

## 6) CI/CD & runtime knobs

* Environment variables for convenience:

  * `LEO_VFS_PACK=/path/to/game.leopack`
  * `LEO_VFS_PASSWORD=…`
  * `LEO_VFS_DIR=assets;dlc` (semicolon/comma separated)
* Your game/bootstrap can call the explicit API instead; envs are just ergonomic.

---

# Files you shared vs. what I’ll touch

**Will change:**

* `src/image.c` (switch file → memory path)
* `src/font.c` (switch file → memory path)
* `src/audio.c` (switch file → memory path via miniaudio decoder/datasource)

**Will add:**

* `include/leo/vfs.h`
* `src/vfs.c`

**Won’t need to change:**

* `include/leo/*` you listed (audio/font/image headers) can stay the same API.
* `src/engine.c` can stay as-is unless you want to add convenience bootstrapping.

If you’re good with this, I can sketch the `vfs.h` surface + the minimal `vfs.c` logic and the exact call changes in each loader next.

