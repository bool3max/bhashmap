#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct BHashMap BHashMap;

BHashMap *
bhm_create(const size_t capacity);

bool
bhm_set(BHashMap *map, const void *key, const size_t keylen, const void *data); 

void *
bhm_get(BHashMap *map, const void *key, const size_t keylen); 

void
bhm_destroy(BHashMap *map);

void
bhm_print_debug_stats(const BHashMap *map, FILE *stream);
