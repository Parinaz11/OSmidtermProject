#include <stdio.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>
#include <stdint.h>

#define SHM_KEY 1234
#define MAX_FILE_TYPES 100

// Structure for shared memory
struct SharedMemory {
    int totalFiles;
    int fileTypeCount;
    long maxFileSize;
    char maxFilePath[200];
    long minFileSize;
    char minFilePath[200];
    struct FileType {
        char extension[20];
        int count;
    } fileTypes[MAX_FILE_TYPES];
};

pthread_mutex_t mutex_totalFiles = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_maxFileSize = PTHREAD_MUTEX_INITIALIZER;

int shmid;

void* count_files(void* arg);

void* count_files(void* arg) {

    struct SharedMemory* sharedMemory = (struct SharedMemory*)shmat(shmid, NULL, 0);
    if ((intptr_t)sharedMemory == -1) {
        perror("shmat in child");
        exit(EXIT_FAILURE);
    }

    const char* directory = (const char*)arg;

    DIR* dir;
    struct dirent* entry;
    struct stat file_stat;

    dir = opendir(directory);
    if (dir == NULL) {
        perror("opendir");
        pthread_exit(NULL);
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        if (stat(full_path, &file_stat) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISREG(file_stat.st_mode)) {
            pthread_mutex_lock(&mutex_totalFiles);
            pthread_mutex_lock(&mutex_maxFileSize);

            sharedMemory->totalFiles++;

            // Get file extension
            const char* extension = strrchr(entry->d_name, '.');
            if (extension != NULL) {
                // Increment count for this file type
                for (int i = 0; i < sharedMemory->fileTypeCount; ++i) {
                    if (strcmp(sharedMemory->fileTypes[i].extension, extension) == 0) {
                        sharedMemory->fileTypes[i].count++;
                        break;
                    }
                }
                // If the extension is not found, add it to the list
                if (extension != NULL && sharedMemory->fileTypeCount < MAX_FILE_TYPES) {
                    strncpy(sharedMemory->fileTypes[sharedMemory->fileTypeCount].extension, extension, sizeof(sharedMemory->fileTypes[sharedMemory->fileTypeCount].extension));
                    sharedMemory->fileTypes[sharedMemory->fileTypeCount].count = 1;
                    sharedMemory->fileTypeCount++;
                }
            }

            if (file_stat.st_size > sharedMemory->maxFileSize) {
                sharedMemory->maxFileSize = file_stat.st_size;
                strcpy(sharedMemory->maxFilePath, full_path);
            }
            else if (file_stat.st_size < sharedMemory->minFileSize) {
                sharedMemory->minFileSize = file_stat.st_size;
                strcpy(sharedMemory->minFilePath, full_path);
            }

            pthread_mutex_unlock(&mutex_maxFileSize);
            pthread_mutex_unlock(&mutex_totalFiles);
        } else if (S_ISDIR(file_stat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            // Create a thread for each subdirectory
            pthread_t tid;
            pthread_create(&tid, NULL, count_files, (void*)full_path);
            pthread_join(tid, NULL);
        }
    }

    closedir(dir);
    // Detach the shared memory segment
    shmdt(sharedMemory);
    pthread_exit(NULL);
}

void count_parentFiles(const char* directory, int shmid){
    struct SharedMemory* sharedMemory = (struct SharedMemory*)shmat(shmid, NULL, 0);
    if ((intptr_t)sharedMemory == -1) {
        perror("shmat in parent");
        exit(EXIT_FAILURE);
    }


    DIR* dir;
    struct dirent* entry;
    struct stat file_stat;

    dir = opendir(directory);
    if (dir == NULL) {
        perror("opendir");
        pthread_exit(NULL);
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        if (stat(full_path, &file_stat) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISREG(file_stat.st_mode)) {
            pthread_mutex_lock(&mutex_totalFiles);
            pthread_mutex_lock(&mutex_maxFileSize);

            sharedMemory->totalFiles++;

            // Get file extension
            const char* extension = strrchr(entry->d_name, '.');
            if (extension != NULL) {
                // Increment count for this file type
                for (int i = 0; i < sharedMemory->fileTypeCount; ++i) {
                    if (strcmp(sharedMemory->fileTypes[i].extension, extension) == 0) {
                        sharedMemory->fileTypes[i].count++;
                        break;
                    }
                }
                // If the extension is not found, add it to the list
                if (extension != NULL && sharedMemory->fileTypeCount < MAX_FILE_TYPES) {
                    strncpy(sharedMemory->fileTypes[sharedMemory->fileTypeCount].extension, extension, sizeof(sharedMemory->fileTypes[sharedMemory->fileTypeCount].extension));
                    sharedMemory->fileTypes[sharedMemory->fileTypeCount].count = 1;
                    sharedMemory->fileTypeCount++;
                }
            }

            if (file_stat.st_size > sharedMemory->maxFileSize) {
                sharedMemory->maxFileSize = file_stat.st_size;
                strcpy(sharedMemory->maxFilePath, full_path);
            }
            else if (file_stat.st_size < sharedMemory->minFileSize) {
                sharedMemory->minFileSize = file_stat.st_size;
                strcpy(sharedMemory->minFilePath, full_path);
            }

            pthread_mutex_unlock(&mutex_maxFileSize);
            pthread_mutex_unlock(&mutex_totalFiles);
        }
    }
    closedir(dir);
    // Detach the shared memory segment
    shmdt(sharedMemory);
}


void process_subdirectories(const char* main_directory, int shmid) {
    DIR* dir;
    struct dirent* entry;

    dir = opendir(main_directory);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }


    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            // Create a child process for each first-level subdirectory
            pid_t pid = fork();

            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // Child process
                count_parentFiles(main_directory,shmid);
                struct SharedMemory* sharedMemory = (struct SharedMemory*)shmat(shmid, NULL, 0);
                if ((intptr_t)sharedMemory == -1) {
                    perror("shmat in child");
                    exit(EXIT_FAILURE);
                }

                char full_path[PATH_MAX];
                snprintf(full_path, sizeof(full_path), "%s/%s", main_directory, entry->d_name);
                printf("Processing subdirectory: %s\n", full_path);
                pthread_t tid;
                pthread_create(&tid, NULL, count_files, (void*)full_path);
                pthread_join(tid, NULL);


                // Detach the shared memory segment
                shmdt(sharedMemory);

                exit(EXIT_SUCCESS);
            } else {
                //Shared memory in the main directory (parent process)
                struct SharedMemory* sharedMemory = (struct SharedMemory*)shmat(shmid, NULL, 0);
                if ((intptr_t)sharedMemory == -1) {
                    perror("shmat in parent");
                    exit(EXIT_FAILURE);
                }
                // Parent process continues here
                int status;
                waitpid(pid, &status, 0);
                count_parentFiles(main_directory,shmid);

                 // Detach the shared memory segment
                 shmdt(sharedMemory);
            }
        }
    }

    closedir(dir);
}

int main() {

    const char* main_directory = "/home/arabdi/Documents";

    key_t key = ftok("shmfile", SHM_KEY);
    // Create a shared memory segment
    shmid = shmget(key, sizeof(struct SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach the shared memory segment to the address space
    struct SharedMemory* sharedMemory = (struct SharedMemory*)shmat(shmid, NULL, 0);

    if ((int)sharedMemory == -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    // Initialize the shared variable
    sharedMemory->totalFiles = 0;
    sharedMemory->maxFileSize = 0;
    strcpy(sharedMemory->maxFilePath, "EMPTY_max");
    sharedMemory->minFileSize = INFINITY;
    strcpy(sharedMemory->minFilePath, "EMPTY_min");

    process_subdirectories(main_directory, shmid);
    
    int totalFiles=sharedMemory->totalFiles/2;

    printf("The total number of files in the main directory and its subdirectories is: %d\n", totalFiles);
    printf("Maximum file size: %ld bytes.\n", sharedMemory->maxFileSize);
    printf("Maximum file path: %s\n", sharedMemory->maxFilePath);
    printf("Minimum file size: %ld bytes.\n", sharedMemory->minFileSize);
    printf("Minimum file path: %s\n", sharedMemory->minFilePath);
    printf("File types:\n");
    for (int i = 0; i < sharedMemory->fileTypeCount; ++i) {
        printf("%s: %d\n", sharedMemory->fileTypes[i].extension, sharedMemory->fileTypes[i].count);
    }

    // Detach the shared memory segment
    shmdt(sharedMemory);

    // Remove the shared memory segment
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}