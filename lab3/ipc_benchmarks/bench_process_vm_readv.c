/*
 * Small benchmark of Linux' process_vm_readv / process_vm_writev calls,
 * see man 2 process_vm_readv
 * These two function calls were added in Linux v3.2
 * 
 * Author: Rainer Keller, HS Esslingen
 */
#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <sys/uio.h>

#include "bench_utils.h"

int main(int argc, char * argv[]) {
    const int sizes[] = {128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288};
    const int sizes_num = sizeof(sizes)/sizeof(sizes[0]);
#define MAX_SIZE sizes[sizes_num-1]
    int pipe_child_to_parent[2];
    int pipe_parent_to_child[2];
    char * buffer;
    pid_t pid;
    pid_t pid_child;
    int ret;

    // Open pipe to communicate the pointer to buffer
    ret = pipe(pipe_child_to_parent);
    if (-1 == ret)
        ERROR("pipe child_to_parent", errno);
    ret = pipe(pipe_parent_to_child);
    if (-1 == ret)
        ERROR("pipe parent_to_child", errno);

    pid = getpid();
    ret = pid_child = fork();
    if (-1 == ret)
        ERROR("fork", errno);

    if (0 == ret) {
        /* CHILD Process */
        int num_written;
        int to_write;
        char ** ptr;

        pid = getpid();
        DEBUG(printf("PID:%d (CHILD) starts\n",
                     (int) pid));

        // close the reading end in child_to_parent and writing end in parent_to_child
        ret = close (pipe_child_to_parent[0]);
        ret = close (pipe_parent_to_child[1]);

        buffer = malloc(MAX_SIZE);
        if (NULL == buffer)
            ERROR("malloc", ENOMEM);
        memset (buffer, 0, MAX_SIZE);

        // Write the child's buffer address to the parent
        ptr = &buffer;
        for (num_written = 0, to_write=sizeof(char*); num_written < sizeof(char*); num_written += ret, to_write -= ret)
            ret = write (pipe_child_to_parent[1], ptr+num_written, to_write);

        DEBUG(printf("PID:%d (CHILD) waits\n",
                     (int) pid));
        ret = read(pipe_parent_to_child[0], &num_written, 1);
        DEBUG(printf("PID:%d (CHILD) exits\n",
                     (int) pid));

        exit (EXIT_SUCCESS);
    }
    int num_read;
    int to_read;
    char * remote_buffer;
    char ** ptr;
    int * ticks;
    struct iovec local[1];
    struct iovec remote[1];
    int i;
    DEBUG(printf("PID:%d (PARENT) starts child_pid:%d\n",
                 (int) pid, (int)pid_child));
    // close the reading end in parent_to_child and writing end in child_to_parent
    ret = close (pipe_parent_to_child[0]);
    ret = close (pipe_child_to_parent[1]);

    // Read the child's buffer_address
    ptr = &remote_buffer;
    for (num_read = 0, to_read = sizeof(char*); num_read < sizeof(char*); num_read += ret, to_read -= ret)
        ret = read(pipe_child_to_parent[0], ptr+num_read, to_read);

    DEBUG(printf("PID:%d (PARENT) remote_buffer:%p\n",
                 (int) pid, remote_buffer));

    buffer = malloc(MAX_SIZE);
    if (NULL == buffer)
        ERROR("malloc", ENOMEM);
    memset (buffer, 0, MAX_SIZE);
        
    ticks = malloc (MEASUREMENTS * sizeof (int));
    if (NULL == ticks)
        ERROR("malloc", ENOMEM);
    memset (ticks, 0, MEASUREMENTS * sizeof(int));

    /* PARENT Process: WRITES into other process' memory! */
    memset(buffer, 'a', MAX_SIZE);
    local[0].iov_base = (void *) buffer;
    remote[0].iov_base = (void *) remote_buffer;

    for (i = 0; i < sizes_num; i++) {
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

        local[0].iov_len = current_size;
        remote[0].iov_len = current_size;

        gettimeofday(&tv_start, NULL);
        for (j = 0; j < MEASUREMENTS; j++) {
            unsigned long long start;
            unsigned long long stop;
            start = getrdtsc();
            nwrite = process_vm_writev(pid_child, local, 1, remote, 1, 0);
            stop = getrdtsc();
            assert (nwrite == current_size);
            ticks[j] = stop - start;
        }
        gettimeofday(&tv_stop, NULL);

        min_ticks = INT_MAX;
        max_ticks = INT_MIN;
        ticks_all = 0;
        for (j = 0; j < MEASUREMENTS; j++) {
            if (min_ticks > ticks[j]) min_ticks = ticks[j];
            if (max_ticks < ticks[j]) max_ticks = ticks[j];
            ticks_all += ticks[j];
        }
        ticks_all -= min_ticks;
        ticks_all -= max_ticks;

        time_delta_sec = ((tv_stop.tv_sec - tv_start.tv_sec) + ((tv_stop.tv_usec - tv_start.tv_usec) / (1000.0 * 1000.0)));

        printf("PID:%d time: min:%d max:%d Ticks Avg without min/max:%f Ticks (for %d measurements) for %d Bytes (%.2f MB/s)\n",
               pid, min_ticks, max_ticks,
               (double) ticks_all / (MEASUREMENTS - 2.0), MEASUREMENTS, nwrite,
               ((double) current_size*MEASUREMENTS)/(1024.0*1024.0*time_delta_sec));
    }

    // Tell Child to exit, too:
    write(pipe_parent_to_child[1], &pid, 1);
    return EXIT_SUCCESS;
}
