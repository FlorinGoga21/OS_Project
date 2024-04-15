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

void saveToFile(int fd, const char *dir, const char *data, int isPath) {
    char fullPath[300];
    if (isPath) {
        sprintf(fullPath, "%s/%s\n", dir, data);
    } else {
        sprintf(fullPath, "%s\n", data);
    }
    write(fd, fullPath, strlen(fullPath)); 
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

    int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
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
                saveToFile(fd, path, d2->d_name, 1); 
            }

            closedir(dir);
        }

        printf("Path: %s\n", path);
        saveToFile(fd, argv[1], d->d_name, 0); 
    }

    closedir(director);
    close(fd);

    exit(EXIT_SUCCESS);
}
