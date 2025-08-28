#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        unsigned char r; // Red channel
        unsigned char g; // Green channel
        unsigned char b; // Blue channel
        unsigned char a; // Alpha channel (opacity: 0 = transparent, 255 = opaque)
    } leo_Color;

    static const leo_Color LEO_GRAY      = {130, 130, 130, 255};
    static const leo_Color LEO_YELLOW    = {253, 249, 0, 255};
    static const leo_Color LEO_ORANGE    = {255, 161, 0, 255};
    static const leo_Color LEO_PINK      = {255, 109, 194, 255};
    static const leo_Color LEO_RED       = {230, 41, 55, 255};
    static const leo_Color LEO_GREEN     = {0, 228, 48, 255};
    static const leo_Color LEO_BLUE      = {0, 121, 241, 255};
    static const leo_Color LEO_PURPLE    = {200, 122, 255, 255};
    static const leo_Color LEO_BROWN     = {127, 106, 79, 255};
    static const leo_Color LEO_WHITE     = {255, 255, 255, 255};
    static const leo_Color LEO_BLACK     = {0, 0, 0, 255};

#ifdef __cplusplus
}
#endif
