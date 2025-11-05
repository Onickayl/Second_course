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

int main(int argc, char *argv[]) 
{
    // создаём буффер для чтения
    char cmd[4096];
    ssize_t rd;

// читаем из stdin
    rd = read(STDIN_FILENO, cmd, sizeof(cmd));

    if (rd <= 0)
    {
        return 0;
    }

    // довляем в конец нуль, чтоб получилась строка
    cmd[rd-1] = '\0';
    //printf("cmd = %s\n", cmd);


// считаем количество разделителей == кол-во команд - 1
    int num_cmd = 1;

    for(int i = 0; i < rd; i++)
    {
        if(cmd[i] == '|')
        {
            num_cmd += 1;
        }
    }

// вводим массив строк с командами. 1 строка - 1 команда
    char *strings[num_cmd+1];

    // Работаем с копией указателя
    char* str =  strdup(cmd);
    char* token = strtok(str, "|");

    int i = 0;
    
    while (token != NULL && i < num_cmd) 
    {
        strings[i] = token; // команды
        i++;
        token = strtok(NULL, "|");        
    }

    strings[i] = NULL; // Конец массива


// create pipe
    int fds[num_cmd-1][2];
    for(int i = 0; i < num_cmd-1; i++)
    {
        pipe(fds[i]);
    }
    

// Do all comands
    for (int i = 0; i < num_cmd; i++)
    {

// create children

        pid_t p = fork();

        if(p == 0)
        {// child

            if(i>0)
            {// если не первый, значит перенапрявляем ввод
                dup2(fds[i-1][0], 0);
                close(fds[i-1][0]);
            }

            if(i < num_cmd-1)
            {// если не последний, значит перенапрявляем вывод
                dup2(fds[i][1], 1);
                close(fds[i][1]);
            }

            for(int j = 0; j < num_cmd-1; j++)
            {// закрываем все выходы пайп
                close(fds[j][0]);
                close(fds[j][1]);
            }

// Создаем аргументы для execvp
            char *args[64]; // тут лежат аргументы
            int num_arg = 0;
            
            char *cmd_copy = strdup(strings[i]); // копируем i команду
            char *arg = strtok(cmd_copy, " ");  // выделяем имя команды arg = "echo" и т.п.

            while (arg != NULL && num_arg < 63) 
            {
                args[num_arg] = arg;
                //printf("args[%d] = %s\n", num_arg, arg); 
                num_arg++;
                arg = strtok(NULL, " ");

            }

            args[num_arg] = NULL;
            
            execvp(args[0], args);
            perror("exec failed");

            free(cmd_copy);

            exit(1);

        }
        
        
    }

    for(int j = 0; j < num_cmd-1; j++)
    {// закрываем все выходы пайп
        close(fds[j][0]);
        close(fds[j][1]);
    }

    for(int j = 0; j < num_cmd; j++)
    {// ждём
        wait(NULL);
    }

    free(str);
    return 0;
}

