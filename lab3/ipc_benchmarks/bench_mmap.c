/*
 * Small benchmark of Anonymous Memory Mmaps.
 *
 * Author: Rainer Keller, HS Esslingen
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <sys/time.h>

#include "bench_utils.h"

#define SLEEP_TIME 1

// #define USE_MEMSET

int main(int argc, char *argv[])
{
    const int sizes[] = {128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288};
    const int sizes_num = sizeof(sizes) / sizeof(sizes[0]);
#define MAX_SIZE sizes[sizes_num - 1]
    pid_t pid = getpid();
    pid_t pid_child;
    char *anon;
    int i;

    /**
     * Anlegen einer Anonymen, gesharten Memory-Map, in die gelesen und geschrieben wird.
     *
     * Aufruf:
     *  void * mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
     */
    anon = mmap(NULL, MAX_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);

    if (anon == MAP_FAILED)
        ERROR("mmap anon", errno);

    int ret = pid_child = fork();

    if (-1 == ret)
        ERROR("fork", errno);

    if (0 == ret)
    {
        /* CHILD */
        char *buffer;
        buffer = malloc(MAX_SIZE);
        if (NULL == buffer)
            ERROR("malloc", ENOMEM);
        memset(buffer, 0, MAX_SIZE);
        pid_child = getpid();
        memcpy(anon, buffer, MAX_SIZE);
        sleep(SLEEP_TIME);
        printf("PID %d (CHILD): COPY DONE\n", pid_child);
        return (EXIT_SUCCESS);
    }

    int *ticks;
    ticks = malloc(MEASUREMENTS * sizeof(int));
    if (NULL == ticks)
        ERROR("malloc", ENOMEM);
    memset(ticks, 0, MEASUREMENTS * sizeof(int));

    /* PARENT: measure the writing into the buffer */
    for (i = 0; i < sizes_num; i++)
    {
        int current_size = sizes[i];
        int j;
        int min_ticks;
        int max_ticks;
        long long ticks_all;
        struct timeval tv_start;
        struct timeval tv_stop;
        double time_delta_sec;

        sleep(SLEEP_TIME);

        gettimeofday(&tv_start, NULL);
        for (j = 0; j < MEASUREMENTS; j++)
        {
            unsigned long long start;
            unsigned long long stop;
            start = getrdtsc();
#ifdef USE_MEMSET
            memset(anon, 'a', current_size);
#else
            {
                int k;
                for (k = 0; k < current_size; k++)
                    anon[k] = 'a';
            }
#endif
            stop = getrdtsc();
            ticks[j] = stop - start;
        }
        gettimeofday(&tv_stop, NULL);

        min_ticks = INT_MAX;
        max_ticks = INT_MIN;
        ticks_all = 0;
        for (j = 0; j < MEASUREMENTS; j++)
        {
            if (min_ticks > ticks[j])
                min_ticks = ticks[j];
            if (max_ticks < ticks[j])
                max_ticks = ticks[j];
            ticks_all += ticks[j];
        }
        ticks_all -= min_ticks;
        ticks_all -= max_ticks;

        time_delta_sec = ((tv_stop.tv_sec - tv_start.tv_sec) + ((tv_stop.tv_usec - tv_start.tv_usec) / (1000.0 * 1000.0)));

        printf("PID:%d time: min:%d max:%d Ticks Avg without min/max:%f Ticks (for %d measurements) for %d Bytes (%.2f MB/s)\n",
               pid, min_ticks, max_ticks,
               (double)ticks_all / (MEASUREMENTS - 2.0), MEASUREMENTS, current_size,
               ((double)current_size * MEASUREMENTS) / (1024.0 * 1024.0 * time_delta_sec));
    }

    return (EXIT_SUCCESS);
}
