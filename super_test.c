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

#define NUM_CLASSES 10
#define NUM_IMAGES_PER_FILE 10000
#define IMAGE_DIMENSION 32

#define NUM_PROCESSES 2

#define MAX 10 

struct mseg_buffer
{
    long mesg_type;
    char mesg_text[100];
} message;


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


void child_process(cifar10* data , int* stoppage , int classes, int *c_to_python_pipe){
	int class1 = classes*2;
	int class2 = class1 + 1;
    printf("I child %d and I have access to data\n", getpid());

    char* fifo = "/tmp/myfifo";
    mkfifo(fifo,0666);
    printf("fifo made\n");

    int fd = open(fifo, O_WRONLY);
    if (fd == -1) {
        perror("Error opening the FIFO for writing");
        exit(EXIT_FAILURE);
    }
    // Send a sample CIFAR-10 instance to the Python script
    write(fd, data[0].image, sizeof(pixel) * IMAGE_DIMENSION * IMAGE_DIMENSION);
    write(fd, &(data[0].label), sizeof(unsigned char));
    // Close the named pipe
    close(fd);
}

int main() {
    int shm_stoppage = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shm_stoppage < 0){
        perror("stoppage couldn't be shared");
        return 1;
    }
    int shmid = shmget(IPC_PRIVATE, NUM_IMAGES_PER_FILE * sizeof(cifar10), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("data couldn't be shared");
        return 1;
    }
	
    int* stoppage = shmat(shm_stoppage,NULL, 0);
    if (stoppage == (int*) -1){
        perror("stoppage is not shared");
        return 1;
    }
    cifar10 *data = shmat(shmid, NULL, 0);
    if (data == (cifar10 *) -1) {
        perror("data is not shared");
        return 1;
    }
    printf("initializing stoppage condition to 0");
    *stoppage = 0;
    printf("starting to read batch 1...\n");
    read_cifar10("data_batch_1.bin", data);
    printf("batch 1 reading done\n");
    printf("sizeof(data) = %ld kB\n", sizeof(cifar10)*NUM_IMAGES_PER_FILE / (1024));
    
    for (int kid=0; kid<NUM_PROCESSES; kid++){
	    pid_t pid = fork();
	    if (pid < 0){
		    perror("fork failed");
		    return 1;
	    }
	    if (pid==0){
		    // child process
		    child_process(data,stoppage,kid);
		    exit(0);
	    }
        else {  // parent process

        }
    }

    // parent 
    
    if (shmdt(data) == -1) {
        perror("shmdt");
        return 1;
   	 }
    return 0;
   }


