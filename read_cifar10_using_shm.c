#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_CLASSES 10
#define NUM_IMAGES_PER_FILE 10000
#define IMAGE_DIMENSION 32

#define NUM_PROCESSES 2

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
       	// Child process
		// TODO: Read data from shared memory, perform computation, write results back to shared memory
    printf("I child %d and I have access to data\n", getpid());
    printf("data[0].label = %d\n",data[0].label);
    printf("believe me now ? \n");
    printf("sizeof each cifar10 object is %ld\n",sizeof(cifar10));

    pid_t pid = fork();
    if (pid==0){
        // grandchild process
        close(c_to_python_pipe[0]); // close read end of pipe
        for (int i=0; i<NUM_IMAGES_PER_FILE, i++){
            int sum = 0;
            for (int j=0; j<IMAGE_DIMENSION; j++){
                for (int k=0; k<IMAGE_DIMENSION; k++){
                    sum += data[i].image[j][k].r;
                    sum += data[i].image[j][k].g;
                    sum += data[i].image[j][k].b;
                }
            }
            write(c_to_python_pipe[1],&sum, sizeof(sum));
        }
        close(c_to_python_pipe[1]); // close write end of pipe
        exit(0);
    } else {
        // child process
        close(c_to_python_pipe[1]); // close write end of pipe

        // Create a unique Python script for each child process
        char script_filename[20];
        sprintf(script_filename, "script%d.py", classes);
        execlp("python3", "python3", script_filename, NULL);
        perror("execlp failed");
        exit(1);
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
    
    for (int kid=0; kid<NUM_PROCESSES; kid++){
        int c_to_python_pipe[2];
        if (pipe(c_to_python_pipe)== -1){
            perror("couldn't make pipe");
            exit(1);
        }

	    pid_t pid = fork();
	    if (pid < 0){
		    perror("fork failed");
		    return 1;
	    }
	    if (pid==0){
		    // child process
		    child_process(data,stoppage,kid,c_to_python_pipe);
		    exit(0);
	    }
        else {
            close(c_to_python_pipe[1]);
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
