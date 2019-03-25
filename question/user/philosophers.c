#include "philosopher.h"
extern uint32_t tos_SharedMem;
extern uint32_t tos_SharedMem1;
void main_philosopher(){
    //channels to be used for IPC
    int channelsTo[16][2]; //e.g philosopher 0 -> 1
    int channelsFrom[16][2]; //e.g philosopher 1 -> 0
    for(int i= 0; i < 16; i++){
        //channelsFrom[i][0] = send;
        //channelsFrom[i][1] = receive;
        channelsTo[i][0] = i;
        channelsTo[i][1] = (i+1) %16;
        pipe(channelsTo[i]);
        channelsFrom[i][0] = (i+1) %16;
        channelsFrom[i][1] = i;
        pipe(channelsFrom[i]);
    }
    memcpy(&tos_SharedMem,channelsTo,sizeof(channelsTo) *sizeof(channelsTo[0]));
    memcpy(&tos_SharedMem1,channelsFrom,sizeof(channelsFrom) *sizeof(channelsFrom[0]));
    
    exit(EXIT_SUCCESS);
}
