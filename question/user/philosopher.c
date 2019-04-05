#include "philosopher.h"
extern uint32_t tos_sharedMem;
void printf(char* string){
    write(STDOUT_FILENO,string,strlen(string));
}

void concatAndPrint(char* s,char* id,char* data){
    strcpy(s,id);
    strcat(s,data);
    printf(s);
}

void main_philosopher(){
    sem_t chopsticks[16];
    for(int i = 0; i < 16; i++){
        chopsticks[i] = 1;
    }
    //could not implement mmap or shm_open so had to use this for semaphores
    memcpy(&tos_sharedMem,&chopsticks,sizeof(int) * 16);
    for(int i = 0; i < 16; i++){
        pid_t p = fork();
        if(p == 0){
            while(1){
                char id[2];
                itoa(id,i);
                char philosopher[16] = "philosopher ";
                strcat(philosopher,id);
                char waiting[50];
                concatAndPrint(waiting,philosopher," is thinking\n");
                sem_wait((sem_t*)(&tos_sharedMem + i));
                char pickupLeft[50];
                concatAndPrint(pickupLeft,philosopher," picked up left fork\n");
                sem_wait((sem_t*)(&tos_sharedMem + ((i + 1) % 16)));
                char pickupRight[50];
                concatAndPrint(pickupRight,philosopher," picked up right fork\n");
                char eating[50];
                concatAndPrint(eating,philosopher," is eating\n");
                sleep(4); //sleep is here to give some time for eating
                sem_post((sem_t*)(&tos_sharedMem + i));
                char putDownLeft[50];
                concatAndPrint(putDownLeft,philosopher," puts down left fork\n");
                sem_post((sem_t*)(&tos_sharedMem + ((i + 1) % 16)));
                char putDownRight[50];
                concatAndPrint(putDownRight,philosopher," puts down right fork\n");
                char finished[60];
                concatAndPrint(finished,philosopher," has finished eating\n");
                sleep(4);
            }
            exit(EXIT_SUCCESS);
        }else{
        }
    }
    exit(EXIT_SUCCESS);
}
