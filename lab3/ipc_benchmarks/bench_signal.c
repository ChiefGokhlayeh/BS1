/*
 * Small benchmark of Unix signal handling
 *
 * Author: Rainer Keller, HS Esslingen
 */

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include "bench_utils.h"

#define SLEEP_TIME 10

void signal_handler(int signum)
{
    DEBUG(printf("Caught signal %d\n", signum));
    if (SIG_ERR == signal(SIGUSR1, &signal_handler))
        ERROR("signal", errno);
}

int main(int argc, char *argv[])
{
    pid_t pid;
    pid_t pid_child;
    int ret;
    const double current_size = 1. / 8.; // Assume a signal is 1 Bit worth of data...

    pid = getpid();

    ret = pid_child = fork();
    if (ret == -1)
        ERROR("fork", errno);

    if (0 == ret)
    {
        /* CHILD */
        int *ticks;
        int i;
        int min_ticks;
        int max_ticks;
        int ticks_all;
        struct timeval tv_start;
        struct timeval tv_stop;
        double time_delta_sec;

        ticks = malloc(MEASUREMENTS * sizeof(int));
        if (NULL == ticks)
            ERROR("malloc", ENOMEM);
        memset(ticks, 0, MEASUREMENTS * sizeof(int));

        gettimeofday(&tv_start, NULL);
        for (i = 0; i < MEASUREMENTS; i++)
        {
            unsigned long long start, stop;
            start = getrdtsc();
            kill(pid, SIGUSR1);
            stop = getrdtsc();
            ticks[i] = stop - start;
        }
        gettimeofday(&tv_stop, NULL);

        min_ticks = INT_MAX;
        max_ticks = INT_MIN;
        ticks_all = 0;
        for (i = 0; i < MEASUREMENTS; i++)
        {
            if (min_ticks > ticks[i])
                min_ticks = ticks[i];
            if (max_ticks < ticks[i])
                max_ticks = ticks[i];
            ticks_all += ticks[i];
        }
        ticks_all -= min_ticks;
        ticks_all -= max_ticks;

        time_delta_sec = ((tv_stop.tv_sec - tv_start.tv_sec) + ((tv_stop.tv_usec - tv_start.tv_usec) / (1000.0 * 1000.0)));

        printf("PID:%d time: min:%d max:%d Ticks Avg without min/max:%f Ticks (for %d measurements) for %d Bytes (%.2f MB/s)\n",
               pid, min_ticks, max_ticks,
               (double)ticks_all / (MEASUREMENTS - 2.0), MEASUREMENTS,
               (int)(current_size * MEASUREMENTS),
               ((double)current_size * MEASUREMENTS) / (1024.0 * 1024.0 * time_delta_sec));
    }
    else if (0 < ret)
    {
        /* PARENT */
        DEBUG(printf("PID:%d (PARENT) Waiting for signals from Child pid:%d\n",
                     (int)pid, (int)pid_child));

        if (SIG_ERR == signal(SIGUSR1, &signal_handler))
            ERROR("signal", errno);
    }

    // Both Child and Parent sleep shortly to allow signals to be received.
    sleep(SLEEP_TIME);
    return 0;
}
