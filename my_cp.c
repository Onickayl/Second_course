#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>


int copying(int source, int dest);

int main(int argc, char *argv[])
{

    const char *opts = "vif"; // доступные опции, никто не принимает аргументы
    char answer = 'y';
    int opt; // каждая следующая опция попадает сюда
    

    if (argc < 4)
    { // если нет ключей -> не перезаписывает
        printf("Please write: ./name [options] source destination\n");
        return 0;  
    }
    else
    {
        while((opt = getopt(argc, argv, opts)) != -1) 
        {            
            switch(opt) 
            {
                case 'v': // если опция -v, то поясняем, что сделали: 'откуда' -> 'куда'
                    printf("'%s' -> '%s'\n", argv[optind], argv[optind + 1]);
                    break;
                case 'i': // если опция -i, то спрашивает копировать или нет y/n
                    printf("cp: want to copy? (y/n) ");
                    scanf("%c", &answer);
                    if(answer == 'n') 
                    {
                        return 0;                
                    }
                    break;
                case 'f': // если опция -f, то молча перезаписывает, не пишет, что-куда, не уточняет
                    break;
            }
        }
 

        // получение имён файлов
        const char *source_file = argv[optind];

        char *new_file = argv[optind+1];
        strcat(new_file, "new_file.txt"); // добавляем имя файла к пути куда хотим копировать

        /*
        printf("source_file = %s\n", source_file);
        printf("new_file = %s\n", new_file); */

        // открываем файлы
        int source = open(source_file, O_RDONLY);                      // открываем исходный файл на чтение
        int destination = open(new_file, O_WRONLY|O_CREAT|O_TRUNC, 0644);     // создаём новый файл на запись

        /*
        printf("source = %d\n", source);
        printf("destination = %d\n", destination); */


        if (source < 0 || destination < 0)
        {
            printf("error opening file\n");
            return 0;
        }

        if (copying(source, destination) == -1)
        {
            close(source);
            close(destination);
            return 1;
        }  
 

        close(source);
        close(destination);

    }

    return 0;
}

int copying(int source, int dest)
{
    char buf[4096];

    ssize_t rd;

    while (1)
    {
        rd = read(source, buf, sizeof(buf));

        if (rd <= 0) 
        {
            break;
        }

        ssize_t wr = write(dest, buf, rd);

        if (wr < 0)
        {
            perror("error writing");
            return -1;
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

