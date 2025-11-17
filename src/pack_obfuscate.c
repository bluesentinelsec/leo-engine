#include "leo/pack_obfuscate.h"
#include "leo/pack_util.h"
#include <stddef.h>
#include <stdint.h>
#include <SDL3/SDL_stdinc.h>

/* Derive a 32-bit stream seed from password and pack_salt. */
uint32_t leo_xor_seed_from_password(const char *password, uint64_t pack_salt)
{
    if (!password)
        password = "";
    /* Mix salt in front and back to avoid trivial collisions */
    uint64_t parts[2];
    parts[0] = pack_salt;
    parts[1] = leo_fnv1a64(password, (size_t)SDL_strlen(password));
    uint64_t mix = leo_fnv1a64(parts, sizeof(parts));
    uint32_t seed = (uint32_t)(mix ^ (mix >> 32));
    if (seed == 0)
        seed = 0xA5A5A5A5u; /* avoid zero LCG */
    return seed;
}

/* Simple bytewise XOR stream based on an LCG keystream (deterministic, non-crypto). */
void leo_xor_stream_apply(uint32_t seed, void *data, size_t n)
{
    if (n == 0)
        return;
    if (seed == 0)
        return; /* nothing to do (treated as 'no password') */

    /* LCG parameters from Numerical Recipes */
    uint32_t x = seed;
    uint8_t *p = (uint8_t *)data;
    for (size_t i = 0; i < n; ++i)
    {
        x = x * 1664525u + 1013904223u;
        p[i] ^= (uint8_t)(x >> 24); /* use high byte for better bit dispersion */
    }
}
