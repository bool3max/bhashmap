#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include "bhashmap.h"

#define ITERATIONS 16

int main(void) {
    struct timespec time_start, time_end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time_start);

    for (size_t j = 0; j < ITERATIONS; j++) {
        BHashMap *map = bhm_create(32);
        if (!map) {
            fprintf(stderr, "error\n");
            return EXIT_FAILURE;
        }
        
        static const size_t upper_limit = 1 << 19;

        for (size_t i = 0; i < upper_limit; i++) {
            if (!bhm_set(map, &i, sizeof(size_t), &i)) {
                fprintf(stderr, "error setting key. aborting.\n");
                bhm_destroy(map);
                return EXIT_FAILURE;
            }
        }

        bhm_destroy(map);
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &time_end);

    uint64_t nano_elapsed = (time_end.tv_sec * 1000000000 + time_end.tv_nsec) - (time_start.tv_sec * 1000000000 + time_start.tv_nsec);

    printf("%d iterations took: %lums\n", ITERATIONS, nano_elapsed / 1000000);
    return EXIT_SUCCESS;
}
