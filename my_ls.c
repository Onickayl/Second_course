#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>



int main()
{
    DIR* dir = opendir(".");


    struct dirent *e;

    while((e = readdir(dir)) != NULL)
    {
        printf("name - %s ", e->d_name);
        printf("i_node - %ld\n", e->d_ino);
    }

    closedir(dir);
}