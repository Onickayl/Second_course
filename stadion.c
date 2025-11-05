#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

struct message 
{
    long mtype;
    char mtext[100];
};

int judge(int N, int id);
int runner(int i, int id, int N);


int main(int argc, char *argv[])
{
    if (argc == 1)
    { // no argv
        printf("write something\n");
        return 0;
    }

    int N = atoi(argv[1]);
    if(N <= 1)
    {
        printf("Must be more than one runner\n");
        return 0;
    }
    else
    {
        printf("Number of Runners = %d\n\n", N);
    }
    

    key_t key = 1234;  // Произвольный ключ
// create queue
    int id = msgget(key, IPC_CREAT|0666);

// children
    pid_t p[N+1]; // child = J + N runners

    /*пусть p[0] - J, остальные это бегуны*/

    p[0] = fork();

    if(p[0] == 0)
    {// Judge
        judge(N, id);
        exit(0);
    }

    for(int i = 1; i < N+1; i++)
    {
        p[i] = fork();

        if(p[i] == 0)
        { // runners
            runner(i, id, N);
            exit(0);
        }

    }

    for(int i = 0; i < N+1; i++)
    {
        wait(NULL);
    }

    msgctl(id, IPC_RMID, 0);    

    return 0;
}


int judge(int N, int id)
{
    printf("I am judge\n");

    struct message buf;

// принимает соо от всех бегунов о прибытии
    for(int i = 1; i <= N; i++)
    {
        msgrcv(id, &buf, sizeof(buf.mtext), 1, 0);
    }

    printf("\nAll runners arrive\n\n");


// засечь время t1
    struct timeval t1;
    gettimeofday(&t1, NULL);
    printf("fix time 1 = %ld\n\n", t1.tv_usec);

// передать старт runner 1
    buf.mtype = 2;                          // тип 2 - бег
    strcpy(buf.mtext, "Start!");
    msgsnd(id, &buf, sizeof(buf.mtext), 0);
    printf("Send flag to first runner\n");


//получает палочку от последнего бегуна
    msgrcv(id, &buf, sizeof(buf.mtext), 3, 0);
    printf("\nJudge receives flag from N runner\n");

// засечь время t2
    struct timeval t2;
    gettimeofday(&t2, NULL);
    printf("\nfix time 2 = %ld\n", t2.tv_usec);


    long time = t2.tv_usec - t1.tv_usec;
    printf("\nTime = %ld micro sec\n", time);

    return 0;
}



int runner(int i, int id, int N)
{
    printf("I am runner %d\n", i);

    struct message buf;  

    buf.mtype = 1;                                  // тип 1 - приход
    strcpy(buf.mtext, "Runner arrives");
    msgsnd(id, &buf, sizeof(buf.mtext), 0); // передать соо судье, что пришёл

    if (i == 1) 
    {// первый бегун получает палочку от судьи

        msgrcv(id, &buf, sizeof(buf.mtext), 2, 0);
        printf("Runner 1 received flag from judge\n");
        
        // передает второму бегуну палочку
        buf.mtype = 2;
        strcpy(buf.mtext, "give flag to next runner");
        msgsnd(id, &buf, sizeof(buf.mtext), 0);
        printf("Runner 1 gives flag to runner 2\n");
    }
    else if (i < N) 
    {// средние бегуны получают от предыдущего

        msgrcv(id, &buf, sizeof(buf.mtext), 2, 0);
        printf("Runner %d received flag from runner %d\n", i, i-1);
        
        //передают следующему
        buf.mtype = 2;
        strcpy(buf.mtext, "give flag to next runner");
        msgsnd(id, &buf, sizeof(buf.mtext), 0);
        printf("Runner %d gives flag to runner %d\n", i, i+1);
    }
    else 
    {// последний бегун

        msgrcv(id, &buf, sizeof(buf.mtext), 2, 0);
        printf("Runner %d received flag from runner %d\n", i, i-1);
        
        // передает судье
        buf.mtype = 3;
        strcpy(buf.mtext, "Finish");
        msgsnd(id, &buf, sizeof(buf.mtext), 0);
        printf("Runner %d gives flag to judge\n", i);
    }
       
    
    return 0;
}





