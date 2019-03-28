#include "philosopher.h"
extern uint32_t tos_sharedMem;
extern void main_philosopher();
void printf(char* string){
    write(STDOUT_FILENO,string,strlen(string));
}

void main_philosopherFactory(){
    sem_t* chopsticks[16];
    for(int i = 0; i < 16; i++){
        *(chopsticks[i]) = 0;
    }
    memcpy(&tos_sharedMem,&chopsticks,sizeof(chopsticks));
    for(int i = 0; i < 16; i++){
        pid_t p = fork();
        if(p == 0){
            while(1){
                char id[2];
                itoa(id,i);
                char philosopher[16] = "philosopher ";
                strcat(philosopher,id);
                sem_wait((sem_t*)(&tos_sharedMem + i));
                sem_wait((sem_t*)(&tos_sharedMem + ((i + 1) % 16)));
                char eating[50];
                strcpy(eating,philosopher);
                strcat(eating," is eating\n");
                printf(eating);
                sem_post((sem_t*)(&tos_sharedMem + i));
                sem_post((sem_t*)(&tos_sharedMem + ((i + 1) % 16)));
                char finished[60];
                strcpy(finished,philosopher);
                strcat(finished," has finished eating\n");
                printf(finished);
            }
            exit(EXIT_SUCCESS);
        }else{
        }
    }
    exit(EXIT_SUCCESS);
}
