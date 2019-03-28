
#include "test.h"

extern uint32_t tos_channelRight;

void main_test(){
   int fd[2] = {96,95};
   int x = (int) &fd[0];
   int y = (int) &fd[1];
   fd[0] = x;
   fd[1] = y;
   pipe(fd);
   char test[2];
   memcpy(&tos_channelRight,fd,sizeof(fd));
   itoa(test,(int)(*(&tos_channelRight)));
   int a = fork();
   if(a == 0){
       //receive from parent
       char string[12];
       strcpy(string,receive(*(&tos_channelRight+ 1), *(&tos_channelRight) ));
       write(STDOUT_FILENO,"message received",strlen("message received"));
       write(STDOUT_FILENO,string,12);
   }else{
       //send to child
       send(*(&tos_channelRight),*(&tos_channelRight + 1),"hello world\n");
       write(STDOUT_FILENO,"message sent to child \n",23); 
   }
   exit(EXIT_SUCCESS);
}