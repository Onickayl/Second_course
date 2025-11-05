#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

int copying(int source, int dest);

int main(int argc, char *argv[])
{

    ssize_t rd; 

    int fds[2];
    int a = pipe(fds);

    pid_t p = fork();

    if (argc == 1)
    {
        if(p == 0)
        {
            close(fds[1]);
            copying(fds[0], STDOUT_FILENO);
            exit(0);
        }
        else
        {
            close(fds[0]);
            copying(STDIN_FILENO, fds[1]);
            exit(0);
        }
            
    }        
    else
    {
        if(p == 0)
        {
            // read from pipe, write to stdout
            close(fds[1]);
            copying(fds[0], STDOUT_FILENO);
            exit(0);
        }
        else
        {
            //in for(){ open file, write to pipe, close file}
        
            close(fds[0]);
            for (int i = 1; i < argc; i++)
            {
                int ff = open(argv[i], O_RDONLY);

                if (ff < 0)
                {
                    perror("error opening file");
                    continue;
                }

                if (copying(ff, fds[1]) == -1)
                {
                    close(ff);
                    return 1;
                }

                close(ff);
                
            }
        }
    }


    return 0;


}

int copying(int source, int dest)
{

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


        while (rd-written > 0)
        {
            wr = write(dest, buf+written, rd - written);
            
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

