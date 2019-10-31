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
static unsigned int nextTlbIndex=0;
static unsigned int clockIndex=0;
static unsigned int nextExchangedPhyPage=0;
extern BitMap bitMap;
extern int targetVirtAddr;
int pageFaultCounter=0;


int FindNextTlbIndex(){
	printf("\n更新tlb\n");
	/*
	int i=0;
	for(i=0;i<TLBSize;++i){
		if(!machine->tlb[clockIndex].use){
			break;
		}
		clockIndex++;
		clockIndex%=TLBSize;
	}
	if(i==TLBSize){
		for(i=0;i<TLBSize;++i){
			if(!machine->tlb[clockIndex].dirty){
				break;
			}
			clockIndex++;
			clockIndex%=TLBSize;
		}
	}
	if(machine->tlb[clockIndex].dirty){
		WriteBackToDisk(machine->tlb[clockIndex].virtualPage);
	}
	int nextIndex=clockIndex;
	clockIndex++;
	clockIndex%=TLBSize;
	*/
	nextTlbIndex%=TLBSize;
	return nextTlbIndex++;
}

void DemandingPage(int virtAddr){
	printf("请求调页\n");
	int vpn = (unsigned) virtAddr / PageSize;
	int nextPhysPage=bitMap.Find();
	if(nextPhysPage==-1){
		nextPhysPage=nextExchangedPhyPage++;
		nextExchangedPhyPage%=NumPhysPages;
	}
	printf("currentExecutable=%d nextPhysPage=%d.",currentThread->space->currentExecutable,nextPhysPage);
	currentThread->space->currentExecutable->ReadAt(&(machine->mainMemory[nextPhysPage*PageSize]),PageSize,currentThread->space->noffH.code.inFileAddr+vpn*PageSize);
	machine->pageTable[vpn].physicalPage = nextPhysPage;
	printf("虚拟页号vpn=%d\n phyPage=%d",vpn,nextPhysPage);
	machine->pageTable[vpn].valid = TRUE;
	machine->pageTable[vpn].use = TRUE;
	machine->pageTable[vpn].dirty = FALSE;
	machine->pageTable[vpn].readOnly = FALSE;
	int nextIndex=FindNextTlbIndex();
	machine->tlb[nextIndex]=machine->pageTable[vpn];
	return ;
}

void
PageFaultHandler(){
		int virtAddr = targetVirtAddr;
		printf("处理缺页错误,virtAddr = %d",virtAddr);
		int vpn = (unsigned) virtAddr / PageSize;
		TranslationEntry *currentPageTable = machine->pageTable;
		//如果也页表中有相应的物理页，则更新tlb即可，否则请求调页。
		if(currentThread->space != NULL && machine->tlb != NULL && currentPageTable[vpn].valid){
			/* tlb 先进先出置换算法
			machine->tlb[nextTlbIndex]=currentPageTable[vpn];
			nextTlbIndex++;
			nextTlbIndex%=TLBSize;
			*/
			//TLB 时钟置换算法
			int nextIndex=FindNextTlbIndex();//与先进先出的nextTlbIndex不一样。
			machine->tlb[nextIndex]=currentPageTable[vpn];
			return ;
		}
		else{
			DemandingPage(virtAddr);
			return ;
		}
}
void
ExceptionHandler(ExceptionType which)
{
  int type = machine->ReadRegister(2);
	switch(which){
		case SyscallException:
			if(type==SC_Halt){
				DEBUG('a', "Shutdown, initiated by user program.\n");
   				interrupt->Halt();
			}
			else{
				printf("Unexpected user mode exception %d %d\n", which, type);
				ASSERT(FALSE);
			}
			break;
		case PageFaultException:
			pageFaultCounter++;
			PageFaultHandler();
			printf("总的缺页次数为：%d次。",pageFaultCounter);
			break;
	}
}
