// Implementation of xxHash by Nicholas Sullivan
//
// xxHash - Fast Hash algorithm
// Copyright (C) 2012-2014, Yann Collet.
// BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// You can contact the author at :
// - xxHash source repository : http://code.google.com/p/xxhash/

#include "hash.h"

#define force_inline static __attribute__((always_inline))

#define hash_aligned 1
#define hash_unaligned 0

#define PRIME64_1 11400714785074694791ULL
#define PRIME64_2 14029467366897019727ULL
#define PRIME64_3 1609587929392839161ULL
#define PRIME64_4 9650029242287828579ULL
#define PRIME64_5 2870177450012600261ULL

#define hash_get64bits(x) hash_read64_align(x, align)
#define hash_get32bits(x) hash_read32_align(x, align)
#define hash_rotl64(x, r) ((x << r) | (x >> (64 - r)))
#define A64(x) (((U64_S *)(x))->v)
#define A32(x) (((U32_S *)(x))->v)

typedef struct _U64_S
{
    uint64_t v;
} U64_S;

typedef struct _U32_S
{
    uint32_t v;
} U32_S;

force_inline uint64_t
hash_read64_align(const void *ptr, uint32_t align)
{
    if (align == hash_unaligned)
    {
        return A64(ptr);
    }
    return *(uint64_t *)ptr;
}

force_inline uint32_t
hash_read32_align(const void *ptr, uint32_t align)
{
    if (align == hash_unaligned)
    {
        return A32(ptr);
    }
    return *(uint32_t *)ptr;
}

force_inline uint64_t
hash_endian_align(const void *input, uint64_t len, uint64_t seed, uint32_t align)
{
    const uint8_t *p = (const uint8_t *)input;
    const uint8_t *bEnd = p + len;
    uint64_t h64;

    if (len >= 32)
    {
        const uint8_t *const limit = bEnd - 32;
        uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
        uint64_t v2 = seed + PRIME64_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - PRIME64_1;

        do
        {
            v1 += hash_get64bits(p) * PRIME64_2;
            p += 8;
            v1 = hash_rotl64(v1, 31);
            v1 *= PRIME64_1;
            v2 += hash_get64bits(p) * PRIME64_2;
            p += 8;
            v2 = hash_rotl64(v2, 31);
            v2 *= PRIME64_1;
            v3 += hash_get64bits(p) * PRIME64_2;
            p += 8;
            v3 = hash_rotl64(v3, 31);
            v3 *= PRIME64_1;
            v4 += hash_get64bits(p) * PRIME64_2;
            p += 8;
            v4 = hash_rotl64(v4, 31);
            v4 *= PRIME64_1;
        } while (p <= limit);

        h64 = hash_rotl64(v1, 1) + hash_rotl64(v2, 7) + hash_rotl64(v3, 12) + hash_rotl64(v4, 18);

        v1 *= PRIME64_2;
        v1 = hash_rotl64(v1, 31);
        v1 *= PRIME64_1;
        h64 ^= v1;
        h64 = h64 * PRIME64_1 + PRIME64_4;

        v2 *= PRIME64_2;
        v2 = hash_rotl64(v2, 31);
        v2 *= PRIME64_1;
        h64 ^= v2;
        h64 = h64 * PRIME64_1 + PRIME64_4;

        v3 *= PRIME64_2;
        v3 = hash_rotl64(v3, 31);
        v3 *= PRIME64_1;
        h64 ^= v3;
        h64 = h64 * PRIME64_1 + PRIME64_4;

        v4 *= PRIME64_2;
        v4 = hash_rotl64(v4, 31);
        v4 *= PRIME64_1;
        h64 ^= v4;
        h64 = h64 * PRIME64_1 + PRIME64_4;
    }
    else
    {
        h64 = seed + PRIME64_5;
    }

    h64 += (uint64_t)len;

    while (p + 8 <= bEnd)
    {
        uint64_t k1 = hash_get64bits(p);
        k1 *= PRIME64_2;
        k1 = hash_rotl64(k1, 31);
        k1 *= PRIME64_1;
        h64 ^= k1;
        h64 = hash_rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
        p += 8;
    }

    if (p + 4 <= bEnd)
    {
        h64 ^= (uint64_t)(hash_get32bits(p)) * PRIME64_1;
        h64 = hash_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p += 4;
    }

    while (p < bEnd)
    {
        h64 ^= (*p) * PRIME64_5;
        h64 = hash_rotl64(h64, 11) * PRIME64_1;
        p++;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;

    return h64;
}

uint64_t
hash(const void *input, uint64_t len, uint64_t seed)
{
    if ((((uint64_t)input) & 7) == 0)
    {
        return hash_endian_align(input, len, seed, hash_aligned);
    }
    return hash_endian_align(input, len, seed, hash_unaligned);
}
