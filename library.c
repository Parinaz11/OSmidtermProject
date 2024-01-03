#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>

pthread_mutex_t mutex_totalFilesChild = PTHREAD_MUTEX_INITIALIZER;

int totalFiles = 0;
pthread_t thread_ids[100];
int thread_count = 0;

void* process_directory(void* arg);

void* process_directory(void* arg) {
    const char* directory = (const char*)arg;

    printf("Thread created for directory: %s\n", directory);

    if (chdir(directory) == -1) {
        perror("chdir");
        pthread_exit(NULL);
    }

    DIR* dir;
    struct dirent* entry;
    struct stat file_stat;

    dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        pthread_exit(NULL);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) {
            char full_path[200];
            snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

            if (stat(full_path, &file_stat) == -1) {
                perror("stat");
                continue;
            }

            printf("File type in directory %s: %s\n", directory,
                   (S_ISREG(file_stat.st_mode) ? "Regular File" :
                    (S_ISDIR(file_stat.st_mode) ? "Directory" :
                     (S_ISLNK(file_stat.st_mode) ? "Symbolic Link" : "Other"))));

            // Increment totalFilesChild inside the critical section
            pthread_mutex_lock(&mutex_totalFilesChild);
            totalFiles++;
            pthread_mutex_unlock(&mutex_totalFilesChild);
        }
        else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            // Create a thread for each subdirectory in this process

            char sub_path[200];
            snprintf(sub_path, sizeof(sub_path), "%s/%s", directory, entry->d_name);
            if (pthread_create(&thread_ids[thread_count++], NULL, process_directory, (void*)sub_path) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }
    }

    closedir(dir);

    pthread_exit(NULL);
}

void discover_and_process_directories(const char* initial_directory) {
    DIR* dir;
    struct dirent* entry;

    dir = opendir(initial_directory);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    int status;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {

            // Create a process for each of these directories
            pid_t pid = fork();

            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // Child process
                char full_path[200];
                snprintf(full_path, sizeof(full_path), "%s/%s", initial_directory, entry->d_name);
                printf("Discovering and processing first-level directory: %s\n", full_path);
                // Creating threads for directories in the second level
                pthread_create(&thread_ids[thread_count++], NULL, process_directory, (void*)full_path);
                exit(EXIT_SUCCESS);
            } else{
                int status2;
                waitpid(pid, &status, 0);
            }
        }
        else if (entry->d_type == DT_REG){
            pthread_mutex_lock(&mutex_totalFilesChild);
            char full_path[200];
            snprintf(full_path, sizeof(full_path), "%s/%s", initial_directory, entry->d_name);
            totalFiles++;
            printf("Discovering first-level file: %s\n", full_path);
            pthread_mutex_unlock(&mutex_totalFilesChild);
        }
    }

    closedir(dir);

    // Wait for all child processes to finish
    while (wait(&status) > 0);

    // Wait for all the created threads to finish
    for (int i = 0; i < thread_count; i++) {
        pthread_join(thread_ids[i], NULL);
    }
}

int main() {
    // Initial directory
    const char* start_directory = "/home/parinaz/Documents";

    if (chdir(start_directory) == -1) {
        perror("chdir");
        exit(EXIT_FAILURE);
    }
    printf("Initial directory: %s\n", start_directory);

    // Discovering and processing first-level directories
    discover_and_process_directories(start_directory);

    // Print the total number of files in the parent process

    // Print the total number of files in the child process
    printf("The total number of files in the child process is %d.\n", totalFiles);

    return 0;
}
