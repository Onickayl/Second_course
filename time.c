#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>

int main(int argc, char* argv[])
{

    struct timeval t1;
    gettimeofday(&t1, NULL);


    pid_t p = fork();


    if(p == 0)
    {
        execvp(argv[1], &argv[1]);
        perror("exec faild");
        exit(0);
    }

    wait(NULL);

    struct timeval t2;
    gettimeofday(&t2, NULL);

    int time = (t2.tv_sec/1000 + t2.tv_usec * 1000) - (t1.tv_sec / 1000 + t1.tv_usec * 1000);
      
    printf("time in ms = %d", time);

    printf("\n\n");

    return 0;
} 

/*
отметка времени

дочерний процесс

в нём exec

в род wait

вторая отметка
 */