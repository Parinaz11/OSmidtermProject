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
            if (pthread_create(&thread_ids[thread_count], NULL, process_directory, (void*)sub_path) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
            thread_count++;
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
                pthread_create(&thread_ids[thread_count], NULL, process_directory, (void*)full_path);
                thread_count++;
//                process_directory(full_path);
                exit(EXIT_SUCCESS);
            }



        }
        else if (entry->d_type == DT_REG){
            char full_path[200];
            snprintf(full_path, sizeof(full_path), "%s/%s", initial_directory, entry->d_name);
            totalFiles++;
            printf("Discovering first-level file: %s\n", full_path);
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




// ----------------------------afrefeh version up-------------------------------------


//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <dirent.h>
//#include <pthread.h>
//#include <sys/wait.h>
//#include <unistd.h>
//
//typedef struct {
//    char dir[256];
//    int count;
//} Dir;
//
//pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
//
//void *count_files(void *arg) {
//    Dir *dir = (Dir *)arg;
//    DIR *d;
//    struct dirent *dir_entry;
//
//    if ((d = opendir(dir->dir)) == NULL) {
//        fprintf(stderr, "Cannot open directory: %s\n", dir->dir);
//        pthread_exit(NULL);
//    }
//
//    while ((dir_entry = readdir(d)) != NULL) {
//        pthread_mutex_lock(&lock);
//        if (dir_entry->d_type == DT_REG) {
//            dir->count++;
//        }
//        pthread_mutex_unlock(&lock);
//    }
//
//    closedir(d);
//
//    pthread_exit(NULL);
//}
////
////int main() {
////
////    pthread_t threads[2];
////    Dir dirs[2];
////
////    strcpy(dirs[0].dir, "/home/parinaz/Documents/newFolder1");
////    dirs[0].count = 0;
////
////    strcpy(dirs[1].dir, "/home/parinaz/Downloads");
////    dirs[1].count = 0;
////
////    if (pthread_mutex_init(&lock, NULL) != 0) {
////        fprintf(stderr, "Mutex initialization has failed.\n");
////        return 1;
////    }
////
////    for (int i = 0; i < 2; i++) {
////        if (pthread_create(&threads[i], NULL, count_files, &dirs[i]) != 0) {
////            fprintf(stderr, "Error creating thread\n");
////            exit(1);
////        }
////    }
////
////    for (int i = 0; i < 2; i++) {
////        if (pthread_join(threads[i], NULL) != 0) {
////            fprintf(stderr, "Error joining thread\n");
////            exit(1);
////        }
////    }
////
////    pthread_mutex_destroy(&lock);
////
////    printf("Number of files in %s: %d\n", dirs[0].dir, dirs[0].count);
////    printf("Number of files in %s: %d\n", dirs[1].dir, dirs[1].count);
////    int total = 0;
////    for (int i = 0; i < 2; i++){
////        total += dirs[i].count;
////    }
////    printf("Todal number of files is: %d\n", total);
////}
//
//
//// -----------------------------------------The main code----------------------------------------
//
//
////void* thread_function(void* arg) {
////    // What each thread should do
////    const char* path = (const char*)arg;
////
////    // Lock the mutex before printing
////    pthread_mutex_lock(&mutex);
////
////    // Print to the console
////    printf("Hello from thread! \n");
//////    countFolders++;
////    DIR *dir;
////    struct dirent *entry;
////
////    // Open the directory
////    printf("Path = %s\n", path);
////    dir = opendir(path);
////    if (dir == NULL) {
////        perror("opendir");
////        exit(EXIT_FAILURE);
////    }
////
////    // Iterate over entries in the directory
////    entry = readdir(dir);
////    while (entry != NULL) {
////        // Check if the entry is a regular file
////        if (entry->d_type == DT_REG) { //&& strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0
////            countFiles++;
////        }
////        entry = readdir(dir);
////    }
////
////    // Close the directory
////    closedir(dir);
////
////    // Unlock the mutex after printing
////    printf("HERE in %s: %d\n", path,countFiles);
////    pthread_mutex_unlock(&mutex);
////
////    return NULL;
////}
//
//int main() {
//    const char *root_directory = "/home/parinaz/Documents";
//    Dir dirs[200];
//    pthread_t thread_ids[200]; // Create an array to store thread IDs
//
//    stpcpy(dirs[0].dir, root_directory);
//    for (int i = 0; i < 200; i++) dirs[i].count = 0;
//
//    pthread_create(&thread_ids[0], NULL, count_files, &dirs[0]);
//
//    DIR *dir = opendir(dirs[0].dir);
//    if (!dir) {
//        perror("Error opening directory");
//        exit(EXIT_FAILURE);
//    }
//    struct dirent *entry;
//    entry = readdir(dir);
//    int indxdr = 1;
//    while (entry != NULL) {
//        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
//            sprintf(dirs[indxdr++].dir, "%s/%s", root_directory, entry->d_name);
//            int childDirectory_indx = indxdr - 1;
//
//            pid_t child_pid = fork();
//            if (child_pid == -1) {
//                perror("fork failure");
//                exit(EXIT_FAILURE);
//            }
//            else if (child_pid == 0) {
//                // Child process (newFolder1 and newFolder2)
//                printf("Child process directory: %s\n", dirs[childDirectory_indx].dir);
//
//                // Create a thread for each subdirectory in this process
//                struct dirent *subEntry;
//                DIR *dir2 = opendir(dirs[childDirectory_indx].dir);
//                subEntry = readdir(dir2); // For each subdirectory in the children there must be a thread
//
//                while (subEntry != NULL){
//                    // Each of these directories has a thread
//                    if (subEntry->d_type == DT_DIR && strcmp(subEntry->d_name, ".") != 0 && strcmp(subEntry->d_name, "..") != 0){
//                        sprintf(dirs[indxdr].dir, "%s/%s", dirs[indxdr-1].dir, subEntry->d_name);
//                        pthread_create(&thread_ids[indxdr], NULL, count_files, &dirs[indxdr]);
//                        indxdr++;
//                    }
//                    subEntry = readdir(dir2);
//                }
//                // Wait for all the created threads to finish
////                for (int i = 0; i < 200; i++) {
////                    pthread_join(thread_ids[i], NULL);
////                }
//                // Close the directory
//                closedir(dir2);
//            }
//            else {
//                // Parent process
//                // Wait for the child process to finish
//                for (int i = 0; i < 200; i++) {
//                    pthread_join(thread_ids[i], NULL);
//                }
//                waitpid(child_pid, NULL, 0);
////                printf("**In here the count is %d\n", countFiles);
//            }
//            // Exit the child process
//            exit(EXIT_SUCCESS);
//        }
//        entry = readdir(dir);
//    }
//
//    closedir(dir);
//    return 0;
//}
//
//
////-------------------------------------------------------------------------------------------
//
//
////
////#include <stdio.h>
////#include <stdlib.h>
////#include <dirent.h>
////
////int countFilesInDirectory(const char *path) {
////    DIR *dir;
////    struct dirent *entry;
////    int fileCount = 0;
////
////    // Open the directory
////    dir = opendir(path);
////    if (dir == NULL) {
////        perror("opendir");
////        exit(EXIT_FAILURE);
////    }
////
////    // Iterate over entries in the directory
////    while ((entry = readdir(dir)) != NULL) {
////        // Check if the entry is a regular file
////        if (entry->d_type == DT_REG) {
////            fileCount++;
////        }
////    }
////
////    // Close the directory
////    closedir(dir);
////
////    return fileCount;
////}
////
////int main() {
////    const char *directoryPath = "/home/parinaz/Documents/newFolder2/newFolder2_1"; // Replace with the path to your directory
////    int numberOfFiles = countFilesInDirectory(directoryPath);
////
////    printf("Number of files in the directory: %d\n", numberOfFiles);
////
////    return 0;
////}
//
//
//
//
////
////#include <stdio.h>
////#include <pthread.h>
////
////pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
////
////void* thread_function(void* arg) {
////    // Some thread-specific logic
////
////    // Lock the mutex before printing
////    pthread_mutex_lock(&mutex);
////
////    // Print to the console
////    printf("Hello from thread!\n");
////
////    // Unlock the mutex after printing
////    pthread_mutex_unlock(&mutex);
////
////    return NULL;
////}
////
////int main() {
////    pthread_t thread_id;
////
////    // Create a new thread
////    pthread_create(&thread_id, NULL, thread_function, NULL);
////
////    // Main thread logic
////
////    // Lock the mutex before printing
////    pthread_mutex_lock(&mutex);
////
////    // Print to the console
////    printf("Hello from main thread!\n");
////
////    // Unlock the mutex after printing
////    pthread_mutex_unlock(&mutex);
////
////    // Wait for the created thread to finish
////    pthread_join(thread_id, NULL);
////
////    return 0;
////}
//
//// *****************************************************************************************
//
////
////#include <stdio.h>
////#include <stdlib.h>
////#include <unistd.h>
////#include <dirent.h>
////#include <pthread.h>
////#include <string.h>
////
////#include <sys/types.h>
////#include <sys/stat.h>
////#define MAX_PATH_LENGTH 1024
////#define MAX_FILE_TYPES 100
////#define MAX_FILE_TYPE_LENGTH 256
////
////typedef struct {
////    int file_count;
////    int type_count;
////    off_t max_size;
////    char max_size_path[MAX_PATH_LENGTH];
////    off_t min_size;
////    char min_size_path[MAX_PATH_LENGTH];
////    off_t total_size;
////    int file_type_counts[MAX_FILE_TYPES];
////    char file_type_names[MAX_FILE_TYPES][MAX_FILE_TYPE_LENGTH];
////    pthread_mutex_t mutex;
////} SharedData;
////
////typedef struct {
////    char path[MAX_PATH_LENGTH];
////    SharedData* shared_data;
////} WorkerArgs;
////
////void* process_directory(void* arg);
////void* thread_worker(void* arg);
////void update_statistics(const char* path, off_t size, SharedData* shared_data);
////
////void initialize_shared_data(SharedData* shared_data) {
////    memset(shared_data, 0, sizeof(SharedData));
////    pthread_mutex_init(&shared_data->mutex, NULL);
////}
////
////int main() {
////
//////    fflush(stdout);
////
////    char root_directory[MAX_PATH_LENGTH] = "/home/parinaz/Documents";
////
////    SharedData shared_data;
////    initialize_shared_data(&shared_data);
////
////    pthread_t thread_id;
////    WorkerArgs main_worker_args;
////
////    strcpy(main_worker_args.path, root_directory);
////    main_worker_args.shared_data = &shared_data;
////    printf("HERE\n");
////    int retval = pthread_create(&thread_id, NULL, process_directory, &main_worker_args);
////    if (retval != 0) {
////        printf("Failed to create main thread\n");
////        fflush(stdout);
////        return EXIT_FAILURE;
////    }
////
////    pthread_join(thread_id, NULL);
////
////    printf("Number of files: %d\n", shared_data.file_count);
////    printf("Number of different types of files: %d\n", shared_data.type_count);
////    printf("Address of the file with maximum size: %s, Size: %lu\n", shared_data.max_size_path, shared_data.max_size);
////    printf("Address of the file with minimum size: %s, Size: %lu\n", shared_data.min_size_path, shared_data.min_size);
////    printf("Final size of the root folder: %lu\n", shared_data.total_size);
////    fflush(stdout);
////
////    return 0;
////}
////
////void* process_directory(void* arg) {
////    printf("HERE2");
////    fflush(stdout);
////    WorkerArgs* worker_args = (WorkerArgs*)arg;
////
////    char* path = worker_args->path;
////    SharedData* shared_data = worker_args->shared_data;
////
////    DIR* dir = opendir(path);
////    if (!dir) {
////        perror("Error opening directory");
////        printf("Failed to open directory: %s\n", path);
////        fflush(stdout);
////        pthread_exit(NULL);
////    }
////
////    struct dirent* entry;
////    pthread_t thread_ids[MAX_FILE_TYPES];
////    int thread_count = 0;
////
////    while ((entry = readdir(dir)) != NULL) {
////        char sub_path[MAX_PATH_LENGTH];
////        sprintf(sub_path, "%s/%s", path, entry->d_name);
////
////        if (entry->d_type == DT_REG || (entry->d_type == DT_DIR
////                                        && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)) {
////            WorkerArgs new_worker_args;
////            strcpy(new_worker_args.path, sub_path);
////            new_worker_args.shared_data = shared_data;
////
////            pthread_create(&thread_ids[thread_count], NULL, thread_worker, &new_worker_args);
////            thread_count++;
////        }
////    }
////
////    for (int i = 0; i < thread_count; ++i) {
////        pthread_join(thread_ids[i], NULL);
////    }
////
////    closedir(dir);
////    pthread_exit(NULL);
////}
////
////void* thread_worker(void* arg) {
////    WorkerArgs* worker_args = (WorkerArgs*) arg;
////    char* path = worker_args->path;
////    SharedData* shared_data = worker_args->shared_data;
////    struct stat st;
////
////    pthread_mutex_lock(&shared_data->mutex);
////
////    if (stat(path, &st) == 0) {
////        update_statistics(path, st.st_size, shared_data);
////    } else {
////        perror("Error getting file stats");
////    }
////
////    pthread_mutex_unlock(&shared_data->mutex);
////
////    pthread_exit(NULL);
////}
////
////void update_statistics(const char* path, off_t size, SharedData* shared_data) {
////    pthread_mutex_lock(&shared_data->mutex);
////
////    shared_data->file_count++;
////    shared_data->total_size += size;
////
////    if (size > shared_data->max_size) {
////        shared_data->max_size = size;
////        strcpy(shared_data->max_size_path, path);
////    }
////
////    if (size < shared_data->min_size || shared_data->min_size == 0) {
////        shared_data->min_size = size;
////        strcpy(shared_data->min_size_path, path);
////    }
////
////    const char *file_extension = strrchr(path, '.');
////    if (file_extension) {
////        file_extension++;
////    } else {
////        file_extension = "unknown";
////    }
////
////    int found = 0;
////    for (int i = 0; i < shared_data->type_count; ++i) {
////        if (strcmp(file_extension, shared_data->file_type_names[i]) == 0) {
////            found = 1;
////            break;
////        }
////    }
////
////    if (!found) {
////        strcpy(shared_data->file_type_names[shared_data->type_count], file_extension);
////        shared_data->file_type_counts[shared_data->type_count] = 1;
////        shared_data->type_count++;
////    } else {
////        int type_index = -1;
////        for (int i = 0; i < shared_data->type_count; ++i) {
////            if (strcmp(file_extension, shared_data->file_type_names[i]) == 0) {
////                type_index = i;
////                break;
////            }
////        }
////
////        if (type_index != -1) {
////            shared_data->file_type_counts[type_index]++;
////        }
////    }
////
////    pthread_mutex_unlock(&shared_data->mutex);
////}
////
////
////
////
////
////
////
////