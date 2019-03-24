#include "philosopher.h"

void main_philosopher(){
    int childCount = 0;
    //2 pipes
    int parentToChild[2];
    int childToParent[2];
    int a = fork();
    int b = fork();
    int c = fork();
    int d = fork();
    philosopher philosophers[16];
    int test[4] = {a,b,c,d};
    char digits[2];
    sem_t sem;
    //States = THINKING, EATING,
    //chopsticks is fixed point in memory (mutexes)
    //have semaphore value here
    const int chopsticks[16];
    // Thinking is sem_wait
    // Eating is sem_post
    for(int i = 0; i < 4; i++){
        if(test[i] != 0){
            childCount++;
        }
    }
    /*
    
    while(true){
    process waits
}

    */
    itoa(digits,childCount);
    write(STDOUT_FILENO,digits,2);

    exit(EXIT_SUCCESS);
}
