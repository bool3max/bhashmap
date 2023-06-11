#include <time.h>
#include <stdint.h>

static struct timespec benchmark_time_start,
                       benchmark_time_end;

static void
start_benchmark(void) {
    clock_gettime(CLOCK_MONOTONIC, &benchmark_time_start);
}

/*
Return time elapsed since benchmark was started, in milliseconds.
*/
static uint64_t
end_benchmark(void) {
    clock_gettime(CLOCK_MONOTONIC, &benchmark_time_end);
    uint64_t nanos_elapsed = (benchmark_time_end.tv_sec * 1000000000 + benchmark_time_end.tv_nsec)
                             - (benchmark_time_start.tv_sec * 1000000000 + benchmark_time_start.tv_nsec);

    return nanos_elapsed / 1000000;
}
