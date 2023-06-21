#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include "bhashmap.h"

#define CLOCK_START clock_gettime(CLOCK_MONOTONIC_RAW, &time_start);
#define CLOCK_END clock_gettime(CLOCK_MONOTONIC_RAW, &time_end);
#define MS_ELAPSED (((time_end.tv_sec * 1000000000 + time_end.tv_nsec) - (time_start.tv_sec * 1000000000 + time_start.tv_nsec)) / 1000000)

#define MAXWORDLEN 50
#define ITERATIONS 4

int main(void) {
    fprintf(stderr, "ITERATIONS: %d\n", ITERATIONS);

    /* Load all words into memory */
    char (*words)[MAXWORDLEN] = malloc(WORDSTXT_COUNT * MAXWORDLEN);
    FILE *f = fopen("./words.txt", "r");

    size_t i = 0;
    while (fgets(words[i], MAXWORDLEN, f)) {
        i++;
    }

    fclose(f);
    /* ------------------ */

    struct timespec time_start, time_end;

    size_t total_create_insert_destroy = 0,
           total_insert_all_keys = 0,
           total_access_all_keys = 0;

    BHashMapConfig current_bench_config;
    /* ------------------- */


    /* BENCH: create-map -> insert all keys -> destroy-map -- DEFAULT CONFIGURATION */

    for (size_t i = 0; i < ITERATIONS; i++) {
        CLOCK_START 

        BHashMap *map = bhm_create(0, NULL);
        if (!map) {
            fprintf(stderr, "Error creating hash table.\n");
            exit(1);
        }
        for (size_t j = 0; j < WORDSTXT_COUNT; j++) {
            if (!bhm_set(map, words[j], strlen(words[j]), words[j])) {
                fprintf(stderr, "Error setting key.\n");
                bhm_destroy(map);
                exit(1);
            }
        }

        current_bench_config = bhm_get_config(map);
        bhm_destroy(map);

        CLOCK_END
        total_create_insert_destroy += MS_ELAPSED;
    }

    fprintf(
        stderr,
        "BENCH: Create->insert all keys->destroy:\n"
        "\tTIME: %lums\n"
        "\tCONFIG:\n"
        "\t\tMAX_LOAD_FACTOR: %.2lf\n"
        "\t\tRESIZE_GROWTH_FACTOR: %lu\n",
        total_create_insert_destroy / ITERATIONS,
        current_bench_config.max_load_factor,
        current_bench_config.resize_growth_factor
    );

    /* BENCH: Insert all keys - ONE ITERATION - DEFAUL CONFIGURATION */
    BHashMap *map_insert_all_keys = bhm_create(0, NULL);
    if (!map_insert_all_keys) {
        fprintf(stderr, "Error creating hash table.\n");
        exit(1);
    }
    current_bench_config = bhm_get_config(map_insert_all_keys);
    CLOCK_START
    for (size_t j = 0; j < WORDSTXT_COUNT; j++) {
        if (!bhm_set(map_insert_all_keys, words[j], strlen(words[j]), words[j])) {
            fprintf(stderr, "Error setting key.\n");
            bhm_destroy(map_insert_all_keys);
            exit(1);
        }
    }
    CLOCK_END
    total_insert_all_keys += MS_ELAPSED;
    fprintf(
        stderr,
        "BENCH: Insert all keys - ONE ITERATION - DEFAULT CONFIGURATION:\n"
        "\tTIME: %lums\n"
        "\tCONFIG:\n"
        "\t\tMAX_LOAD_FACTOR: %.2lf\n"
        "\t\tRESIZE_GROWTH_FACTOR: %lu\n",
        total_insert_all_keys,
        current_bench_config.max_load_factor,
        current_bench_config.resize_growth_factor
    );

    bhm_destroy(map_insert_all_keys);

    /* BENCH: access all keys -- DEFAULT CONFIGURATION */
    BHashMap *map_access_all_keys = bhm_create(0, NULL);
    if (!map_access_all_keys) {
        fprintf(stderr, "Error creating hash table.\n");
        exit(1);
    }
    current_bench_config = bhm_get_config(map_access_all_keys);
    for (size_t j = 0; j < WORDSTXT_COUNT; j++) {
        if (!bhm_set(map_access_all_keys, words[j], strlen(words[j]), words[j])) {
            fprintf(stderr, "Error setting key.\n");
            bhm_destroy(map_access_all_keys);
            exit(1);
        }
    }
    for (size_t i = 0; i < ITERATIONS; i++) {
        CLOCK_START

        for (size_t j = 0; j < WORDSTXT_COUNT; j++) {
            if (!bhm_get(map_access_all_keys, words[j], strlen(words[j]))) {
                fprintf(stderr, "Error getting key.\n");
                bhm_destroy(map_access_all_keys);
                exit(1);
            }
        }

        CLOCK_END

        total_access_all_keys += MS_ELAPSED;
    }

    bhm_destroy(map_access_all_keys);

    fprintf(
        stderr,
        "BENCH: Access all keys:\n"
        "\tTIME: %lums\n"
        "\tCONFIG:\n"
        "\t\tMAX_LOAD_FACTOR: %.3lf\n"
        "\t\tRESIZE_GROWTH_FACTOR: %lu\n",
        total_access_all_keys / ITERATIONS,
        current_bench_config.max_load_factor,
        current_bench_config.resize_growth_factor
    );

    free(words);
    return EXIT_SUCCESS;
}
