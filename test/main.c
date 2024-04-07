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

void saveToFile(FILE *file, const char *data) {
    fprintf(file, "%s\n", data); 
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    DIR *director = opendir(argv[1]);
    if (director == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen("output.txt", "w"); 
    if (file == NULL) {
        perror("output.txt");
        exit(EXIT_FAILURE);
    }

    struct dirent *d;
    while ((d = readdir(director)) != NULL) {
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
            continue; 
        
        printf("%s\n", d->d_name);
        char path[300];
        sprintf(path, "%s/%s", argv[1], d->d_name);

        if (isDirectory(path)) {
            struct dirent *d2;
            DIR *dir = opendir(path);
            if (dir == NULL) {
                perror("opendir");
                continue; 
            }

            while ((d2 = readdir(dir)) != NULL) {
                printf("Path: %s\n", path);
                printf("%s\n", d2->d_name);
                saveToFile(file, d2->d_name); 
            }

            closedir(dir);
        }

        printf("Path: %s\n", path);
        saveToFile(file, d->d_name); 
    }

    closedir(director);
    fclose(file); 

    exit(EXIT_SUCCESS);
}
