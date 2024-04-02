#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <unistd.h>

int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

int main(int argc, char *argv[])
{
    struct dirent *d;

    DIR *director = opendir(argv[1]);

    int flag = 1;

    while ((d = readdir(director)) != NULL) {
        
        if(strcmp(d->d_name,".") == 0 || strcmp(d->d_name,"..") == 0)
        flag = 0;
        else
        flag = 1;

        if(flag != 0){
        printf("%s\n", d->d_name);
        char path[300];
        sprintf(path, "%s/%s", argv[1], d->d_name);
        
        if(isDirectory(path)){
            struct dirent *d2;
            DIR *dir = opendir(path);
            while((d2 = readdir(dir)) != NULL){
                 printf("Path: %s\n", path);
                 printf("%s\n", d2->d_name);
            }
            closedir(dir);
        } 

        printf("Path: %s\n", path);
        
        }
    }

    closedir(director);

    exit(EXIT_SUCCESS);
}