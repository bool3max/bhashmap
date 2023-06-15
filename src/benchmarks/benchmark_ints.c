#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <time.h>

#include "bhashmap.h"

#define ITERATIONS 1

int main(void) {
    struct timespec time_start, time_end;
    size_t total_time_ms = 0;

    for (size_t j = 0; j < ITERATIONS; j++) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &time_start);

        BHashMap *map = bhm_create(32, NULL);
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

        clock_gettime(CLOCK_MONOTONIC_RAW, &time_end);
        uint64_t ms_elapsed  = ((time_end.tv_sec * 1000000000 + time_end.tv_nsec) - (time_start.tv_sec * 1000000000 + time_start.tv_nsec)) / 1000000;
        total_time_ms += ms_elapsed;
    }

    fprintf(stderr, "Avg. time of %d ITERATIONS: %lums\n", ITERATIONS, total_time_ms / ITERATIONS);

    return EXIT_SUCCESS;
}
