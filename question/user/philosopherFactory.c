#include "philosopher.h"
extern uint32_t tos_channelRight;
extern uint32_t tos_channelLeft;
extern uint32_t tos_philosophers;
extern void main_philosopher();
sem_t chopsticks[16];
void printf(char* string){
    write(STDOUT_FILENO,string,strlen(string));
}
void main_philosopherFactory(){
    for(int i = 0; i < 16; i++){
        pid_t p = fork();
        if(p == 0){
            char test[2];
            itoa(test,i);
            while(1){
                printf(test);
            }
            exit(EXIT_SUCCESS);
        }else{
        }
    }
    exit(EXIT_SUCCESS);
}
