#!/bin/bash

# Script to concatenate Leo pack header files into a single mega file
# Output file clearly separates each input file with a header

# Output file name
OUTPUT_FILE="leo_pack_mega.h"

# List of input header files
FILES=(
    "include/leo/pack_format.h"
    "include/leo/pack_writer.h"
    "include/leo/pack_reader.h"
    "include/leo/pack_compress.h"
    "include/leo/pack_obfuscate.h"
    "include/leo/pack_util.h"
    "src/pack_reader.c"
    "src/pack_writer.c"
    "src/pack_obfuscate.c"
    "src/pack_util.c"
    "src/pack_compress.c"
    "sources.cmake"
    "CMakeLists.txt"
    "tests/pack_rw_test.cpp"


)

# Initialize or clear the output file
> "$OUTPUT_FILE"

# Function to append a file with a separator header
append_file() {
    local file="$1"
    if [ ! -f "$file" ]; then
        echo "Error: File $file not found" >&2
        return 1
    fi
    # Add separator and file header
    echo "/* ==========================================================" >> "$OUTPUT_FILE"
    echo " * File: $file" >> "$OUTPUT_FILE"
    echo " * ========================================================== */" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
    # Append the file contents
    cat "$file" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
    echo "" >> "$OUTPUT_FILE"
}

# Iterate through the files and append each to the output
for file in "${FILES[@]}"; do
    append_file "$file"
    if [ $? -ne 0 ]; then
        echo "Failed to process $file, continuing with next files" >&2
    fi
done

echo "Mega file created: $OUTPUT_FILE"
