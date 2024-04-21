#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PATH_LENGTH 300
#define MAX_FILE_LENGTH 1000
#define MAX_DIRECTORIES 10

int isDirectory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

void saveToFile(int fd, const char *dir, const char *data, int isPath) {
    char fullPath[MAX_PATH_LENGTH + MAX_FILE_LENGTH];
    if (isPath) {
        snprintf(fullPath, sizeof(fullPath), "%s/%s\n", dir, data);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s\n", data);
    }
    write(fd, fullPath, strlen(fullPath)); 
}

int hasChanged(const char *filePath, size_t newSize) {
    struct stat st;
    if (stat(filePath, &st) == -1) {
        perror("Error getting file status");
        return 1; 
    }

    if (st.st_size != newSize) {
        return 1;
    }
    return 0;
}

int saveAndCheckChanges(const char *directory) {
    DIR *director = opendir(directory);
    if (director == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    char outputFileName[MAX_PATH_LENGTH + 20]; 
    snprintf(outputFileName, sizeof(outputFileName), "snapshot_%s.txt", directory);

    int fd = open(outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror(outputFileName);
        exit(EXIT_FAILURE);
    }

    struct dirent *d;
    while ((d = readdir(director)) != NULL) {
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
            continue; 
        
        printf("%s\n", d->d_name);
        char path[MAX_PATH_LENGTH];
        snprintf(path, sizeof(path), "%s/%s", directory, d->d_name);

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
        saveToFile(fd, directory, d->d_name, 0); 
    }
    close(fd);

    int changed = hasChanged(outputFileName, strlen(directory) + 1); 
    if (changed)
        printf("Yes\n\n");
    else
        printf("No\n\n");

    return changed;
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > MAX_DIRECTORIES + 1) {
        fprintf(stderr, "Usage: %s <directory1> [<directory2> ... <directoryN>]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        saveAndCheckChanges(argv[i]);
    }

    return EXIT_SUCCESS;
}
