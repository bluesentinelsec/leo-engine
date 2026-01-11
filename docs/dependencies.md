# Leo Engine Dependencies

This document lists all third-party dependencies used in the Leo Engine project.

## Dependencies via CMake FetchContent

### SDL3
- **Version**: 3.4.0
- **URL**: https://github.com/libsdl-org/SDL
- **License**: Zlib License
- **Purpose**: Core library for windowing, input, audio, timing, and rendering

### Catch2
- **Version**: 3.12.0
- **URL**: https://github.com/catchorg/Catch2
- **License**: Boost Software License 1.0
- **Purpose**: Unit testing framework

### tmxlite
- **Version**: 1.4.4
- **URL**: https://github.com/fallahn/tmxlite
- **License**: Zlib License
- **Purpose**: Tiled map loader for parsing TMX/JSON map files

### Lua
- **Version**: 5.4.4
- **URL**: https://github.com/marovira/lua
- **License**: MIT License
- **Purpose**: Scripting language for game logic (planned)

### PhysFS
- **Version**: commit 5ee045551a5d3404822f8ea08486b74d1b283858
- **URL**: https://github.com/icculus/physfs
- **License**: Zlib License
- **Purpose**: Abstract file system access for archives and directories

### miniaudio
- **Version**: 0.11.23
- **URL**: https://github.com/mackron/miniaudio
- **License**: MIT License or Public Domain (dual-licensed)
- **Purpose**: Audio playback and capture library

## Dependencies in external/

### CLI11
- **Version**: 2.6.1
- **URL**: https://github.com/CLIUtils/CLI11
- **License**: BSD 3-Clause License
- **Purpose**: Command-line argument parsing

### STB Libraries
- **Version**: 10jan2026 snapshot
- **URL**: https://github.com/nothings/stb
- **License**: MIT License or Public Domain (dual-licensed)
- **Purpose**: Header-only libraries for image loading (stb_image.h) and font rendering (stb_truetype.h)

## Memory Allocation

All dependencies are configured to use SDL3's memory allocation functions (`SDL_malloc`, `SDL_realloc`, `SDL_free`) to support custom allocators via `SDL_SetMemoryFunctions()`.

## Build Configuration

All dependencies are statically linked into the Leo Engine executable for simplified deployment and predictable runtime behavior across platforms.
