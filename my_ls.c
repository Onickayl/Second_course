#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>


int without_arg(DIR* dir);    //good
int i_case(DIR* dir);         //good
int l_case(DIR* dir);         //good
int n_case(DIR* dir);         //good
void octal_to_rwx_simple(mode_t mode, char *buffer);
int a_case(DIR* dir);         //good
int d_case(DIR* dir);         //good
int R_case(const char *current_path, int depth);


/*
struct dirent {
               ino_t          d_ino;       /* Inode number 
               off_t          d_off;       /* Not an offset; see below 
               unsigned short d_reclen;    /* Length of this record 
               unsigned char  d_type;      /* Type of file; not supported by all filesystem types 
               char           d_name[256]; /* Null-terminated filename 
           }
*/
struct stat st;

int main(int argc, char *argv[])
{

    DIR* dir = opendir(".");
    struct dirent *e;
    int depth = 0;
    const char *path = ".";

    const char *opts = "linRad"; // доступные опции
    int opt; // каждая следующая опция попадает сюда
 
    if (argc == 1)
    { 
        without_arg(dir);
        return 0;
    }
    else
    {
        while((opt = getopt(argc, argv, opts)) != -1) 
        {            
            switch(opt) 
            {
                case 'l': // если опция -l, то выводим развёрнутую инфу про файлы
                    l_case(dir);
                    break;
                case 'i': // если опция -i, то выводим ещё и i_node
                    i_case(dir);
                    break;
                case 'n': // если опция -n, то что-то там в численном виде
                    n_case(dir);
                    break;
                case 'R': // если опция -R, то рекурсивно обходим, depth это глубина или уровень
                    R_case(path, 0);
                    break;
                case 'a': // если опция -a, то все-все файлы и даже с точкой
                    a_case(dir);
                    break;
                case 'd': // если опция -d, то выводим только директории
                    d_case(dir);
                    break;
            }
        }

    }

    return 0;
}


int without_arg(DIR* dir)
{
    struct dirent *e;

    while((e = readdir(dir)) != NULL)
    {
        if (strcmp(e->d_name, ".") != 0 && strcmp(e->d_name, "..") != 0)
        {
            printf("%s  ", e->d_name);
        }
        
    }
    
    printf("\n");

    closedir(dir);
    return 0;
}


int l_case(DIR* dir)
{
    struct dirent *e;

    while((e = readdir(dir)) != NULL)
    {
        if (strcmp(e->d_name, ".") != 0 && strcmp(e->d_name, "..") != 0)
        {
            stat(e->d_name, &st);
            struct tm *now = localtime(&st.st_ctime);
            const char* months[12] = {"янв", "фев",  "мар", "апр", "май", "июн", "июл", "авг", "сен", "окт", "ноя", "дек"};
            struct passwd *pw = getpwuid(st.st_uid);
            struct group *gid = getgrgid(pw->pw_gid);
            char permissions[10];
            octal_to_rwx_simple(st.st_mode, permissions);
// права доступа (ДА)-> колво ссылок на файл (ДА)-> имя владельца (ДА)-> назв группы -> размер файла в байт (ДА)-> мес (ДА)-> число (ДА)-> чч:мм (ДА)-> имя файла (ДА)\n
            printf("%-10s %1ld %-1s %-4s %5ld %3s %2d %02d:%02d %s\n", permissions, st.st_nlink, pw->pw_name, gid->gr_name, st.st_size, months[now->tm_mon], now->tm_mday, now->tm_hour, now->tm_min, e->d_name);
        }
    }

    closedir(dir);
    return 0;
}

int n_case(DIR* dir)
{
    struct dirent *e;

    while((e = readdir(dir)) != NULL)
    {
        if (strcmp(e->d_name, ".") != 0 && strcmp(e->d_name, "..") != 0)
        {
            stat(e->d_name, &st);
            struct tm *now = localtime(&st.st_ctime);
            const char* months[12] = {"янв", "фев",  "мар", "апр", "май", "июн", "июл", "авг", "сен", "окт", "ноя", "дек"};
            struct passwd *pw = getpwuid(st.st_uid);
            char permissions[10];
            octal_to_rwx_simple(st.st_mode, permissions);
// права доступа (ДА)-> колво ссылок на файл (ДА)-> имя владельца (ДА)-> назв группы -> размер файла в байт (ДА)-> мес (ДА)-> число (ДА)-> чч:мм (ДА)-> имя файла (ДА)\n
            printf("%-10s %1ld %1d %4d %5ld %3s %2d %02d:%02d %s\n", permissions, st.st_nlink, st.st_uid, pw->pw_gid, st.st_size, months[now->tm_mon], now->tm_mday, now->tm_hour, now->tm_min, e->d_name);
        }
    }

    closedir(dir);
    return 0;
}

void octal_to_rwx_simple(mode_t mode, char *buffer) 
{
    const char *patterns[8] = {"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};
    
    int octal = mode & 0777;
    memcpy(&buffer[0], patterns[(octal >> 6) & 7], 3);
    memcpy(&buffer[3], patterns[(octal >> 3) & 7], 3);
    memcpy(&buffer[6], patterns[octal & 7], 3);
    buffer[9] = '\0';
}

int i_case(DIR* dir)
{
    struct dirent *e;

    while((e = readdir(dir)) != NULL)
    {
        if (strcmp(e->d_name, ".") != 0 && strcmp(e->d_name, "..") != 0)
        {
            printf("%ld %s  ", e->d_ino, e->d_name);
        }
    
    }

    printf("\n");

    closedir(dir);
    return 0;
}


int a_case(DIR* dir)
{
    struct dirent *e;
    
    while((e = readdir(dir)) != NULL)
    {
        printf("%s  ", e->d_name);        
    }
    
    printf("\n");

    closedir(dir);
    return 0;
}


int d_case(DIR* dir)
{
    struct dirent *e;

    while((e = readdir(dir)) != NULL)
    {
        stat(e->d_name, &st);

        if (S_ISDIR(st.st_mode)) 
        {
            printf("%s ", e->d_name);
        }
    
    }

    printf("\n");

    closedir(dir);
    return 0;
}


int R_case(const char *current_path, int depth)
{
    DIR *dir = opendir(current_path);

    if (dir == NULL) 
    {
        return -1;
    }

    struct dirent *e;

    char full_path[1024];
    int first_item = 1; // Для красивого вывода в строку

    // заголовок директории
    if (depth > 0) 
    {
        printf("\n"); // Пустая строка между директориями
    }
    
    printf("%s:\n", current_path);

    // первый проход: выводим только обычные файлы и директории (не начинающиеся с точки, кроме . и ..)
    while ((e = readdir(dir)) != NULL) 
    {
        // Пропускаем все скрытые файлы
        if (e->d_name[0] == '.') 
        {
            continue; // пропускаем всё, что начинается с точки
        }

        // Формируем полный путь
        if (strcmp(current_path, ".") == 0) 
        {
            strcpy(full_path, e->d_name);
        } 
        else 
        {
            // склеиваем путь
            strcpy(full_path, current_path);
            int len = strlen(full_path);

            if (full_path[len-1] != '/') 
            {
                strcat(full_path, "/");
            }

            strcat(full_path, e->d_name);
        }

        // Выводим с отступами
        if (first_item == 0) 
        {
            printf("  ");
        }

        first_item = 0;

        printf("%s", e->d_name);

        // Добавляем / для директорий
        struct stat st;

        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) 
        {
            printf("/");
        }
    }

    // Если были элементы - новая строка
    if (first_item == 0) 
    {
        printf("\n");
    }

    // второй проход: рекурсия для поддиректорий

    rewinddir(dir); // возвращаемся в начало директории

    while ((e = readdir(dir)) != NULL) 
    {
        // пропускаем скрытые 
        if (e->d_name[0] == '.') 
        {
            continue;
        }

        // Формируем полный путь
        if (strcmp(current_path, ".") == 0) 
        {
            strcpy(full_path, e->d_name);
        } 
        else 
        {
            strcpy(full_path, current_path);
            int len = strlen(full_path);

            if (full_path[len-1] != '/') 
            {
                strcat(full_path, "/");
            }

            strcat(full_path, e->d_name);
        }

        // Рекурсия только для директорий
        struct stat st;

        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) 
        {
            R_case(full_path, depth + 1);
        }
    }

    closedir(dir);
    return 0;
}
