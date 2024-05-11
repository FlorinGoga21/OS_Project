#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_PATH_LENGTH 300
#define MAX_FILE_LENGTH 2000
#define MAX_DIRECTORIES 10

void createDirectoryRecursive(const char *path) {
    char *pathCopy = strdup(path); 
    char *token = strtok(pathCopy, "/");

    char dir[MAX_PATH_LENGTH];
    strcpy(dir, token);

    while (token != NULL) {

        if (mkdir(dir, 0700) == -1 && errno != EEXIST) {

            perror("Error creating directory");
            free(pathCopy);
            exit(EXIT_FAILURE);

        }
        token = strtok(NULL, "/");
        
        if (token != NULL) {

            strcat(dir, "/");
            strcat(dir, token);

        }
    }

    free(pathCopy);
}

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
        printf("\nNothing changed\n\n");
    else
        printf("\nSomething changed\n\n");

    return changed;
}

int isCorrupted(const char *filePath) {// 1 = corrupted 0 = safe
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 1; 
    }

    int count = 0;
    char c;
    while (read(fd, &c, 1) > 0) {
        if (!isascii(c)) {
            close(fd);
            return 1; 
        }
        count++;
        if (count > MAX_FILE_LENGTH) {
            close(fd);
            return 1; 
        }
    }

    close(fd);
    return 0; 
}

void checkCorrupted(const char *directory) {
    DIR *dir = opendir(directory);

    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue; 

        char filePath[MAX_PATH_LENGTH];
        snprintf(filePath, sizeof(filePath), "%s/%s", directory, entry->d_name);
        struct stat statbuf;

        if (stat(filePath, &statbuf) == -1) {

            perror("Error getting file status");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            
            checkCorrupted(filePath);

        } else if (S_ISREG(statbuf.st_mode) && strstr(entry->d_name, ".txt") != NULL) {
           
            if (isCorrupted(filePath)) {

                printf("File \"%s\" in directory \"%s\" is corrupted.\n", entry->d_name, directory);

            } else {
                
                printf("File \"%s\" in directory \"%s\" is safe.\n", entry->d_name, directory);

            }
        }
    }

    closedir(dir);
}


void child_process(const char *directory) {

   saveAndCheckChanges(directory);
    exit(EXIT_SUCCESS);

}

void parent_process(char *directories[], int num_directories) {
    pid_t pid;
    int status;
    int num_children = 0;

    for (int i = 0; i < num_directories; i++) {
        pid = fork();
        
        if (pid == -1) {

            perror("fork");
            exit(EXIT_FAILURE);

        } else if (pid == 0) {

            child_process(directories[i]);
            exit(EXIT_SUCCESS);

        } else {
            num_children++;
        }
    }

    for (int i = 0; i < num_children; i++) {

        pid = wait(&status);
        if (WIFEXITED(status)) {
            printf("Child Process %d terminated with PID %d and exit code %d.\n", i+1, pid, WEXITSTATUS(status));
        } else {
            printf("Child Process %d terminated abnormally.\n", i+1);
        }
    }

    printf("\nChecking for corrupted files:\n\n");
    for (int i = 2; i < num_directories; i++) { 

        checkCorrupted(directories[i]);
    }
}


void moveFileToIsolatedSpace(const char *filePath, const char *isolatedSpaceDir) {
    char isolatedFilePath[MAX_PATH_LENGTH];
    snprintf(isolatedFilePath, sizeof(isolatedFilePath), "%s/%s", isolatedSpaceDir, strrchr(filePath, '/') + 1);

    createDirectoryRecursive(isolatedSpaceDir);
    printf("Moving file from: %s\n", filePath);
    printf("Moving file to: %s\n", isolatedFilePath); 

    if (rename(filePath, isolatedFilePath) == -1) {

        perror("Error moving file to isolated space");
        exit(EXIT_FAILURE);
    }
}

void moveDangerousFiles(const char *directory, const char *isolatedSpaceDir) {
    DIR *dir = opendir(directory);

    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        char filePath[MAX_PATH_LENGTH];
        snprintf(filePath, sizeof(filePath), "%s/%s", directory, entry->d_name);
        struct stat statbuf;

        if (stat(filePath, &statbuf) == -1) {
            perror("Error getting file status");
            continue;
        }

        if (S_ISREG(statbuf.st_mode) && strncmp(entry->d_name, "dangerous", 9) == 0 && strstr(entry->d_name, ".txt") != NULL) {
            moveFileToIsolatedSpace(filePath, isolatedSpaceDir);
            printf("File \"%s\" has been isolated.\n", entry->d_name);
        }
    }

    closedir(dir);
}

void isolateDangerousFiles(char *directories[], int numDirectories, const char *isolatedSpaceDir) {

    for (int i = 0; i < numDirectories; i++) {
        moveDangerousFiles(directories[i], isolatedSpaceDir);
    }
}

int main(int argc, char *argv[]) {

    if (argc < 3 || argc > MAX_DIRECTORIES + 2) {
        fprintf(stderr, "Usage: %s -s <isolated_space_dir> <directory1> <directory2> ... <directoryN>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *isolatedSpaceDir;
    char **directories;
    int numDirectories;

    if (strcmp(argv[1], "-s") == 0) {
        isolatedSpaceDir = argv[2];
        directories = argv + 3;
        numDirectories = argc - 3;
    } else {
        fprintf(stderr, "Usage: %s -s <isolated_space_dir> <directory1> <directory2> ... <directoryN>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    isolateDangerousFiles(directories, numDirectories, isolatedSpaceDir);

    parent_process(argv + 1, argc - 1);

    return EXIT_SUCCESS;
}   // input: -s isolated_folder dir1 dir 2 ...
