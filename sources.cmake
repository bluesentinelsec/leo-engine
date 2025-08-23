set(LEO_SOURCES
        src/engine.c
        src/error.c
        src/graphics.c
        src/keyboard.c
        src/image.c
        external/miniaudio/miniaudio.c
        external/getopt/getopt.c
)

set(LEO_TEST_SOURCES
        tests/engine_test.cpp
        tests/error_test.cpp
        tests/graphics_test.cpp
        tests/keyboard_test.cpp
        tests/image_test.cpp
        tests/caller_testing.cpp
)
