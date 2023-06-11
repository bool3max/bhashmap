#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct BHashMap BHashMap;
typedef void (*bhm_iterator_keys_callback)(const void *key, const size_t keylen);

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

void
bhm_iterate_keys(const BHashMap *map, bhm_iterator_keys_callback callback_function); 

size_t
bhm_count(const BHashMap *map); 