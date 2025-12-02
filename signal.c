#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

pid_t pid;
volatile sig_atomic_t sig;

void wait_sig();
void handler(int s);
void sender(pid_t p, char **argv);
void reciever();

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Write: ./name str\n");
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = handler;

    sigaction(SIGUSR1, &sa, NULL); // когда придёт сигнал вызовем handler
    sigaction(SIGUSR2, &sa, NULL); // когда придёт сигнал вызовем handler

    pid_t p = fork();

    if (p == 0)
    { // ребёнок
        reciever();
        exit(0);
    }
    else
    { // родитель

        sender(p, argv);
        wait(NULL);
    }

    return 0;
}

void sender(pid_t p, char **argv)
{
    pid = p;    // пид ребёнка
    usleep(10000);  // чтобы ребёнок успел настроить обработчики сигналов

    for (int i = 0; i <= strlen(argv[1]); i++)
    {
        char c = argv[1][i];

        for (int b = 7; b >= 0; b--)
        {
            sig = 0;

            kill(pid, ((c >> b) & 1) ? SIGUSR2 : SIGUSR1);  // отправляем сигнал usr1 - 0, usr2 - 1

            wait_sig();     // ждём сигнал, подтверждение
        }
    }
}

void reciever()
{
    pid = getppid();    // пид родителя
    
    char ch = 0;    // символ
    int bits = 0;   //счётчик битов

    while (1)
    {
        sig = 0;

        wait_sig();     //ждём сигнал usr1 или usr2

        ch = (ch << 1) | (sig == SIGUSR2);
        bits++;

        if (bits == 8)
        {
            if (ch == 0)
            {
                break;
            }

            write(1, &ch, 1);
            ch = bits = 0;
        }

        kill(pid, SIGUSR1);
    }

    write(1, "\n", 1);
}

void handler(int s)
{
    sig = s;
}

void wait_sig()
{
    sigset_t block, old;
    sigemptyset(&block);
    sigemptyset(&old);
    sigaddset(&block, SIGUSR1);
    sigaddset(&block, SIGUSR2);
    sigprocmask(SIG_BLOCK, &block, &old); // заблокировали сигналы usr1 и usr2, старая маска пустая

    while (sig == 0)
    {
        sigsuspend(&old);   // ждём любой сигнал, чтоб проснуться
    }

    sigprocmask(SIG_SETMASK, &old, NULL);   // восстанавливаем старую (пустую) маску
}