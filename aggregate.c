#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main(){
    // recieve serialized global model from aggregate.py
    char buffer[1024]; // should it really be this small ??
    int count = read(STDIN_FILENO, buffer, sizeof(buffer));

    // send serialized global model to train.py
    FILE* fp = popen("python3 train.py" , "w");
    fwrite(buffer, 1, count, fp);
    pclose(fp);

    return 0;
}