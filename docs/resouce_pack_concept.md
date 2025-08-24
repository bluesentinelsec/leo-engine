Awesome—this is a great feature to add. Here’s a clean, C-friendly plan that keeps the core simple, leaves room to evolve, and gives you testable primitives.

---

# File format (v1, little-endian)

```
PACK FILE
┌──────────────────────────────────────────────────────────────────┐
│ 0x00: magic "LEOPACK\0" (8)                                      │
│ 0x08: u32  version = 1                                           │
│ 0x0C: u32  pack_flags (bit 0: obfuscated)                        │
│ 0x10: u64  toc_offset (absolute)                                  │
│ 0x18: u64  toc_size   (bytes)                                     │
│ 0x20: u64  data_offset (start of first payload; aligned)          │
│ 0x28: u64  pack_salt (for XOR keystream derivation)               │
│ 0x30: u32  reserved[8] (future)                                   │
│ 0x50: u32  header_crc32 (over bytes [0x00 .. 0x4F])               │
└──────────────────────────────────────────────────────────────────┘

TOC (packed as a simple array of entries):
┌──────────────────────────────────────────────────────────────────┐
│ u32 entry_count                                                  │
│ For each entry i in [0..N):                                      │
│   u16 name_len                                                   │
│   char name[name_len] (UTF-8, no NUL, forward-slash separators)  │
│   u16 flags   (bit0: compressed, bit1: obfuscated)               │
│   u64 offset  (absolute file payload start)                      │
│   u64 size_uncompressed                                          │
│   u64 size_stored  (== size_uncompressed if not compressed)      │
│   u32 crc32_uncompressed                                         │
└──────────────────────────────────────────────────────────────────┘

DATA AREA
- Concatenated payloads (optionally compressed, then optionally XOR’d).
- Each payload aligned (e.g., 16 bytes) for mmap-friendliness.
```

Notes

* **Compression** is per-entry; you decide per file (e.g., only if it helps, or by suffix).
* **Obfuscation** is per-entry too; TOC reflects this even if the pack’s global flag is set.
* **XOR** keystream = xorshift32 seeded from `fnv1a_64(password) ^ pack_salt`.
* **CRC32** lets you catch corruption after decompression & deobfuscation.
* Use little-endian everywhere; store version so you can change later.

---

# Division of responsibilities

* **Writer**: walks a directory, decides compress/obfuscate per file, writes data & TOC.
* **Reader**: opens from file handle or memory; lists, locates, and extracts entries (optionally streaming).
* **Compression adapter**: thin wrappers around `sdefl.h`/`sinfl.h`.
* **Obfuscation**: trivial XOR keystream (deterrent, not security).
* **Format**: structs, constants, and serialization helpers.
* **Errors**: small, stable enum used by all modules.

---

# Proposed headers (public surface)

You asked for “a list of .h files” — here’s a tidy set with what each exposes.

### `leo/pack_format.h`

* **Purpose:** On-disk format constants and PODs used by reader/writer; no I/O here.
* **Contents:**

  * `#define LEO_PACK_MAGIC "LEOPACK\0"`
  * `enum leo_pack_version { LEO_PACK_V1 = 1 };`
  * `enum leo_pack_flags { LEO_PACK_FLAG_OBFUSCATED = 1<<0 };`
  * `enum leo_pack_entry_flags { LEO_PE_COMPRESSED=1<<0, LEO_PE_OBFUSCATED=1<<1 };`
  * `typedef struct leo_pack_header_v1 { ... } leo_pack_header_v1;`
  * `typedef struct leo_pack_entry_v1 { ... } leo_pack_entry_v1;` *(without name buffer; name serialized separately)*
  * Helper inline functions for endian write/read (no-op on LE), CRC32 prototype.

### `leo/pack_errors.h`

* **Purpose:** Shared error codes across pack modules.
* **Contents:**

  * `typedef enum leo_pack_result {`

    * `LEO_PACK_OK = 0,`
    * `LEO_PACK_E_IO, LEO_PACK_E_FORMAT, LEO_PACK_E_VERSION,`
    * `LEO_PACK_E_MEM, LEO_PACK_E_CRC, LEO_PACK_E_NOTFOUND,`
    * `LEO_PACK_E_COMPRESS, LEO_PACK_E_DECOMPRESS,`
    * `LEO_PACK_E_OBFUSCATE, LEO_PACK_E_INVALID_ARG`
  * `} leo_pack_result;`
  * `const char* leo_pack_strerror(leo_pack_result);`

### `leo/pack_compress.h`

* **Purpose:** Thin adapter over `sdefl`/`sinfl`.
* **Contents:**

  * `typedef struct { int level; } leo_deflate_opts;` *(0..9 or simple presets)*
  * `leo_pack_result leo_compress_deflate(const void* in, size_t in_sz, void* out, size_t* out_sz, const leo_deflate_opts*);`
  * `leo_pack_result leo_decompress_deflate(const void* in, size_t in_sz, void* out, size_t* out_sz);`
  * Compile-time switches to stub out compression if not available.

### `leo/pack_obfuscate.h`

* **Purpose:** Simple per-byte XOR keystream.
* **Contents:**

  * `uint64_t leo_xor_seed_from_password(const char* password, uint64_t pack_salt);`
  * `void leo_xor_stream_apply(uint32_t seed, uint8_t* data, size_t n);`

    * Internally uses xorshift32 to generate a byte stream; XOR in place.

### `leo/pack_writer.h`

* **Purpose:** Build pack files from a directory or programmatically.
* **Contents:**

  * `typedef struct {`

    * `const char* root_dir;`      // walk files here
    * `const char* strip_prefix;`  // remove this path prefix from entry names
    * `int compress;`              // 0/1 (or a function callback)
    * `int obfuscate;`             // 0/1 (or a function callback)
    * `const char* password;`      // required if obfuscate
    * `int level;`                 // compression level
    * `size_t align;`              // payload alignment (e.g., 16)
    * `int (*should_compress)(const char* relpath, uint64_t size);`
    * `int (*should_obfuscate)(const char* relpath);`
  * `} leo_pack_build_opts;`
  * `leo_pack_result leo_pack_build_file(const char* out_path, const leo_pack_build_opts*);`
  * Programmatic API:

    * `typedef struct leo_pack_writer leo_pack_writer;`
    * `leo_pack_result leo_pack_writer_begin(leo_pack_writer**, FILE* out, const leo_pack_build_opts*);`
    * `leo_pack_result leo_pack_writer_add_file(leo_pack_writer*, const char* logical_name, const void* data, size_t size, int compress, int obfuscate);`
    * `leo_pack_result leo_pack_writer_add_path(leo_pack_writer*, const char* abs_or_rel_path, const char* logical_name);`
    * `leo_pack_result leo_pack_writer_end(leo_pack_writer*);`

### `leo/pack_reader.h`

* **Purpose:** Open, enumerate, and extract entries (to memory or via callback).
* **Contents:**

  * `typedef struct leo_pack leo_pack;`
  * `typedef struct { const char* name; uint16_t flags; uint64_t size_uncompressed; uint64_t size_stored; } leo_pack_stat;`
  * Open/close:

    * `leo_pack_result leo_pack_open_file(leo_pack**, const char* path, const char* password);`
    * `leo_pack_result leo_pack_open_memory(leo_pack**, const void* data, size_t size, const char* password);`
    * `void leo_pack_close(leo_pack*);`
  * Lookup & list:

    * `int leo_pack_count(leo_pack*);`
    * `leo_pack_result leo_pack_stat_index(leo_pack*, int index, leo_pack_stat* out);`
    * `leo_pack_result leo_pack_find(leo_pack*, const char* name, int* out_index);`
  * Extraction:

    * `leo_pack_result leo_pack_extract_index(leo_pack*, int index, void* dst, size_t dst_cap, size_t* out_size);`  // full into buffer
    * `typedef leo_pack_result (*leo_pack_write_cb)(void* user, const void* chunk, size_t n);`
    * `leo_pack_result leo_pack_extract_stream(leo_pack*, int index, leo_pack_write_cb, void* user);`               // streaming; deobfuscate → decompress → callback
  * Convenience:

    * `leo_pack_result leo_pack_read_all(leo_pack*, const char* name, void** out_ptr, size_t* out_size);` // mallocs

### (Optional) `leo/pack_util.h`

* **Purpose:** Shared helpers used by both sides and useful for tests.
* **Contents:** FNV-1a (32/64), CRC32 (IEEE), path normalization, tiny vec.

---

# Implementation sketch (how it fits together)

**Writer pipeline per file**

```
read file bytes → maybe compress (sdefl) → maybe XOR (with seed) → write payload (aligned)
record entry {name, flags, offset, size_uncompressed, size_stored, crc32_uncompressed}
```

After all files:

```
serialize TOC → write header (with toc offsets, global flags, salt, crc) → seek fixups done
```

**Reader pipeline per entry**

```
locate entry via TOC →
  open a view/seek to payload →
  read stored bytes → if obfuscated: XOR → if compressed: sinfl →
  (optionally stream to callback) → verify CRC32(uncompressed)
```

**Obfuscation**

* Derive seed: `seed32 = (uint32_t)(fnv1a_64(password) ^ pack_salt);`
* `xorshift32` keystream: `x ^= x<<13; x ^= x>>17; x ^= x<<5;`
* Use low 8 bits per byte; repeat.

**Compression**

* Use `sdefl` for deflate; `sinfl` for inflate.
* Allow `should_compress()` to skip tiny/incompressible files (e.g., already compressed assets).

**Alignment**

* Default `align=16`. Store absolute `offset` so you can change alignment later without recalculating relative bases.

**Safety**

* Validate magic/version; bounds-check TOC and entries; verify CRC32 after full decode; reject negative/overflow math.
* Fail closed if password is missing but any entry or pack is marked obfuscated.

---

# Minimal test plan

* Build: a directory with a few text + PNG files; mix compressed/obfuscated permutations using callbacks.
* Read: open by path & memory; list, find, extract to buffer and stream; verify payload and CRCs.
* Edge cases: 0-byte file, very small files (no compression), long nested names, wrong password, corrupted CRC.

---

If you like this breakdown, I can generate skeletons for each header with doc comments and no implementation yet (so you can land headers, then fill .c files next), or go straight to working `.c` implementations for the writer/reader using stdio.

