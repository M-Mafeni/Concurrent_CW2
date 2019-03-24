
#include "test.h"

extern uint32_t tos_SharedMem;
void printf(char* string){
    write(STDOUT_FILENO,string,strlen(string));
}
void main_test(){
   int fd[2] = {96,95};
   int x = (int) &fd[0];
   int y = (int) &fd[1];
   fd[0] = x;
   fd[1] = y;
   pipe(fd);
   char test[2];
   memcpy(&tos_SharedMem,fd,sizeof(fd));
   itoa(test,(int)(*(&tos_SharedMem)));
   int a = fork();
   if(a == 0){
       //receive from parent
       char string[12];
       strcpy(string,receive(*(&tos_SharedMem + 1)));
       //strcpy(string,receive(fd[1]));
       write(STDOUT_FILENO,"message received",strlen("message received"));
       write(STDOUT_FILENO,string,12);
   }else{
       //send to child
       send(*(&tos_SharedMem),"hello world\n");
     //  send( fd[0],"hello world\n");
       write(STDOUT_FILENO,"message sent to child \n",23); 
   }
   exit(EXIT_SUCCESS);
}