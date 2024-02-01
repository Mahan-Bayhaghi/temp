#include <stdio.h>
#include <stdlib.h>

#define NUM_CLASSES 10
#define NUM_IMAGES_PER_FILE 600
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
    printf("start reading first batch\n");
    cifar10 data[NUM_IMAGES_PER_FILE];
    read_cifar10("data_batch_1.bin", data);
    printf("succefully read batch 1 \n");
    // Now you can access the images and labels from the data array
    // For example, to access the label of the first image, use data[0].label
    // To access the RGB values of the first pixel of the first image, use data[0].image[0][0].r, data[0].image[0][0].g, and data[0].image[0][0].b
    printf("Label of the first image: %d\n", data[0].label);
    int* count = calloc(sizeof(int) , 10);
    for (int i=0; i<NUM_IMAGES_PER_FILE;i++){
        count[data[i].label]+=1;
    }
    for (int i=0; i<10;i++){
        printf("%d --> %d\n",i,count[i]);
    }
    return 0;
}
