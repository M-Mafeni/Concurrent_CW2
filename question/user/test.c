
#include "test.h"

void main_test(){
   int fd[2];
  // pipe(fd);
   int a = fork();
   if(a == 0){
       //receive from parent
       char string[12];
    //   strcpy(string,receive(&fd[0]));
       write(STDOUT_FILENO,"message received",strlen("message received"));
   //    write(STDOUT_FILENO,string,12);
   }else{
       //send to child
  //     send(&fd[1],"hello world\n");
       write(STDOUT_FILENO,"message sent to child \n",23); 
   }
   exit(EXIT_SUCCESS);
}