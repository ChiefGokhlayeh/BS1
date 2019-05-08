#include <stdio.h>
#include <stdlib.h>

#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <unistd.h>

int main(void)
{
    puts("First fork");
    pid_t fork1 = fork();
    pid_t fork2 = 0;
    if (fork1 == 0)
    {
        puts("Second fork");
        fork2 = fork();
        if (fork2 != 0)
        {
            sleep(3);
            waitpid(fork2, NULL, 0);
        }
        sleep(10);
    }
    else
    {
        sleep(1);
        puts("Killing fork1");
        kill(fork1, SIGTERM);
        waitpid(fork1, NULL, 0);
    }
    printf("PID=%d, PPID=%d, PGRP=%d, fork1=%d, fork2=%d\n", getpid(), getppid(), getpgrp(), fork1, fork2);

    return EXIT_SUCCESS;
}
