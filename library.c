#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int countFolders = 0;
int somethingRandom;

void* thread_function(void* arg) {

    const char* path = (const char*)arg;
    // What each thread should do

    // Lock the mutex before printing
    pthread_mutex_lock(&mutex);

    // Print to the console
    printf("Hello from thread! ");
//    countFolders++;
    DIR *dir;
    struct dirent *entry;

    // Open the directory
    printf("Path = %s\n", path);
    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    // Iterate over entries in the directory
    entry = readdir(dir);
    while (entry != NULL) {
        // Check if the entry is a regular file
        if (entry->d_type == DT_REG && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            countFolders++;
        }
        entry = readdir(dir);
    }

    // Close the directory
    closedir(dir);

    // Unlock the mutex after printing
    printf("HERE: %d\n", countFolders);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void process_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    entry = readdir(dir);
    while (entry != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char sub_path[PATH_MAX];
            sprintf(sub_path, "%s/%s", path, entry->d_name);

            pid_t child_pid = fork();
            if (child_pid == -1) {
                perror("fork failure");
                exit(EXIT_FAILURE);
            }
            else if (child_pid == 0) {
                // Child process (newFolder1 and newFolder2)
                printf("Child process for directory: %s\n", sub_path);

                // Create a thread for each subdirectory in this process

                struct dirent *subEntry;
                DIR *dir2 = opendir(sub_path);
                subEntry = readdir(dir2);
                int thread_count = 0;
                // Create an array to store thread IDs
                pthread_t thread_ids[100];
                // Create a new thread
                char threadPath[100];
                while (subEntry != NULL){
                    // Each of these directories has a thread

                    if (subEntry->d_type == DT_DIR && strcmp(subEntry->d_name, ".") != 0 && strcmp(subEntry->d_name, "..") != 0){

                        sprintf(threadPath, "%s/%s", sub_path, subEntry->d_name);
                        pthread_create(&thread_ids[thread_count], NULL, thread_function, (void*) threadPath);
                        thread_count++;
                    }
                    subEntry = readdir(dir2);
                }
                // Wait for all the created threads to finish
                for (int i = 0; i < thread_count; i++) {
                    pthread_join(thread_ids[i], NULL);
                }
                // Close the directory
                closedir(dir2);

            }

            // Exit the child process
            exit(EXIT_SUCCESS);
            }
            else {
                // Parent process
                // Wait for the child process to finish
                waitpid(child_pid, NULL, 0);
            }

        entry = readdir(dir);
    }

    closedir(dir);
}

int main() {
    const char *root_directory = "/home/parinaz/Documents";
    process_directory(root_directory);
    printf("The number of folders is %d.", countFolders);
    return 0;
}


//-------------------------------------------------------------------------------------------


//
//#include <stdio.h>
//#include <stdlib.h>
//#include <dirent.h>
//
//int countFilesInDirectory(const char *path) {
//    DIR *dir;
//    struct dirent *entry;
//    int fileCount = 0;
//
//    // Open the directory
//    dir = opendir(path);
//    if (dir == NULL) {
//        perror("opendir");
//        exit(EXIT_FAILURE);
//    }
//
//    // Iterate over entries in the directory
//    while ((entry = readdir(dir)) != NULL) {
//        // Check if the entry is a regular file
//        if (entry->d_type == DT_REG) {
//            fileCount++;
//        }
//    }
//
//    // Close the directory
//    closedir(dir);
//
//    return fileCount;
//}
//
//int main() {
//    const char *directoryPath = "/home/parinaz/Documents/newFolder2/newFolder2_1"; // Replace with the path to your directory
//    int numberOfFiles = countFilesInDirectory(directoryPath);
//
//    printf("Number of files in the directory: %d\n", numberOfFiles);
//
//    return 0;
//}




//
//#include <stdio.h>
//#include <pthread.h>
//
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//
//void* thread_function(void* arg) {
//    // Some thread-specific logic
//
//    // Lock the mutex before printing
//    pthread_mutex_lock(&mutex);
//
//    // Print to the console
//    printf("Hello from thread!\n");
//
//    // Unlock the mutex after printing
//    pthread_mutex_unlock(&mutex);
//
//    return NULL;
//}
//
//int main() {
//    pthread_t thread_id;
//
//    // Create a new thread
//    pthread_create(&thread_id, NULL, thread_function, NULL);
//
//    // Main thread logic
//
//    // Lock the mutex before printing
//    pthread_mutex_lock(&mutex);
//
//    // Print to the console
//    printf("Hello from main thread!\n");
//
//    // Unlock the mutex after printing
//    pthread_mutex_unlock(&mutex);
//
//    // Wait for the created thread to finish
//    pthread_join(thread_id, NULL);
//
//    return 0;
//}

// *****************************************************************************************

//
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <dirent.h>
//#include <pthread.h>
//#include <string.h>
//
//#include <sys/types.h>
//#include <sys/stat.h>
//#define MAX_PATH_LENGTH 1024
//#define MAX_FILE_TYPES 100
//#define MAX_FILE_TYPE_LENGTH 256
//
//typedef struct {
//    int file_count;
//    int type_count;
//    off_t max_size;
//    char max_size_path[MAX_PATH_LENGTH];
//    off_t min_size;
//    char min_size_path[MAX_PATH_LENGTH];
//    off_t total_size;
//    int file_type_counts[MAX_FILE_TYPES];
//    char file_type_names[MAX_FILE_TYPES][MAX_FILE_TYPE_LENGTH];
//    pthread_mutex_t mutex;
//} SharedData;
//
//typedef struct {
//    char path[MAX_PATH_LENGTH];
//    SharedData* shared_data;
//} WorkerArgs;
//
//void* process_directory(void* arg);
//void* thread_worker(void* arg);
//void update_statistics(const char* path, off_t size, SharedData* shared_data);
//
//void initialize_shared_data(SharedData* shared_data) {
//    memset(shared_data, 0, sizeof(SharedData));
//    pthread_mutex_init(&shared_data->mutex, NULL);
//}
//
//int main() {
//
////    fflush(stdout);
//
//    char root_directory[MAX_PATH_LENGTH] = "/home/parinaz/Documents";
//
//    SharedData shared_data;
//    initialize_shared_data(&shared_data);
//
//    pthread_t thread_id;
//    WorkerArgs main_worker_args;
//
//    strcpy(main_worker_args.path, root_directory);
//    main_worker_args.shared_data = &shared_data;
//    printf("HERE\n");
//    int retval = pthread_create(&thread_id, NULL, process_directory, &main_worker_args);
//    if (retval != 0) {
//        printf("Failed to create main thread\n");
//        fflush(stdout);
//        return EXIT_FAILURE;
//    }
//
//    pthread_join(thread_id, NULL);
//
//    printf("Number of files: %d\n", shared_data.file_count);
//    printf("Number of different types of files: %d\n", shared_data.type_count);
//    printf("Address of the file with maximum size: %s, Size: %lu\n", shared_data.max_size_path, shared_data.max_size);
//    printf("Address of the file with minimum size: %s, Size: %lu\n", shared_data.min_size_path, shared_data.min_size);
//    printf("Final size of the root folder: %lu\n", shared_data.total_size);
//    fflush(stdout);
//
//    return 0;
//}
//
//void* process_directory(void* arg) {
//    printf("HERE2");
//    fflush(stdout);
//    WorkerArgs* worker_args = (WorkerArgs*)arg;
//
//    char* path = worker_args->path;
//    SharedData* shared_data = worker_args->shared_data;
//
//    DIR* dir = opendir(path);
//    if (!dir) {
//        perror("Error opening directory");
//        printf("Failed to open directory: %s\n", path);
//        fflush(stdout);
//        pthread_exit(NULL);
//    }
//
//    struct dirent* entry;
//    pthread_t thread_ids[MAX_FILE_TYPES];
//    int thread_count = 0;
//
//    while ((entry = readdir(dir)) != NULL) {
//        char sub_path[MAX_PATH_LENGTH];
//        sprintf(sub_path, "%s/%s", path, entry->d_name);
//
//        if (entry->d_type == DT_REG || (entry->d_type == DT_DIR
//                                        && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)) {
//            WorkerArgs new_worker_args;
//            strcpy(new_worker_args.path, sub_path);
//            new_worker_args.shared_data = shared_data;
//
//            pthread_create(&thread_ids[thread_count], NULL, thread_worker, &new_worker_args);
//            thread_count++;
//        }
//    }
//
//    for (int i = 0; i < thread_count; ++i) {
//        pthread_join(thread_ids[i], NULL);
//    }
//
//    closedir(dir);
//    pthread_exit(NULL);
//}
//
//void* thread_worker(void* arg) {
//    WorkerArgs* worker_args = (WorkerArgs*) arg;
//    char* path = worker_args->path;
//    SharedData* shared_data = worker_args->shared_data;
//    struct stat st;
//
//    pthread_mutex_lock(&shared_data->mutex);
//
//    if (stat(path, &st) == 0) {
//        update_statistics(path, st.st_size, shared_data);
//    } else {
//        perror("Error getting file stats");
//    }
//
//    pthread_mutex_unlock(&shared_data->mutex);
//
//    pthread_exit(NULL);
//}
//
//void update_statistics(const char* path, off_t size, SharedData* shared_data) {
//    pthread_mutex_lock(&shared_data->mutex);
//
//    shared_data->file_count++;
//    shared_data->total_size += size;
//
//    if (size > shared_data->max_size) {
//        shared_data->max_size = size;
//        strcpy(shared_data->max_size_path, path);
//    }
//
//    if (size < shared_data->min_size || shared_data->min_size == 0) {
//        shared_data->min_size = size;
//        strcpy(shared_data->min_size_path, path);
//    }
//
//    const char *file_extension = strrchr(path, '.');
//    if (file_extension) {
//        file_extension++;
//    } else {
//        file_extension = "unknown";
//    }
//
//    int found = 0;
//    for (int i = 0; i < shared_data->type_count; ++i) {
//        if (strcmp(file_extension, shared_data->file_type_names[i]) == 0) {
//            found = 1;
//            break;
//        }
//    }
//
//    if (!found) {
//        strcpy(shared_data->file_type_names[shared_data->type_count], file_extension);
//        shared_data->file_type_counts[shared_data->type_count] = 1;
//        shared_data->type_count++;
//    } else {
//        int type_index = -1;
//        for (int i = 0; i < shared_data->type_count; ++i) {
//            if (strcmp(file_extension, shared_data->file_type_names[i]) == 0) {
//                type_index = i;
//                break;
//            }
//        }
//
//        if (type_index != -1) {
//            shared_data->file_type_counts[type_index]++;
//        }
//    }
//
//    pthread_mutex_unlock(&shared_data->mutex);
//}
//
//
//
//
//
//
//
//