#include "philosopher.h"

void main_philosopher(){
    int childCount = 0;
    int a = fork();
    int b = fork();
    int c = fork();
    int d = fork();
    int test[4] = {a,b,c,d};
    char digits[2];
    for(int i = 0; i < 4; i++){
        if(test[i] != 0){
            childCount++;
        }
    }
    itoa(digits,childCount);
    write(STDOUT_FILENO,digits,2);

    exit(EXIT_SUCCESS);
}
