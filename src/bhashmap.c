#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

#include "bhashmap.h"
#include "benchmark.h"

#define BHM_LOAD_FACTOR_LIMIT 0.75
#define BHM_RESIZE_FACTOR 2

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

#define HASH(dataptr, datalen) murmur3_32(dataptr, datalen, 1u)

typedef struct HashPair {
    void *key;
    size_t keylen;
    const void *value;
    struct HashPair *next;
} HashPair;

struct BHashMap {
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

static void
free_buckets(HashPair **buckets, const size_t bucket_count);

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
        if (map->buckets[i]->key == NULL) {
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
bhm_create(const size_t initial_capacity) {
    BHashMap *new_map = malloc(sizeof(BHashMap));

    if (!new_map) {
        return NULL;
    }

    *new_map = (BHashMap) {
        .capacity = initial_capacity,
        .pair_count = 0,
        .buckets = calloc(initial_capacity, sizeof(HashPair *)),
        #ifdef BHM_DEBUG_BENCHMARK
        .debug_benchmark_times = (struct _debugBenchmarkTimes) {
            0, 0, 0
        }
        #endif
    };

    if (!new_map->buckets) {
        free(new_map);
        return NULL;
    }

    for (size_t i = 0; i < initial_capacity; i++) {
        new_map->buckets[i] = malloc(sizeof(HashPair));
        if (!new_map->buckets[i]) {
            /* free all previously allocated buckets */
            for (size_t j = 0; j <= i; j++)  {
                free(new_map->buckets[j]);
            }

            free(new_map->buckets);
            free(new_map);
            return NULL;
        }

        /* zero-out all fields of the struct */
        *(new_map->buckets[i]) = (struct HashPair) {0};
    }

    DEBUG_PRINT("\e[93;1mbhm_create\e[0m: created hash map with capacity %lu.\n", initial_capacity);

    return new_map;
}

/*
Find and return the appropriate bucket for a given key,
based on its hash and the capacity of the hashmap.
*/
static HashPair *
find_bucket(BHashMap *map, const void *key, const size_t keylen) {
    const uint32_t hash       = HASH(key, keylen),
                   bucket_idx = hash % map->capacity;

    DEBUG_PRINT("KEY: '%s', BUCKET IDX: %u\n", (char *) key, bucket_idx);

    return map->buckets[bucket_idx];
}

/*
Insert a key-value pair into a HashPair structure.
*/
static inline bool
insert_pair(HashPair *pair, const void *key, const size_t keylen, const void *data) {
    pair->key = malloc(keylen);
    pair->keylen = keylen;

    if (!pair->key) {
        return false;
    }

    memcpy(pair->key, key, keylen);
    pair->value = data;

    return true;
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

    size_t capacity_new = map->capacity * BHM_RESIZE_FACTOR,
           capacity_old = map->capacity;

    HashPair **buckets_new = calloc(capacity_new, sizeof(HashPair *)),
             **buckets_old = map->buckets;

    if (!buckets_new) {
        DEBUG_PRINT("\tresizing %lu -> %lu failed\n", capacity_old, capacity_new);
        return false;
    }

    /* allocate the new bucket pairs */
    for (size_t i = 0; i < capacity_new; i++) {
        buckets_new[i] = malloc(sizeof(HashPair));
        if (!buckets_new[i]) {
            DEBUG_PRINT("\tfailed allocating space for new HashPair\n");
            for (size_t j = 0; j <= i; j++) {
                free(buckets_new[j]);
            }

            free(buckets_new);
            return false;
        }

        /* zero all fields of new struct*/
        *(buckets_new[i]) = (struct HashPair) {0};
    }

    map->buckets = buckets_new;
    map->capacity = capacity_new;

    // rehash every HashPair in the old table

    for (size_t idx_old = 0; idx_old < capacity_old; idx_old++) {
        HashPair *head = buckets_old[idx_old];

        /* head bucket pair empty -- no need to rehash it / move it, just free its memory */
        /* no need to traverse its list, as it doesn't have one */
        if (head->key == NULL) {
            free(head);
            continue;
        }

        /* head bucket pair exists */
        while (head) {
            /* find new bucket position for this pair*/
            HashPair *bucket_new = find_bucket(map, head->key, head->keylen);

            /* head dest bucket empty */
            /* insert current old head's data into new head dest bucket, free old head */
            if (bucket_new->key == NULL) {
                insert_pair(bucket_new, head->key, head->keylen, head->value);

                HashPair *n = head->next;

                free(head->key);
                free(head);

                head = n;
                continue;
            }

            /* head dest bucket not empty, find last pair in ll, copy current old head over */
            while (bucket_new->next != NULL) {
                bucket_new = bucket_new->next;
            }

            HashPair *n = head->next;

            bucket_new->next = head;
            head->next = NULL;

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

    HashPair *bucket = find_bucket(map, key, keylen);
    
    /* best case - given bucket completely empty - insert key-value pair */
    if (bucket->key == NULL) {
        if (!insert_pair(bucket, key, keylen, data)) {
            return false;
        }

        map->pair_count += 1;

        #ifdef BHM_DEBUG_BENCHMARK
        uint64_t time_elapsed = end_benchmark(bench_start_nanos);
        map->debug_benchmark_times.bhm_set_total_ms += time_elapsed;
        #endif

        if (get_load_factor(map) >= BHM_LOAD_FACTOR_LIMIT) {
            resize(map);
        }

        return true;
    }

    HashPair *head = bucket;

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
            HashPair *new_pair = malloc(sizeof(HashPair));
            if (!new_pair) {
                return false;
            }

            if (!insert_pair(new_pair, key, keylen, data)) {
                free(new_pair);
                return false;
            }

            new_pair->next = NULL;
            head->next = new_pair;

            map->pair_count += 1;

            #ifdef BHM_DEBUG_BENCHMARK
            uint64_t time_elapsed = end_benchmark(bench_start_nanos);
            map->debug_benchmark_times.bhm_set_total_ms += time_elapsed;
            #endif

            if (get_load_factor(map) >= BHM_LOAD_FACTOR_LIMIT) {
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
bhm_get(BHashMap *map, const void *key, const size_t keylen) {
    #ifdef BHM_DEBUG_BENCHMARK
    uint64_t bench_start_nanos = start_benchmark();
    #endif

    HashPair *bucket = find_bucket(map, key, keylen);

    /* empty bucket, key definitely not in map */
    if (!bucket->key) {
        return NULL;
    }

    while (bucket) {
        if (bucket->keylen == keylen && memcmp(key, bucket->key, keylen) == 0) {
            return (void *) bucket->value;
        }

        bucket = bucket->next;
    }

    #ifdef BHM_DEBUG_BENCHMARK
    uint64_t time_elapsed = end_benchmark(bench_start_nanos);
    map->debug_benchmark_times.bhm_get_total_ms += time_elapsed;
    #endif

    return NULL;
}

static void
free_buckets(HashPair **buckets, const size_t bucket_count) {
    for (size_t i = 0; i < bucket_count; i++)  {
        HashPair *head = buckets[i];

        while (head) {
            HashPair *n = head->next;

            free(head->key);
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
        HashPair *pair = map->buckets[i];

        if (pair->key == NULL) {
            continue;
        }

        while (pair) {
            callback_function(pair->key, pair->keylen, (void *) pair->value);
            pair = pair->next;
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
    free(map);

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
}
