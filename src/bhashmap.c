/*
BHashMap is a generic Hash Table implementation written in C11. It was initially written with the primary goal of learning the ins-and-outs
of the hash table data structure, however it has since evolved to be a very solid, usable implementation.

The goal is to provide performant, general hash table implementation with a pleasant-to-use API that allows for keys and values of arbitrary types
and sizes. The goal is NOT to provide a specific hyper-optimized implementation for any one specific use case.

If the macro BHM_DEBUG is defined, the functions throughout the library print various
debug messages and info to stderr.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

#include "bhashmap.h"

/*
The DEBUG_PRINT macro only expands if BHM_DEBUG is defined. Otherwise, it expands to nothing
and as such no print is performed.
*/
#ifdef BHM_DEBUG
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, __func__ ": " fmt, __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

#define HASH(dataptr, len) murmur3_32(dataptr, len, 1u)

typedef struct HashPair {
    void *key, *value;
    struct HashPair *next;
} HashPair;

struct BHashMap {
    size_t capacity,
           pair_count;

    HashPair *table;
};

// Murmurhash3 implementation
uint32_t
murmur3_32(const char *key, uint32_t len, uint32_t seed) {
    static const uint32_t c1 = 0xcc9e2d51;
    static const uint32_t c2 = 0x1b873593;
    static const uint32_t r1 = 15;
    static const uint32_t r2 = 13;
    static const uint32_t m = 5;
    static const uint32_t n = 0xe6546b64;

    uint32_t hash = seed;

    const int nblocks = len / 4;
    const uint32_t *blocks = (const uint32_t *) key;
    int i;
    for (i = 0; i < nblocks; i++) {
        uint32_t k = blocks[i];
        k *= c1;
        k = (k << r1) | (k >> (32 - r1));
        k *= c2;

        hash ^= k;
        hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
    }

    const uint8_t *tail = (const uint8_t *) (key + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3) {
    case 3:
        k1 ^= tail[2] << 16;
    case 2:
        k1 ^= tail[1] << 8;
    case 1:
        k1 ^= tail[0];

        k1 *= c1;
        k1 = (k1 << r1) | (k1 >> (32 - r1));
        k1 *= c2;
        hash ^= k1;
    }

    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);

    return hash;
}

/*
Create a new BHashMap with a given capacity.
RETURN VALUE:
    On success, return a pointer to the new BHashMap.
    On failure, return NULL.
*/
BHashMap *
bhm_create(const size_t capacity) {
    BHashMap *new_map = malloc(sizeof(BHashMap));

    if (!new_map) {
        return NULL;
    }

    *new_map = (BHashMap) {
        .capacity = capacity,
        .pair_count = 0,
        .table = calloc(capacity, sizeof(HashPair))
    };

    if (!new_map->table) {
        free(new_map);
        return NULL;
    }

    DEBUG_PRINT("Created hash map with capacity %lu.\n", capacity);

    return new_map;
}