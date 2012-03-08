// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUF_SIZE	100		//maximum size of the string that must be passed to Print()

char buf[BUF_SIZE];
char buf2[BUF_SIZE];

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void updatePC(){

	// Note that we have to maintain three PC registers, 
	// namely : PCReg, NextPCReg, PrevPCReg. 
	// (See machine/machine.cc, machine/machine.h) for more details.
	int pc, nextpc, prevpc;

	// Read PCs
	prevpc = machine->ReadRegister(PrevPCReg);
	pc = machine->ReadRegister(PCReg);
	nextpc = machine->ReadRegister(NextPCReg);

	// Update PCs
	prevpc = pc;
	pc = nextpc;
	nextpc = nextpc + 4;	// PC incremented by 4 in MIPS

	// Write back PCs
	machine->WriteRegister(PrevPCReg, prevpc);
	machine->WriteRegister(PCReg, pc);
	machine->WriteRegister(NextPCReg, nextpc);
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

	switch(which) {
		case SyscallException:
			switch(type) {
				case SC_Halt:
					DEBUG('a', "Shutdown, initiated by user program.\n");
					interrupt->Halt();
					updatePC();
					break;
			
				case SC_Open:
					{
					DEBUG('a', "Open() system call invoked \n");
					
					int fd1=machine->ReadRegister(4);
					
					int size=0;
					buf[BUF_SIZE - 1] = '\0';
					
					 do {
					   // Invoke ReadMem to read the contents from user space

					   machine->ReadMem(fd1,    // Location to be read
					      sizeof(char),      // Size of data to be read
					      (int*)(buf+size)   // where the read contents 
					      );                 // are stored

					   // Compute next address
					   fd1+=sizeof(char);
					   size++;
					   
					} while( size < (BUF_SIZE - 1) && buf[size-1] != '\0');

					size--;
					DEBUG('a', "Size of string = %d", size);
					
					//now we have the filename to be opened in buf
					int fd = open(buf,O_CREAT | O_RDWR,0666);	
					
					machine->WriteRegister(2,fd);		// Open() must return the file descriptor
									
					bzero(buf, sizeof(char)*BUF_SIZE);  // Zeroing the buffer.
                        		updatePC();
					}
					break;
				case SC_Close:
					{
					/*Close(fd)
					  Read the argument from r4
					  close the file
					 */
					DEBUG('a', "Close() system call invoked");
					
					int fd2=machine->ReadRegister(4);
					close(fd2);
					
					updatePC();
					}
					break;	//SC_Close
					
				case SC_Read:
					{
					/* int Read(char *buffer, int size, OpenFileId id)
						ipstring=r4
						ipsize=r5=number of characters to be read from the file
						fd=r6
					   sizeread=read(fd,buf,ipsize)
					   sizeread< 0 means some read error
					   if no read error then write the string read from the file to the starting memory as pointed by ipstring
					   write sizeread into r2
					   updatePC
					 */
					 DEBUG('a',"Read() system call invoked");

					int ipstring=machine->ReadRegister(4);
					int ipsize=machine->ReadRegister(5);
					int fd3=machine->ReadRegister(6);

					int size=0;
					bzero(buf,sizeof((char*)BUF_SIZE));
					int sizeread=read(fd3,buf,ipsize);

					if(sizeread < 0)
						printf("\nRead() Failed");

					printf("\ncontent read: %s",buf);

					for(size=0;size<=sizeread; size++) {
						machine->WriteMem(ipstring,sizeof(char),(int)(*(buf+size)));
						ipstring+=sizeof(char);
					}
					size--;
					DEBUG('a', "Size of string = %d", size);
						
					machine->WriteRegister(2,sizeread);
					bzero(buf, sizeof(char)*BUF_SIZE);  // Zeroing the buffer.
					updatePC();
					}
					break;	//SC_Read
					
				case SC_Write:
					{
					/*
						void Write(char *buffer, int size, OpenFileId id)
	
						read arguments from registers
							ipstring=r4
							ipsize=r5
							fd=r6
						store the string pointed to by buffer into buf
						if write(.....) <0 then Write() fails
						zero the buf
						updatePC
					 */
					 
					DEBUG('a',"Write() system call invoked");
					 
					int ipstring=machine->ReadRegister(4);
					int ipsize=machine->ReadRegister(5);
					int fd4=machine->ReadRegister(6);

					int size = 0;
					buf[BUF_SIZE - 1] = '\0';

					do {
						machine->ReadMem(ipstring,sizeof(char),(int*)(buf+size));
						ipstring+=sizeof(char);    
						size++;
					}while(size<(BUF_SIZE-1) && buf[size-1]!='\0');
					size--;
					DEBUG('a', "Size of string = %d", size);

					if(ipsize>size)
						ipsize=size;

					printf("Content To Be Written : %s", buf);

					if((write(fd4,buf,ipsize)) < 0)
						printf("\nWrite() Failed\n");

					bzero(buf, sizeof(char)*BUF_SIZE);  // Zeroing the buffer.
					updatePC();
					}
					break;	//SC_Write
					
				case SC_Print:
					{
					DEBUG('a', "Print() system call invoked \n");
					int vaddr = machine->ReadRegister(4);
					// This address (pointer to the string to be printed) is 
					// the address that pointes to the user address space.
					// Simply trying printf("%s", (char*)addr) will not work
					// as we are now in kernel space.

					// Get the string from user space.

					int size = 0;

					buf[BUF_SIZE - 1] = '\0';               // For safety.

					do{
						// Invoke ReadMem to read the contents from user space

						machine->ReadMem(vaddr,    // Location to be read
							sizeof(char),      // Size of data to be read
							(int*)(buf+size)   // where the read contents 
							);                 // are stored

						// Compute next address
						vaddr+=sizeof(char);    size++;

					} while( size < (BUF_SIZE - 1) && buf[size-1] != '\0');

					size--;
					DEBUG('a', "Size of string = %d", size);

					printf("%s", buf);
					bzero(buf, sizeof(char)*BUF_SIZE);  // Zeroing the buffer.
					updatePC();
					}
					break; // SC_Print

				case SC_MyCopy:
					{
					DEBUG('a', "MyCopy() system call invoked \n");
					int fd1=machine->ReadRegister(4);
					int fd2=machine->ReadRegister(5);
//// Opening file1									
					int size=0;
					buf[BUF_SIZE - 1] = '\0';
					
					 do {
					   // Invoke ReadMem to read the contents from user space

					   machine->ReadMem(fd1,    // Location to be read
					      sizeof(char),      // Size of data to be read
					      (int*)(buf+size)   // where the read contents 
					      );                 // are stored

					   // Compute next address
					   fd1+=sizeof(char);
					   size++;
					   
					} while( size < (BUF_SIZE - 1) && buf[size-1] != '\0');

					size--;
					DEBUG('a', "Size of string = %d", size);
					
					//now we have the filename to be opened in buf
					int fdd1 = open(buf,O_CREAT | O_RDWR,0666);	
					bzero(buf, sizeof(char)*BUF_SIZE);  // Zeroing the buffer.


					int pos1=machine->ReadRegister(6);
					int pos2 = machine->ReadRegister(7);;

					lseek(fdd1,pos1,SEEK_SET); // putting file pointer to pos1
					int sizeread=read(fdd1,buf2,pos2);	// reading upto pos2
					if(sizeread < 0)
						printf("\nRead() Failed");
					printf("\nMyCopy: Content Read: %s\n",buf2);
					
					
///	Opening file2
					size = 0;	
					buf[BUF_SIZE - 1] = '\0';
					
					 do {
					   // Invoke ReadMem to read the contents from user space

					   machine->ReadMem(fd2,    // Location to be read
					      sizeof(char),      // Size of data to be read
					      (int*)(buf+size)   // where the read contents 
					      );                 // are stored

					   // Compute next address
					   fd2+=sizeof(char);
					   size++;
					   
					} while( size < (BUF_SIZE - 1) && buf[size-1] != '\0');

					size--;
					DEBUG('a', "Size of string = %d", size);
					
					//now we have the filename to be opened in buf
					int fdd2 = open(buf,O_CREAT | O_RDWR,0666);	
					bzero(buf, sizeof(char)*BUF_SIZE);  // Zeroing the buffer.


					printf("MyCopy:Content To Be Written : %s\n", buf2);

					if((write(fdd2,buf2,pos2-pos1+1)) < 0)
						printf("\nWrite() Failed\n");

					bzero(buf, sizeof(char)*BUF_SIZE);  // Zeroing the buffer.
					bzero(buf2, sizeof(char)*BUF_SIZE);  // Zeroing the buffer.

					updatePC();
					}
					break; // SC_MyCopy				
			
				default:
					printf("Unknown/Unimplemented system call %d!", type);
					ASSERT(FALSE);
					break;
			}//end switch(type)
			break;//end case SyscallException
	
		default:
			printf("Unexpected user mode exception %d %d\n", which, type);
			ASSERT(FALSE);
			break;
	}//end switch(which)
}
