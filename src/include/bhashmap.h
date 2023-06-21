#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct BHashMap BHashMap;
typedef void (*bhm_iterator_callback)(const void *key, const size_t keylen, void *value);
typedef uint32_t (*bhm_hash_function)(const void *data, size_t len);

typedef struct BHashMapConfig {
    bhm_hash_function hashfunc;
    double max_load_factor;
    size_t resize_growth_factor;
} BHashMapConfig;

BHashMap *
bhm_create(const size_t capacity, const BHashMapConfig *config_user);

bool
bhm_set(BHashMap *map, const void *key, const size_t keylen, const void *data); 

void *
bhm_get(const BHashMap *map, const void *key, const size_t keylen); 

bool 
bhm_remove(BHashMap *map, const void *key, const size_t keylen); 

void
bhm_destroy(BHashMap *map);

void
bhm_print_debug_stats(const BHashMap *map, FILE *stream);

void
bhm_iterate(const BHashMap *map, bhm_iterator_callback callback_function); 

size_t
bhm_count(const BHashMap *map); 

BHashMapConfig
bhm_get_config(const BHashMap *map);