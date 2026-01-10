#!/usr/bin/env bash
set -euo pipefail

# ==========================================================
# Merge all sources into a single document for AI copy/paste
# - Scans:
#     include/leo/*.h
#     src/*.c
#     tests/*.cpp
# - Writes separators with file paths
# - Stable order: headers -> sources -> tests (each alpha)
#
# Usage: ./merge_all_sources.sh [OUTPUT_FILE]
# Default OUTPUT_FILE: merged_sources.txt
# ==========================================================

OUTPUT_FILE="${1:-merged_sources.txt}"

# Create/clear output
: > "$OUTPUT_FILE"

# --- Helpers ---------------------------------------------------------------

# Append one file with a nice separator (path-safe)
append_file() {
  local file="$1"
  if [[ ! -f "$file" ]]; then
    echo "warn: missing file: $file" >&2
    return 0
  fi
  {
    echo "/* =========================================================="
    echo " * File: $file"
    echo " * ========================================================== */"
    echo
    cat "$file"
    echo
    echo
  } >> "$OUTPUT_FILE"
}

# Collect files via find (non-recursive) and output NUL-delimited list
collect_files_nul() {
  local dir="$1"
  local pattern="$2"
  if [[ -d "$dir" ]]; then
    # Non-recursive (-maxdepth 1) to match the user's request
    find "$dir" -maxdepth 1 -type f -name "$pattern" -print0
  fi
}

# Print a NUL-delimited list in sorted order (prefers sort -z; falls back safely)
sort_nul() {
  # If sort supports -z, use it (best for spaces/newlines in names)
  if sort -z </dev/null >/dev/null 2>&1; then
    sort -z
    return
  fi
  # Fallback: convert NULs to newlines, sort, convert back.
  # NOTE: This fallback is not newline-safe in filenames, which is rare in codebases.
  tr '\0' '\n' | LC_ALL=C sort | tr '\n' '\0'
}

# Append a NUL-delimited list of paths
append_nul_list() {
  local tmplist
  tmplist="$(mktemp)"
  # Save and iterate robustly over NUL-separated entries
  cat > "$tmplist"
  while IFS= read -r -d '' f; do
    append_file "$f"
  done < "$tmplist"
  rm -f "$tmplist"
}

# --- Header ---------------------------------------------------------------

{
  echo "/*"
  echo " * =========================================================="
  echo " *  Leo Engine — merged sources"
  echo " *  Generated: $(date -u +"%Y-%m-%d %H:%M:%S UTC")"
  echo " *  Includes:"
  echo " *    - include/leo/*.h"
  echo " *    - src/*.c"
  echo " *    - tests/*.cpp"
  echo " * =========================================================="
  echo " */"
  echo
  echo
} >> "$OUTPUT_FILE"

# --- Collect & append in groups -------------------------------------------

# 1) Headers
collect_files_nul "include/leo" "*.h" | sort_nul | append_nul_list

# 2) C sources
# collect_files_nul "src" "*.c" | sort_nul | append_nul_list

# 3) C++ tests
# collect_files_nul "tests" "*.cpp" | sort_nul | append_nul_list

echo "Mega file created: $OUTPUT_FILE"

