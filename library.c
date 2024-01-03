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

#define SHM_KEY 1234

// Structure for shared memory
struct SharedMemory {
    int totalFiles;
    int fileTypeCount;
    long maxFileSize;
    char maxFilePath[200];
    long minFileSize;
    char minFilePath[200];
};

pthread_mutex_t mutex_totalFiles = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_maxFileSize = PTHREAD_MUTEX_INITIALIZER;

int shmid;

void* count_files(void* arg);

void* count_files(void* arg) {

    struct SharedMemory* sharedMemory = (struct SharedMemory*)shmat(shmid, NULL, 0);
    if ((int)sharedMemory == -1) {
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

                struct SharedMemory* sharedMemory = (struct SharedMemory*)shmat(shmid, NULL, 0);
                if ((int)sharedMemory == -1) {
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
                if ((int)sharedMemory == -1) {
                    perror("shmat in parent");
                    exit(EXIT_FAILURE);
                }
                // Parent process continues here
                int status;
                waitpid(pid, &status, 0);



                 // Detach the shared memory segment
                 shmdt(sharedMemory);
            }
        }
    }

    closedir(dir);
}

int main() {

    const char* main_directory = "/home/parinaz/Documents";

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

    // Creates thread for the main directory
//    pthread_t tid;
//    pthread_create(&tid, NULL, count_files, (void*)main_directory);
//    pthread_join(tid, NULL);

    process_subdirectories(main_directory, shmid);

    printf("The total number of files in the main directory and its subdirectories is: %d\n", sharedMemory->totalFiles);
    printf("Maximum file size: %ld bytes.\n", sharedMemory->maxFileSize);
    printf("Maximum file path: %s\n", sharedMemory->maxFilePath);
    printf("Minimum file size: %ld bytes.\n", sharedMemory->minFileSize);
    printf("Minimum file path: %s\n", sharedMemory->minFilePath);

    // Detach the shared memory segment
    shmdt(sharedMemory);

    // Remove the shared memory segment
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
