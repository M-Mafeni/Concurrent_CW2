#include "philosopher.h"

extern uint32_t tos_channelRight;
extern uint32_t tos_channelLeft;
extern uint32_t tos_philosophers;
void printf(char* string){
    write(STDOUT_FILENO,string,strlen(string));
}
/* if you hardcode offset value(3)
 * philosopher can only run if it's first value;
 */
void main_philosopher(){
    int Pid = getPID();
    int destination = *(&tos_channelRight + 0) + 1;
    char* test = (char*) receive(destination);
    char id[2];
    itoa(id,Pid);
    printf(id);
    printf("\n");
    exit(EXIT_SUCCESS);
}