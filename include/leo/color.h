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

#ifdef __cplusplus
}
#endif
