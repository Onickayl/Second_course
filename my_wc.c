#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>

int copying(int source, int dest);
int count_lines(char* buf, size_t size);
int count_words(char* buf, size_t size);

int main(int argc, char *argv[])
{
    if (argc == 1)
    { // no argv
        printf("write something\n");
        return 0;
    }

    // pipe()
    int fds[2];
    int a = pipe(fds);

    // fork()
    pid_t p = fork();

    if (p == 0) // child
    { // read from "do it", write to pipe

        close(fds[0]); // закрываем ребёнка на чтение

        // dup2 + close
        dup2(fds[1], 1);
        close(fds[1]);

        // execvp
        execvp(argv[1], &argv[1]); // какая-то полезная прога "do it"
        perror("exec faild");
        exit(0);
    }

    else // parent
    { // read from pipe, write to stdout

        // close
        close(fds[1]); // закрываем родителя на запись

        copying(fds[0], STDOUT_FILENO);

        close(fds[0]);    
        wait(NULL);

    }

    return 0;
}

// functions
int copying(int source, int dest)
{
    int count[3] = {0, 0, 0};

    char buf[4096];
    int written = 0;

    ssize_t rd;
    ssize_t wr;

    while (1)
    {
        rd = read(source, buf, sizeof(buf));

        if (rd <= 0)
        {
            break;
        }

        while (rd - written > 0)
        {
            wr = write(dest, buf + written, rd - written);

            count[0] = wr;
            count[1] = count_words(buf, rd);
            count[2] = count_lines(buf, rd);
            printf("Number of bytes = %d words = %d lines = %d\n", count[0], count[1], count[2]);

            written = wr;

            if (wr < 0)
            {
                perror("error writing");
                return -1;
            }

            rd = rd - written;
        }
    }

    if (rd < 0)
    {
        perror("error reading");
        return -1;
    }
    else if (rd == 0)
    {
        return -1;
    }

    return 0;
}

int count_words(char* buf, size_t size)
{
    int num_words = 0;

    for(int i = 0; i < size; i++)
    {
        if(isspace(buf[i]))
        {
            num_words += 1;
        }
    }

    return num_words;
    
}

int count_lines(char* buf, size_t size)
{
    int num_lines = 0;

    for(int i = 0; i < size; i++)
    {
        if(buf[i] == '\n')
        {
            num_lines += 1;
        }
    }

    return num_lines;
    
}
