#include <stdio.h>
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

    while ((d = readdir(director)) != NULL) {

        printf("%s\n", d->d_name);

        char path[300];
        sprintf(path, "%s/%s", argv[1], d->d_name);

       /* if(isDirectory(path)){
            struct dirent *d2;
            DIR *dir = opendir(path);
            while((d2 = readdir(dir)) != NULL){
                 printf("%s\n", d2->d_name);
            }
            closedir(dir);
        }*/ /*make it not go out of the folder and its ok*/ 
        //commit test 

        printf("Path: %s\n", path);

    }

    closedir(director);

    exit(EXIT_SUCCESS);
}