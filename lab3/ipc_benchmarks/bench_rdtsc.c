/*
 * Small benchmark as a basis for further timing.
 *
 * Author: Rainer Keller, HS Esslingen
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <assert.h>

// We have to drastically increase to get stable timings.
#ifndef MEASUREMENTS
#define MEASUREMENTS (100 * 1000 * 1000)
#endif

#include "bench_utils.h"

int main(int argc, char *argv[])
{
    pid_t pid = getpid();
    struct timeval tv_start;
    struct timeval tv_stop;
    unsigned long long ret = 0;
    int i;

    // This measures the time for MEASUREMENTS number of one getrdtsc-calls,
    // including the loop-bodys dec/decrement and jne/jump-not-equal instruction.
    // The ret+ addition is just to make sure rdtsc is not optimized away.
    gettimeofday(&tv_start, NULL);
    for (i = 0; i < MEASUREMENTS; i++)
    {
        ret += getrdtsc();
    }
    gettimeofday(&tv_stop, NULL);
    double time_delta_sec = ((tv_stop.tv_sec - tv_start.tv_sec) * (1000.0 * 1000.0) +
                             ((tv_stop.tv_usec - tv_start.tv_usec))) /
                            (double)MEASUREMENTS;

    // This measures the time for MEASUREMENTS number for two getrdtsc-calls.
    // The difference of the two will allow us to see know the time for one rdtsc.
    gettimeofday(&tv_start, NULL);
    for (i = 0; i < MEASUREMENTS; i++)
    {
        ret += getrdtsc();
        ret += getrdtsc();
    }
    gettimeofday(&tv_stop, NULL);

    double time_delta_rdtsc_sec = ((tv_stop.tv_sec - tv_start.tv_sec) * (1000.0 * 1000.0) +
                                   ((tv_stop.tv_usec - tv_start.tv_usec))) /
                                  (double)MEASUREMENTS;
    // The difference is the time for one rdtsc-instruction.
    time_delta_rdtsc_sec -= time_delta_sec;
    printf("PID:%d MEASUREMENTS: %d time per getrdtsc(): %f microseconds (10^-6 seconds), %f nanoseconds (10^-9 seconds)\n",
           pid, MEASUREMENTS, time_delta_rdtsc_sec, time_delta_rdtsc_sec * 1000.0);
    return 0;
}
