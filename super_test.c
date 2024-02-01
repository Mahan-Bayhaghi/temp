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

void child_process(cifar10* data, int* stoppage, int classes) {
    int class1 = classes * 2;
    int class2 = class1 + 1;
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

        // Send 16 instances of CIFAR-10 to the Python script
        int instances_to_send = (filtered_count >= 16) ? 16 : filtered_count;
        write(fd2, &filtered_data[filtered_count - instances_to_send], sizeof(cifar10) * instances_to_send);

        // Wait for updated weights
        // TODO: Receive data and aggregate weights. Also, check if we should stop or not

        // Update stoppage condition or decrement filtered_count based on your logic
        // *stoppage = ...; // Update stoppage condition
        // filtered_count -= instances_to_send; // Decrement if needed
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
            child_process(data, stoppage, kid);
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
}
