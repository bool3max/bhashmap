/*
BHashMap is a generic Hash Table implementation written in C11. It was initially written with the primary goal of learning the ins-and-outs
of the hash table data structure, however it has since evolved to be a very solid, usable implementation.

The goal is to provide a solid, general hash table implementation with a pleasant-to-use API that allows for keys and values of arbitrary types
and sizes. The goal is NOT to provide a specific hyper-optimized implementation for any one specific use case.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

#include "include/bhashmap.h"

typedef struct HashPair {
    void *key, *value;
    struct HashPair *next;
} HashPair;

struct BHashMap {
    size_t capacity,
           paircount;

    HashPair *table;
};

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
        .paircount = 0,
        .table = calloc(capacity, sizeof(HashPair))
    };

    if (!new_map->table) {
        free(new_map);
        return NULL;
    }

    return new_map;
}