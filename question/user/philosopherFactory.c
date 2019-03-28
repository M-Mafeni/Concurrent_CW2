#include "philosopher.h"
extern uint32_t tos_channelRight;
extern uint32_t tos_channelLeft;
extern uint32_t tos_philosophers;
extern void main_philosopher();
void main_philosopherFactory(){
   // channels to be used for IPC
    int channelsRight[16][2]; //e.g philosopher 0 -> 1
    int channelsLeft[16][2]; //e.g philosopher 1 -> 0
    for(int i= 0; i < 16; i++){
        //channelsFrom[i][0] = send;
        //channelsFrom[i][1] = receive;
//         channelsRight[i][0] = i;
//         channelsRight[i][1] = (i+1)%16;
//         pipe(channelsRight[i]);
//         channelsLeft[i][0] = (i+1)%16;
//         channelsLeft[i][1] = i;
//         pipe(channelsLeft[i]);
        
    } 

    int offset;
    for(int i = 0; i < 16; i++){
        int a = fork();
        if(a == 0){
            exec(&main_philosopher);
            exit(EXIT_SUCCESS);
        }else{
            if(i == 0){
                offset = a;
            }
            int neighbour;
            if(a + 1 - offset >= 16){
                neighbour = offset;
            }else{
                neighbour = a + 1;
            }
            channelsRight[i][0] = a;
            channelsRight[i][1] = neighbour;
            pipe(channelsRight[i]);
            channelsLeft[i][0] = neighbour;
            channelsLeft[i][1] = a;
            pipe(channelsLeft[i]);
        }
    }
    memcpy(&tos_channelRight,channelsRight,sizeof(channelsRight) *sizeof(channelsRight[0]));
    memcpy(&tos_channelLeft,channelsLeft,sizeof(channelsLeft) *sizeof(channelsLeft[0]));
    for(int i = 0; i < 16; i++){
        int offset = channelsRight[0][0];
        char x[2];
        itoa(x,offset);
        send(channelsLeft[i][0],channelsLeft[i][1],x);
    }
    exit(EXIT_SUCCESS);
}
