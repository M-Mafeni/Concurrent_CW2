#include "philosopher.h"

extern uint32_t tos_channelRight;
extern uint32_t tos_channelLeft;
extern uint32_t tos_philosophers;
void printf(char* string){
    write(STDOUT_FILENO,string,strlen(string));
}

void main_philosopher(){
    exit(EXIT_SUCCESS);
}