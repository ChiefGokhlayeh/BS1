#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "bench_utils.h"

int main(int argc, char *argv[])
{
    const int sizes[] = {
        128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
        65536, 131072, 262144, 524288, 1048576, 2097152,
        4194304, 8388608, 16777216, 33554432, 67108864};
    const int sizes_num = sizeof(sizes) / sizeof(sizes[0]);
#define MAX_SIZE sizes[sizes_num - 1]
    int pipe_parent_to_child[2];
    char *buffer;
    pid_t pid;
    pid_t pid_child;
    int ret;

    ret = pipe(pipe_parent_to_child);
    if (-1 == ret)
        ERROR("pipe parent_to_child", errno);

    pid = getpid();
    ret = pid_child = fork();
    if (-1 == ret)
        ERROR("fork", errno);

    buffer = malloc(MAX_SIZE);
    if (NULL == buffer)
        ERROR("malloc", ENOMEM);
    memset(buffer, 'a', MAX_SIZE);

    if (0 == ret)
    {
        /* CHILD Process */
        close(pipe_parent_to_child[1]);

        for (int i = 0; i < sizes_num; i++)
        {
            for (int j = 0; j < MEASUREMENTS; j++)
            {
                int current_size = sizes[i];
                do
                {
                    int nread = read(pipe_parent_to_child[0], buffer, current_size);
                    current_size -= nread;
                } while (current_size > 0);
            }
        }

        DEBUG(printf("PID:%d (CHILD) waits\n",
                     (int)pid));
        pause();
        close(pipe_parent_to_child[0]);
        DEBUG(printf("PID:%d (CHILD) exits\n",
                     (int)pid));

        return EXIT_SUCCESS;
    }
    int *ticks;

    ticks = malloc(MEASUREMENTS * sizeof(int));
    if (NULL == ticks)
        ERROR("malloc", ENOMEM);
    memset(ticks, 0, MEASUREMENTS * sizeof(int));

    close(pipe_parent_to_child[0]);

    for (int i = 0; i < sizes_num; i++)
    {
        int current_size = sizes[i];
        int nwrite;
        int j;
        int min_ticks;
        int max_ticks;
        long long ticks_all;
        struct timeval tv_start;
        struct timeval tv_stop;
        double time_delta_sec;

        assert(current_size <= MAX_SIZE);

        gettimeofday(&tv_start, NULL);
        for (j = 0; j < MEASUREMENTS; j++)
        {
            unsigned long long start;
            unsigned long long stop;
            start = getrdtsc();
            nwrite = write(pipe_parent_to_child[1], buffer, current_size);
            stop = getrdtsc();
            assert(nwrite == current_size);
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
               (double)ticks_all / (MEASUREMENTS - 2.0), MEASUREMENTS, nwrite,
               ((double)current_size * MEASUREMENTS) / (1024.0 * 1024.0 * time_delta_sec));
    }

    DEBUG(printf("PID:%d sending shutdown command\n",
                 (int)pid));
    kill(pid_child, SIGTERM);
    wait(NULL);
    close(pipe_parent_to_child[1]);

    return EXIT_SUCCESS;
}
