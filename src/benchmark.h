#include <time.h>
#include <stdint.h>

/* return: nanos at bench start*/
uint64_t
start_benchmark(void) {
    struct timespec benchmark_time_start;
    clock_gettime(CLOCK_MONOTONIC_RAW, &benchmark_time_start);

    return (benchmark_time_start.tv_nsec) + (benchmark_time_start.tv_sec * 1000000000);
}

/*
Return time elapsed since benchmark was started, in milliseconds.
*/
static uint64_t
end_benchmark(const uint64_t nanos_at_start) {
    struct timespec benchmark_time_end;
    clock_gettime(CLOCK_MONOTONIC_RAW, &benchmark_time_end);

    uint64_t nanos_at_end = (benchmark_time_end.tv_nsec) + (benchmark_time_end.tv_sec * 1000000000);

    return (nanos_at_end - nanos_at_start) / 1000000;
}
