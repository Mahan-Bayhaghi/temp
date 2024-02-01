#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_CLASSES 10
#define NUM_IMAGES_PER_FILE 10000
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

void child_process(cifar10* data , int* stoppage , int classes){
	int class1 = classes*2;
	int class2 = class1 + 1;
       		// Child process
		// TODO: Read data from shared memory, perform computation, write results back to shared memory
        printf("I child %d and I have access to data\n", getpid());
	printf("data[0].label = %d\n",data[0].label);
	printf("believe me now ? \n");
 	printf("sizeof each cifar10 object is %ld\n",sizeof(cifar10));
//	while (!(*stoppage)){
		// TODO: Send weights and data to a python process and get back results
//	}
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
    printf("sizeof(data) = %ld MB\n", sizeof(cifar10)*NUM_IMAGES_PER_FILE / (1024*1024));
    
    for (int kid=0; kid<5; kid++){
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
    }

    // parent 
    
    	if (shmdt(data) == -1) {
        	perror("shmdt");
        	return 1;
   	 }
  //  }
    return 0;
}
