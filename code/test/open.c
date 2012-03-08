#include "syscall.h"

 void main()
 {
   int id,rd;
   char *p = "Test:Achieve your dreams!";
   char buff[100];
   id = Open("test1.txt");
   Write(p,30,id);
   Close(id);
   MyCopy("test1.txt","test2.txt",5,30);
   id = Open("test2.txt");
   if(id<0)
   	Print("\nOpening Failed");
   rd=Read(buff,30,id);
   Print("\nFile Read: ");
   Print(buff);
   Print("\n");
   Close(id);
   Halt();
 }
