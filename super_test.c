#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/msg.h> 
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define NUM_CLASSES 10
#define NUM_IMAGES_PER_FILE 10000
#define NUM_BATCHES 5
#define IMAGE_DIMENSION 32

#define NUM_PROCESSES 1

#define MAX 10 

typedef struct {
    unsigned char r, g, b;
} pixel;

typedef struct {
    pixel image[IMAGE_DIMENSION][IMAGE_DIMENSION];
    unsigned char label;
} cifar10;

// Define the dimensions of each weight matrix
#define DIM1 3
#define DIM2 3
#define DIM3 3
#define DIM4 32
#define DIM5 32
#define DIM6 3
#define DIM7 3
#define DIM8 32
#define DIM9 64
#define DIM10 64
#define DIM11 3
#define DIM12 3
#define DIM13 64
#define DIM14 64
#define DIM15 1024
#define DIM16 64
#define DIM17 64
#define DIM18 10
// Define the total number of weights
#define TOTAL_WEIGHTS (DIM1 * DIM2 * DIM3 * DIM4 + DIM5 + DIM6 * DIM7 * DIM8 * DIM9 + DIM10 + DIM11 * DIM12 * DIM13 * DIM14 + DIM15 + DIM16 * DIM17 + DIM18)
typedef struct {
    float data[TOTAL_WEIGHTS];
} Weights;

void read_cifar10(char *filename, cifar10 *data) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error: Unable to open file %s\n", filename);
        return;
    }
    for (int i = 0; i < NUM_IMAGES_PER_FILE; i++) {
        fread(&data[i].label, sizeof(unsigned char), 1, file);
        for (int j = 0; j < IMAGE_DIMENSION; j++) {
            for (int k = 0; k < IMAGE_DIMENSION; k++) {
                fread(&data[i].image[j][k], sizeof(pixel), 1, file);
            }
        }
    }
    fclose(file);
}

void receive_weights(int fd, Weights *weights) {
    size_t total_bytes = sizeof(float) * TOTAL_WEIGHTS;
    size_t bytes_read = 0;

    while (bytes_read < total_bytes) {
        ssize_t bytes_received = read(fd, (char *)weights + bytes_read, total_bytes - bytes_read);
        if (bytes_received == -1) {
            perror("Error reading from the pipe");
            exit(EXIT_FAILURE);
        }
        bytes_read += bytes_received;
    }
}

void aggregate_weights(Weights *global_weight, const Weights *temp_weights, int count) {
    for (int i = 0; i < TOTAL_WEIGHTS; i++) {
        global_weight->weights[i] += temp_weights->weights[i] / count;
    }
}

float calculate_weight_difference_norm(const Weights *w1, const Weights *w2) {
    float norm = 0.0;
    for (int i = 0; i < TOTAL_WEIGHTS; i++) {
        float diff = w1->weights[i] - w2->weights[i];
        norm += diff * diff;
    }
    return sqrt(norm);
}

void child_process(cifar10* data, Weights* global_weight, int* stoppage, int classes) {
    int class1 = classes * 2;
    int class2 = class1 + 1;
    Weights* temp_weights = malloc(sizeof(float) * TOTAL_WEIGHTS);

    printf("I'm child %d and I have access to data\n", getpid());

    // Filter and save instances with labels class1 or class2
    cifar10* filtered_data = malloc(NUM_IMAGES_PER_FILE * sizeof(cifar10));
    if (filtered_data == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    int filtered_count = 0;
    for (int i = 0; i < NUM_IMAGES_PER_FILE; i++) {
        if (data[i].label == class1 || data[i].label == class2) {
            memcpy(&filtered_data[filtered_count], &data[i], sizeof(cifar10));
            filtered_count++;
        }
    }

    // Create named pipes (FIFOs)
    char weight_send_fifo[20];
    sprintf(weight_send_fifo, "/tmp/wsfifo%d", classes);
    mkfifo(weight_send_fifo, 0777);
    printf("weight_send_fifo is made\n");

    char weight_receive_fifo[20];
    sprintf(weight_receive_fifo, "/tmp/wrfifo%d", classes);
    mkfifo(weight_receive_fifo, 0777);
    printf("weight_receive_fifo is made\n");

    char image_send_fifo[20];
    sprintf(image_send_fifo, "/tmp/isfifo%d", classes);
    mkfifo(image_send_fifo, 0777);
    printf("image_send_fifo is made\n");

    // Open pipes for writing
    int fd1 = open(weight_send_fifo, O_WRONLY);
    if (fd1 == -1) {
        perror("Error opening the FIFO for writing weight");
        exit(EXIT_FAILURE);
    }

    int fd2 = open(image_send_fifo, O_WRONLY);
    if (fd2 == -1) {
        perror("Error opening the FIFO for writing images");
        exit(EXIT_FAILURE);
    }

    // Open pipe for reading
    int fd3 = open(weight_receive_fifo, O_RDONLY);
    if (fd3 == -1) {
        perror("Error opening the FIFO for receiving updated weights");
        exit(EXIT_FAILURE);
    }

    // Main training loop of each child process
    while (*stoppage == 0 && filtered_count > 0) {
        // TODO: Send weights (implementation depends on your specific requirements)
        write(fd1, global_weight, sizeof(Weights));

        // Send 16 instances of CIFAR-10 to the Python script
        int instances_to_send = (filtered_count >= 16) ? 16 : filtered_count;
        write(fd2, &filtered_data[filtered_count - instances_to_send], sizeof(cifar10) * instances_to_send);

        // Wait for updated weights
        // TODO: Receive data and aggregate weights. Also, check if we should stop or not
        receive_weights(fd3,temp_weights);
        aggregate_weights(global_weight, temp_weights, instances_to_send);

        // Check the stopping condition based on the norm of the difference
        float weight_diff_norm = calculate_weight_difference_norm(global_weight, temp_weights);
        if (weight_diff_norm < 0.001) {
            *stoppage = 1;  // Set stoppage condition
        }

        filtered_count -= instances_to_send; // Decrement if needed
    }

    // Close the named pipes
    close(fd1);
    close(fd2);
    close(fd3);
    
    // Free allocated memory for filtered_data
    free(filtered_data);
}

int main() {
    int shm_stoppage = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shm_stoppage < 0) {
        perror("stoppage couldn't be shared");
        return 1;
    }
    int shmid = shmget(IPC_PRIVATE, NUM_IMAGES_PER_FILE * NUM_BATCHES * sizeof(cifar10), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("data couldn't be shared");
        return 1;
    }

    int* stoppage = shmat(shm_stoppage, NULL, 0);
    if (stoppage == (int*) -1) {
        perror("stoppage is not shared");
        return 1;
    }
    cifar10 *data = shmat(shmid, NULL, 0);
    if (data == (cifar10 *) -1) {
        perror("data is not shared");
        return 1;
    }

    // Create shared memory for global_weight
    int shmid_global_weight = shmget(IPC_PRIVATE, sizeof(Weights), IPC_CREAT | 0666);
    if (shmid_global_weight < 0) {
        perror("global_weight couldn't be shared");
        return 1;
    }
    Weights *global_weight = shmat(shmid_global_weight, NULL, 0);
    if (global_weight == (Weights *) -1) {
        perror("global_weight is not shared");
        return 1;
    }
    // Initialize global_weight to zeros
    memset(global_weight->weights, 0, sizeof(global_weight->weights));


    printf("initializing stoppage condition to 0\n");
    *stoppage = 0;

    // Read all batches of CIFAR-10 data
    for (int batch = 1; batch <= NUM_BATCHES; batch++) {
        char filename[60];
        sprintf(filename, "../cifar-10-batches-bin/data_batch_%d.bin", batch);
        printf("starting to read batch %d...\n", batch);
        read_cifar10(filename, data + (batch - 1) * NUM_IMAGES_PER_FILE);
        printf("batch %d reading done\n", batch);
    }

    printf("sizeof(data) = %ld kB\n", sizeof(cifar10) * NUM_IMAGES_PER_FILE * NUM_BATCHES / (1024));

    for (int kid = 0; kid < NUM_PROCESSES; kid++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            return 1;
        }
        if (pid == 0) {
            // child process
            child_process(data, global_weight, stoppage, kid);
            exit(0);
        } else {  // parent process
            // TODO: Additional parent process logic if needed
        }
    }

    // parent process
    // TODO: Wait for all child processes to finish
    for (int i = 0; i < NUM_PROCESSES; i++) {
        wait(NULL);
    }

    // Cleanup shared memory
    if (shmdt(data) == -1) {
        perror("shmdt");
        return 1;
    }
    // Detach and remove shared memory for global_weight
    if (shmdt(global_weight) == -1) {
        perror("shmdt for global_weight");
        return 1;
    }
    if (shmctl(shmid_global_weight, IPC_RMID, NULL) == -1) {
        perror("shmctl for global_weight");
        return 1;
    }
    return 0;
}
