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
int fib(int x){
    if(x == 0) return 0;
    if(x == 1) return 1;
    else{
        return fib(x-1) + fib(x-2);
    }
}
void main_philosopher(){
    int var[16];
    sem_t *chopsticks[16];
    for(int i = 0; i < 16; i++){
        var[i] = 1;
        chopsticks[i] = &var[i];
        // chopsticks[i] = (int) &x;
        // sem_t * p = (sem_t *)chopsticks[i];
        // *p = 1;
        // memset(&chopsticks[i],0,sizeof(chopsticks[i]));
        // sem_init(chopsticks[i],1);
    }
    memcpy(&tos_sharedMem,&var,sizeof(int) * 16);
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
