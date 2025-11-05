#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    int n = 4;

    printf("parent pid = %d\n\n", getpid());

    for(int i = 1; i <= n; i++)
    {
        pid_t p = fork();

        if(p==0)
        {
            printf("child pid = %d my parent ppid = %d\n", getpid(), getppid());
            return 0;
        }
    }

    for(int i = 1; i <= n; i++)
    {
        wait(NULL);
        return 0;
    }
}