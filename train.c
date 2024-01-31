#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main(){
    // read trained data from input pipe
    char buffer[1024]; // should it really be this small ??
    int count = read(STDIN_FILENO, buffer, sizeof(buffer));

    // send aggregated data to aggregate.py 
    FILE* fp = popen("python3 aggregate.py", "w");
    fwrite(buffer, 1, count, fp);
    pclose(fp);

    return 0;
}