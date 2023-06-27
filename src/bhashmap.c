#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

#include "bhashmap.h"
#include "murmurhash3.h"
#include "benchmark.h"

#define BHM_DEFAULT_INITIAL_CAPCACITY 32
#define BHM_DEFAULT_MAX_LOAD_FACTOR 0.75
#define BHM_DEFAULT_RESIZE_GROWTH_FACTOR 2

/*
The DEBUG_PRINT macro only expands if BHM_DEBUG is defined. Otherwise, it expands to nothing
and as such no print is performed.
*/
#ifdef BHM_DEBUG
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, "\e[1;93m%s\e[0m: \e[4m" fmt "\e[0m", __func__, __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

typedef struct HashPair {
    size_t keylen;
    const void *value;
    struct HashPair *next;
    unsigned char key[];
} HashPair;

struct BHashMap {
    BHashMapConfig config;

    size_t capacity,
           pair_count;

    HashPair **buckets;

    #ifdef BHM_DEBUG_BENCHMARK
    struct _debugBenchmarkTimes {
        size_t bhm_resize_total_ms,
               bhm_set_total_ms,
               bhm_get_total_ms;
    } debug_benchmark_times;
    #endif
};

static uint32_t
murmur3_32_wrapper(const void *data, size_t len) {
    return murmur3_32(data, len, 1u);
}

static const BHashMapConfig DEFAULT_HASHMAP_CONFIG = (BHashMapConfig) {
    .hashfunc = murmur3_32_wrapper,
    .max_load_factor = BHM_DEFAULT_MAX_LOAD_FACTOR ,
    .resize_growth_factor = BHM_DEFAULT_RESIZE_GROWTH_FACTOR
};

static void
free_buckets(HashPair **buckets, const size_t bucket_count);

static inline HashPair *
create_pair(const size_t keylen); 

static inline double
get_load_factor(const BHashMap *map) {
    return (double) map->pair_count / (double) map->capacity;
}

/*
Calculate and print various statistics to specified stream.
*/
void
bhm_print_debug_stats(const BHashMap *map, FILE *stream) {
    size_t empty_bucket_count = 0,
           overflow_bucket_count = 0; // buckets with more than one element in ll

    for (size_t i = 0; i < map->capacity; i++) {
        if (map->buckets[i] == NULL) {
            empty_bucket_count += 1;
            continue; // an empty bucket cannot be a chain
        }

        if (map->buckets[i]->next != NULL) {
            overflow_bucket_count += 1;
        }
    }

    fprintf(stream, "\e[1;93mcapacity (buckets): %lu\n", map->capacity);
    fprintf(stream, "\e[1;93mitems (pairs): %lu\n", map->pair_count);
    fprintf(stream, "\e[1;93mempty buckets: %lu\n", empty_bucket_count);
    fprintf(stream, "\e[1;93moverflown buckets: %lu\n", overflow_bucket_count);
    fprintf(stream, "\e[1;93mload factor: %.3lf\n", get_load_factor(map));
}


/*
Create a new BHashMap with a given initial capacity.
RETURN VALUE:
    On success, return a pointer to the new BHashMap.
    On failure, return NULL.
*/
BHashMap *
bhm_create(const size_t initial_capacity, const BHashMapConfig *config_user) {
    BHashMap *new_map = malloc(sizeof(BHashMap));

    if (!new_map) {
        return NULL;
    }

    const size_t capacity = initial_capacity != 0 ? initial_capacity : BHM_DEFAULT_INITIAL_CAPCACITY;

    *new_map = (BHashMap) {
        .capacity = capacity,
        .pair_count = 0,
        .buckets = calloc(capacity, sizeof(HashPair *)),
        #ifdef BHM_DEBUG_BENCHMARK
        .debug_benchmark_times = (struct _debugBenchmarkTimes) {
            0, 0, 0
        }
        #endif
    };

    if (config_user == NULL) {
        new_map->config = DEFAULT_HASHMAP_CONFIG;
    } else {
        new_map->config = (BHashMapConfig) {
            .hashfunc = config_user->hashfunc != NULL ? config_user->hashfunc : murmur3_32_wrapper,
            .max_load_factor = config_user->max_load_factor > 0 ? config_user->max_load_factor : BHM_DEFAULT_MAX_LOAD_FACTOR,
            .resize_growth_factor = config_user->resize_growth_factor > 0 ? config_user->resize_growth_factor : BHM_DEFAULT_RESIZE_GROWTH_FACTOR
        };
    }

    if (!new_map->buckets) {
        free(new_map);
        return NULL;
    }

    DEBUG_PRINT("\e[93;1mbhm_create\e[0m: created hash map with capacity %lu.\n", initial_capacity);

    return new_map;
}

/*
Find and return the appropriate bucket for a given key,
based on its hash and the capacity of the hashmap.
*/
static inline HashPair **
find_bucket(const BHashMap *map, const void *key, const size_t keylen) {
    const uint32_t hash       = map->config.hashfunc(key, keylen),
                   bucket_idx = hash % map->capacity;

    DEBUG_PRINT("KEY: '%s', BUCKET IDX: %u\n", (char *) key, bucket_idx);

    return &map->buckets[bucket_idx];
}

/*
Insert a key-value pair into a HashPair structure.
*/
static inline void
insert_pair(HashPair *pair, const void *key, const size_t keylen, const void *data) {
    memcpy(pair->key, key, keylen);
    pair->value = data;
}

/* 
Return a pointer to a new zeroed-out HashPair struct allocated on the heap, or NULL on failure.
*/
static inline HashPair *
create_pair(const size_t keylen) {
    HashPair *new = malloc(sizeof(HashPair) + keylen);
    if (!new) {
        return NULL;
    }

    *new = (struct HashPair) {
        .keylen = keylen,
        0
    };

    return new;
}

/*
Resize the given hash map by the constant resize factor.

On failure, the hash map is not resized and remains just as it was before the call.

RETURN VALUE:
    On success, true is returned.
    On failure, false is returned.
*/
static bool
resize(BHashMap *map) {
    #ifdef BHM_DEBUG_BENCHMARK
    double start_load_factor = get_load_factor(map);
    uint64_t bench_start_nanos = start_benchmark();
    #endif

    size_t capacity_new = map->capacity * map->config.resize_growth_factor,
           capacity_old = map->capacity;

    HashPair **buckets_new = calloc(capacity_new, sizeof(HashPair *)),
             **buckets_old = map->buckets;

    if (!buckets_new) {
        DEBUG_PRINT("\tresizing %lu -> %lu failed\n", capacity_old, capacity_new);
        return false;
    }

    map->buckets = buckets_new;
    map->capacity = capacity_new;

    /* rehash every key-value pair from the old table */

    for (size_t idx_old = 0; idx_old < capacity_old; idx_old++) {
        HashPair *head = buckets_old[idx_old];

        /* empty bucket slot */
        if (head == NULL) {
            continue;
        }

        /* head bucket pair exists */
        while (head) {
            /* find new bucket position for this pair*/
            HashPair **bucket_new = find_bucket(map, head->key, head->keylen);

            /* dest bucket slot empty */
            if (*bucket_new == NULL) {
                *bucket_new = head;

                HashPair *n = head->next;
                head->next = NULL;
                head = n;

                continue;
            }

            /* dest bucket slot filled, prepend current old head to beginning of chain*/
            
            HashPair *n = head->next;
            head->next = *bucket_new;
            *bucket_new = head;

            head = n;
        }
    }

    free(buckets_old);

    #ifdef BHM_DEBUG_BENCHMARK
    uint64_t time_elapsed = end_benchmark(bench_start_nanos);
    double end_load_factor = get_load_factor(map);
    fprintf(stderr, "\e[1;93mresize\e[0m \e[32m%6lu\e[0m -> \e[32m%7lu\e[0m, LF \e[32m%.3lf\e[0m -> \e[32m%.3lf\e[0m took %5lums.\n", capacity_old, capacity_new, start_load_factor, get_load_factor(map), time_elapsed);
    map->debug_benchmark_times.bhm_resize_total_ms += time_elapsed;
    #endif

    return true;
}

/*
Insert a new key-value pair into the hashmap, or update the associated value if the key already
exists in the hashmap.

"keylen" is the length of the key in bytes.

RETURN VALUE:
    On success, true is returned.
    On failure, false is returned.
*/
bool
bhm_set(BHashMap *map, const void *key, const size_t keylen, const void *data) {
    #ifdef BHM_DEBUG_BENCHMARK
    const uint64_t bench_start_nanos = start_benchmark();
    #endif

    HashPair **bucket = find_bucket(map, key, keylen);
    
    /* best case - no bucket at idx */
    if (*bucket == NULL) {
        *bucket = create_pair(keylen);
        if (!(*bucket)) {
            return false;
        }

        insert_pair(*bucket, key, keylen, data);

        map->pair_count += 1;

        #ifdef BHM_DEBUG_BENCHMARK
        uint64_t time_elapsed = end_benchmark(bench_start_nanos);
        map->debug_benchmark_times.bhm_set_total_ms += time_elapsed;
        #endif

        if (get_load_factor(map) >= map->config.max_load_factor) {
            resize(map);
        }

        return true;
    }

    /* bucket present at idx */

    HashPair *head = *bucket;

    while (head) {
        if (head->keylen == keylen && memcmp(key, head->key, keylen) == 0) {
            /* found the key already in the map - update its value */
            head->value = data;

            #ifdef BHM_DEBUG_BENCHMARK
            uint64_t time_elapsed = end_benchmark(bench_start_nanos);
            map->debug_benchmark_times.bhm_set_total_ms += time_elapsed;
            #endif

            return true;
        }

        if (head->next == NULL) {
            /* at end of linked list - allocate space for new pair and copy data over */
            HashPair *new_pair = create_pair(keylen);
            if (!new_pair) {
                return false;
            }

            insert_pair(new_pair, key, keylen, data);

            head->next = new_pair;

            map->pair_count += 1;

            #ifdef BHM_DEBUG_BENCHMARK
            uint64_t time_elapsed = end_benchmark(bench_start_nanos);
            map->debug_benchmark_times.bhm_set_total_ms += time_elapsed;
            #endif

            if (get_load_factor(map) >= map->config.max_load_factor) {
                resize(map);
            }

            return true;
        }

        head = head->next;
    }

    #ifdef BHM_DEBUG_BENCHMARK
    uint64_t time_elapsed = end_benchmark(bench_start_nanos);
    map->debug_benchmark_times.bhm_set_total_ms += time_elapsed;
    #endif

    return false; // ERROR ?
}

/*
Get the value of a key from the map.
RETURN VALUE:
    NULL     - key not found / error
    NON-NULL - appropriate data
*/
void *
bhm_get(const BHashMap *map, const void *key, const size_t keylen) {
    #ifdef BHM_DEBUG_BENCHMARK
    uint64_t bench_start_nanos = start_benchmark();
    #endif

    HashPair **bucket = find_bucket(map, key, keylen);

    /* empty bucket, key definitely not in map */
    if (bucket == NULL) {
        return NULL;
    }

    HashPair *head = *bucket;

    while (head) {
        if (head->keylen == keylen && memcmp(key, head->key, keylen) == 0) {
            return (void *) head->value;
        }

        head = head->next;
    }

    #ifdef BHM_DEBUG_BENCHMARK
    uint64_t time_elapsed = end_benchmark(bench_start_nanos);
    ((BHashMap*) map)->debug_benchmark_times.bhm_get_total_ms += time_elapsed;
    #endif

    return NULL;
}

/* remove a key from the hash map */
bool
bhm_remove(BHashMap *map, const void *key, const size_t keylen) {
    HashPair **bucket = find_bucket(map, key, keylen);

    /* head bucket empty - key definitely not in map */
    if (bucket == NULL) {
        return false;
    }

    /* head bucket has at least one pair */
    HashPair *head = *bucket;

    /* if it's the ONLY pair and it's the right key */
    if (head->next == NULL && head->keylen == keylen && memcmp(head->key, key, keylen) == 0) {
        free(head);
        *bucket = NULL;
        return true;
    }

    while (head && head->next) {
        HashPair *next = head->next;

        if (next->keylen == keylen && memcmp(key, next->key, keylen) == 0) {
            HashPair *nn = next->next;
            free(next);
            head->next = nn;

            return true;
        }

        head = next;
    }

    return false;
}

static void
free_buckets(HashPair **buckets, const size_t bucket_count) {
    for (size_t i = 0; i < bucket_count; i++)  {
        HashPair *head = buckets[i];

        while (head) {
            HashPair *n = head->next;

            free(head);

            head = n;
        }
    }

    free(buckets);
}

/*
For each pair in the map, call the passed in callback function, passing in a pointer to the key,
the length of the key, and a pointer to the value.
*/
void
bhm_iterate(const BHashMap *map, bhm_iterator_callback callback_function) {
    for (size_t i = 0; i < map->capacity; i++) {
        HashPair *head = map->buckets[i];

        if (head == NULL) {
            continue;
        }

        while (head) {
            callback_function(head->key, head->keylen, (void *) head->value);
            head = head->next;
        }
    }
}

/*
Return the count of key-value pairs in the hash map.
*/
size_t
bhm_count(const BHashMap *map) {
    return map->pair_count;
}

/* Return the configuration structure currently used by the specified BHashMap instance. 
   NOTE: This functions returns a copy of the configuration strucutre, _not_ a pointer to it. 
*/
BHashMapConfig
bhm_get_config(const BHashMap *map) {
    return map->config;
}

/*
Free all resources occupied by the hash map. This includes the memory of the main BHashMap
structure, the memory for all the hash pairs in the structure (those at the 'root' as well as 
those in the linked lists), as well as memory for all the copied keys.
*/
void
bhm_destroy(BHashMap *map) {
    #ifdef BHM_DEBUG_BENCHMARK
    uint64_t bench_start_nanos = start_benchmark();
    #endif

    free_buckets(map->buckets, map->capacity);

    #ifdef BHM_DEBUG_BENCHMARK
    uint64_t time_elapsed = end_benchmark(bench_start_nanos);
    fprintf(
        stderr,
        "\e[1;93mbhm_destroy:\e[0m total time spent in functions of this instance:\n"
        "\t\e[1;93mbhm_resize\e[0m:\e[92m%lums\e[0m\n"
        "\t\e[1;93mbhm_get\e[0m:\e[92m%lums\e[0m\n"
        "\t\e[1;93mbhm_set\e[0m:\e[92m%lums\e[0m\n"
        "\t\e[1;93mbhm_destroy\e[0m:\e[92m%lums\e[0m\n",
        map->debug_benchmark_times.bhm_resize_total_ms,
        map->debug_benchmark_times.bhm_get_total_ms,
        map->debug_benchmark_times.bhm_set_total_ms,
        time_elapsed
    );
    #endif

    free(map);
}
