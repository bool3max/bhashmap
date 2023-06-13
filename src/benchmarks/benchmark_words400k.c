#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include "bhashmap.h"

#define MAXWORDLEN 50
#define ITERATIONS 1

int main(void) {
    char (*words)[MAXWORDLEN] = malloc(WORDSTXT_COUNT * MAXWORDLEN);
    FILE *f = fopen("./words.txt", "r");

    size_t i = 0;
    while (fgets(words[i], MAXWORDLEN, f)) {
        i++;
    }

    fclose(f);

    struct timespec time_start, time_end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_start);

    for (size_t j = 0; j < ITERATIONS; j++) {
        BHashMap *map = bhm_create(32);

        if (!map) {
            fprintf(stderr, "Error creating hash table.\n");
            exit(1);
        }

        for (size_t i = 0; i < WORDSTXT_COUNT; i++) {
            if (!bhm_set(map, words[i], strlen(words[i]) + 1, words[i])) {
                fprintf(stderr, "Error setting key.\n");
                bhm_destroy(map);
                exit(1);
            }
        }

        bhm_destroy(map);
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &time_end);

    uint64_t nano_elapsed = (time_end.tv_sec * 1000000000 + time_end.tv_nsec) - (time_start.tv_sec * 1000000000 + time_start.tv_nsec);

    printf("%d iterations took: %lums\n", ITERATIONS, nano_elapsed / 1000000);

    free(words);
    return EXIT_SUCCESS;
}
