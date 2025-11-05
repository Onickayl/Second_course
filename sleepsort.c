#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{

    pid_t pids[argc];
    int numbers[argc];

    for (int i = 1; i < argc; i++)
    {
        numbers[i] = atoi(argv[i]);
    }


    for (int i = 1; i < argc; i++)
    {
        pids[i] = fork();

        if (pids[i] == 0)
        {
            usleep(numbers[i]*1000);

            printf("%d ", numbers[i]);

            exit(0);
        }
    }

    for (int i = 1; i < argc; i++)
    {
        wait(NULL);
    }

    printf("\n");
    return 0;
}