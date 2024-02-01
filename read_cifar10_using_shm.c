#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_CLASSES 10
#define NUM_IMAGES_PER_FILE 200
#define IMAGE_DIMENSION 32

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

int main() {
    int shmid = shmget(IPC_PRIVATE, NUM_IMAGES_PER_FILE * sizeof(cifar10), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return 1;
    }

    cifar10 *data = shmat(shmid, NULL, 0);
    if (data == (cifar10 *) -1) {
        perror("shmat");
        return 1;
    }
    printf("starting to read batch 1...\n");
    read_cifar10("data_batch_1.bin", data);
    printf("batch 1 reading done\n");

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Child process
        // TODO: Read data from shared memory, perform computation, write results back to shared memory
        printf("I am a child and I have access to data\n");
	printf("data[0].label = %d\n",data[0].label);
	printf("believe me now ? \n");
    } else {
        // Parent process
	printf("I am a parent process and i'll be aggregating data from my childs\n");
        wait(NULL);  // Wait for child process to finish
        // TODO: Read results from shared memory, aggregate them
    }

    if (shmdt(data) == -1) {
        perror("shmdt");
        return 1;
    }

    return 0;
}
