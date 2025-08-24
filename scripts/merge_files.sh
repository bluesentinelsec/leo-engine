#!/bin/bash

# Script to concatenate Leo pack header files into a single mega file
# Output file clearly separates each input file with a header

# Output file name
OUTPUT_FILE="leo_pack_mega.h"

# List of input header files
FILES=(
    src/io.c
    include/leo/io.h
    src/pack_reader.c
    include/leo/pack_reader.h
    src/pack_writer.c
    include/leo/pack_writer.h
    include/leo/pack_format.h
    src/pack_util.c
    include/leo/pack_util.h
    include/leo/pack_compress.h
    src/pack_compress.c
    include/leo/pack_obfuscate.h
    src/pack_obfuscate.c
    include/leo/error.h
    src/error.c
    CMakeLists.txt
    sources.cmake
    src/engine.c
    include/leo/engine.h
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
