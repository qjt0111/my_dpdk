// xxHash Implementation

#include <stdint.h>

#ifndef HASH_H
#define HASH_H

uint64_t
hash(const void *input, uint64_t len, uint64_t seed);

#endif
