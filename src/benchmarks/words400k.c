#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bhashmap.h"

#define TIMER_GET(s) clock_gettime(CLOCK_MONOTONIC_RAW, s);
#define TIMER_DIFF(s, e) ((e.tv_sec * 1000000000 + e.tv_nsec) - (s.tv_sec * 1000000000 + s.tv_nsec))

#define WORDS_COUNT 466550
#define MAXWORDLEN  50

#define DEFAULT_ITER_COUNT 16

static BHashMapConfig hashmap_config; // zeroed-out by default

int access_all(size_t iterations, const char *path) {
    fprintf(
        stderr,
        "Benchmark: Access keys of all %d words\n"
        "ITERATIONS: %lu\n"
        "WORDS.TXT path: %s\n"
        "------------------\n",
        WORDS_COUNT,
        iterations,
        path
    );

    struct wordpair {
        char word[MAXWORDLEN];
        size_t len;
    };

    /* load all the words into memory and insert them into the hashmap as keys */
    FILE *words_file = fopen(path, "r");
    if (!words_file) {
        return EXIT_FAILURE;
    }

    struct wordpair *words = malloc(WORDS_COUNT * sizeof(struct wordpair));
    if (!words) {
        fclose(words_file);
        return EXIT_FAILURE;
    }

    size_t i = 0;
    while (fgets(words[i].word, MAXWORDLEN, words_file)) {
        words[i].len = strlen(words[i].word);
        i += 1;
    }

    fclose(words_file);

    BHashMap *map = bhm_create(0, &hashmap_config);
    if (!map) {
        free(words);
        return EXIT_FAILURE;
    }

    for (size_t j = 0; j < WORDS_COUNT; j++) {
        bhm_set(map, words[j].word, words[j].len, (void *) 0x1234);
    }

    /* ---------------------------------------- */

    struct timespec time_start, time_end;
    size_t ns_total = 0;    

    for (size_t i = 0; i < iterations; i++) {
        TIMER_GET(&time_start);

        for (size_t j = 0; j < WORDS_COUNT; j++) {
            void *val = bhm_get(map, words[j].word, words[j].len);
        }

        TIMER_GET(&time_end);
        ns_total += TIMER_DIFF(time_start, time_end);
    }

    bhm_destroy(map);

    fprintf(
        stderr,
        "%-30s: %lums\n"
        "%-30s: %luns\n",
        "RUNTIME:",
        ns_total / iterations / 1000000,
        "AVG. TIME TO ACCESS KEY:",
        ns_total / iterations / WORDS_COUNT
    );

    free(words);
    return EXIT_SUCCESS;
}

int insert_all_create_destroy(size_t iterations, const char *path) {
    fprintf(
        stderr,
        "Benchmark: Create hashmap->Insert all %d words as keys->Destroy hashmap\n"
        "ITERATIONS: %lu\n"
        "WORDS.TXT path: %s\n"
        "------------------\n",
        WORDS_COUNT,
        iterations,
        path
    );

    struct wordpair {
        char word[MAXWORDLEN];
        size_t len;
    };

    /* load all the words into memory */
    FILE *words_file = fopen(path, "r");
    if (!words_file) {
        return EXIT_FAILURE;
    }

    struct wordpair *words = malloc(WORDS_COUNT * sizeof(struct wordpair));
    if (!words) {
        fclose(words_file);
        return EXIT_FAILURE;
    }

    size_t i = 0;
    while (fgets(words[i].word, MAXWORDLEN, words_file)) {
        words[i].len = strlen(words[i].word);
        i += 1;
    }

    fclose(words_file);
    /* ---------------------------------------- */

    struct timespec time_start, time_end;
    size_t ns_total = 0;    

    for (size_t i = 0; i < iterations; i++) {
        TIMER_GET(&time_start);
        BHashMap *map = bhm_create(0, &hashmap_config);
        for (size_t j = 0; j < WORDS_COUNT; j++) {
            bhm_set(map, words[j].word, words[j].len, (void *) 0x1234);
        }

        bhm_destroy(map);

        TIMER_GET(&time_end);
        ns_total += TIMER_DIFF(time_start, time_end);
    }

    fprintf(
        stderr,
        "%-30s: %lums\n",
        "RUNTIME:",
        ns_total / iterations / 1000000
    );


    free(words);

    return EXIT_SUCCESS;
}

int insert_all(size_t iterations, const char *path) {
    fprintf(
        stderr,
        "Benchmark: Insert all %d words as keys\n"
        "ITERATIONS: %lu\n"
        "WORDS.TXT path: %s\n"
        "------------------\n",
        WORDS_COUNT,
        iterations,
        path
    );

    struct wordpair {
        char word[MAXWORDLEN];
        size_t len;
    };

    /* load all the words into memory */
    FILE *words_file = fopen(path, "r");
    if (!words_file) {
        return EXIT_FAILURE;
    }

    struct wordpair *words = malloc(WORDS_COUNT * sizeof(struct wordpair));
    if (!words) {
        fclose(words_file);
        return EXIT_FAILURE;
    }

    size_t i = 0;
    while (fgets(words[i].word, MAXWORDLEN, words_file)) {
        words[i].len = strlen(words[i].word);
        i += 1;
    }

    fclose(words_file);
    /* ---------------------------------------- */

    struct timespec time_start, time_end;
    size_t ns_total = 0;    

    for (size_t i = 0; i < iterations; i++) {
        BHashMap *map = bhm_create(0, &hashmap_config);

        TIMER_GET(&time_start);

        for (size_t j = 0; j < WORDS_COUNT; j++) {
            bhm_set(map, words[j].word, words[j].len, (void const *) 0x1234);
        }

        TIMER_GET(&time_end);
        ns_total += TIMER_DIFF(time_start, time_end);

        bhm_destroy(map);
    }

    fprintf(
        stderr,
        "%-30s: %lums\n"
        "%-30s: %luns\n",
        "RUNTIME:",
        ns_total / iterations / 1000000,
        "AVG. TIME TO INSERT KEY:",
        ns_total / iterations / WORDS_COUNT
    );

    free(words);
    return EXIT_SUCCESS;
}

/* usage: ./prog <type> <words.txt_file_path> [<iterations>] [<max_load_factor>] [<resize_growth_factor>] */
int main(int argc, char **argv) {
    size_t iterations = argc >= 4 ? atoll(argv[3]) : DEFAULT_ITER_COUNT;

    /* this instance is accessed by all the individual benchmark functions, we init it in main */
    hashmap_config = (BHashMapConfig) {
        .max_load_factor = argc >= 5 ? atof(argv[4]) : 0,
        .resize_growth_factor = argc >= 6 ? atoll(argv[5]) : 0,
        .hashfunc = NULL
    };

    fprintf(
        stderr,
        "HASHMAP CONFIGURATION:\n"
        "\tMAX. LOAD FACTOR: %.3lf\n"
        "\tRESIZE GROWTH FACTOR: %lu\n"
        "---------------------------\n",
        hashmap_config.max_load_factor,
        hashmap_config.resize_growth_factor
    );

    if (strcmp(argv[1], "access") == 0) {
        return access_all(iterations, argv[2]);
    } else if (strcmp(argv[1], "insert") == 0) {
        return insert_all(iterations, argv[2]);
    } else if (strcmp(argv[1], "insert_create_destroy") == 0) {
        return insert_all_create_destroy(iterations, argv[2]);
    } else return EXIT_FAILURE;
}
