#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <malloc.h>
#include "malloc.h"

char mem2[8388608];

void fileCreate()
{
	int Fd = open("swapfile.txt", O_RDWR | O_TRUNC | O_CREAT);
	off_t offset = lseek(Fd,(off_t)16777216, SEEK_SET);

	if(Fd == -1){
		fprintf(stderr, "File is not open, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if(offset == -1){
		fprintf(stderr, "Could not lseek file, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	write(Fd,"/0",1);

	close(Fd);
}

//CURSOR MOVEMENT

void moveCurABS(int Fd, int dist)
{
	if(Fd == -1){
		fprintf(stderr, "File is not open, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	off_t offset = lseek(Fd,(off_t)dist, SEEK_SET);
	
	if(offset == -1){
		fprintf(stderr, "Could not lseek file, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}
void moveCur(int Fd, int dist)
{
	if(Fd == -1){
		fprintf(stderr, "File is not open, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	off_t offset = lseek(Fd,(off_t)dist, SEEK_CUR);
	
	if(offset == -1){
		fprintf(stderr, "Could not lseek file, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}
void resetFilePointer(int Fd)
{
	off_t offset = lseek(Fd,(off_t)swapStart, SEEK_SET);
	if(Fd == -1){
		fprintf(stderr, "File is not open, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if(offset == -1){
		fprintf(stderr, "Could not lseek file, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}
void resetABS(int Fd)
{
	off_t offset = lseek(Fd,(off_t)0, SEEK_SET);
	if(Fd == -1){
		fprintf(stderr, "File is not open, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if(offset == -1){
		fprintf(stderr, "Could not lseek file, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void movePage(int Fd)
{
		if(Fd == -1){
		fprintf(stderr, "File is not open, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	off_t offset = lseek(Fd,(off_t)PAGE_SIZE, SEEK_CUR);
	
	if(offset == -1){
		fprintf(stderr, "Could not lseek file, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}
// READ WRITE TO PAGES
void readPage(int Fd,void * address)
{
	int off = 0;
	if(Fd == -1){
		fprintf(stderr, "File is not open, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	while(off<PAGE_SIZE)
	{
	off += read(Fd,address+off,TEMP_PAGE_SIZE-off);
	}
	fsync(Fd);
	moveCur(Fd,-1*PAGE_SIZE);
	
	
}
void readAmount(int Fd,void * address,int amount)
{
	int off = 0;
	if(Fd == -1){
		fprintf(stderr, "File is not open, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	while(off<PAGE_SIZE)
	{
	off += read(Fd,address+off,amount-off);
	}
	fsync(Fd);
	moveCur(Fd,-1*amount);

}


void writePage(int Fd,void* address)
{
	int off = 0;
	if(Fd == -1){
		fprintf(stderr, "File is not open, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	while(off<PAGE_SIZE)
	{
	off += write(Fd,address+off,TEMP_PAGE_SIZE-off);
	}
	fsync(Fd);
	moveCur(Fd,-1*PAGE_SIZE);

}

//Swap
void swap(int Fd,void * addrRec,void* addrTemp,int swapFDist)
{
	resetFilePointer(Fd);
	moveCur(Fd,swapFDist*PAGE_SIZE);
	for (int i = 0; i < segSwap ;i++)
	{
		readPage(Fd,addrTemp); //Read swap to temp
		writePage(Fd,addrRec); //Read memory to swap
		memcpy(addrRec,addrTemp,PAGE_SIZE); //Copy temp to memory
		movePage(Fd); //Move to next page in swap
		addrRec+=PAGE_SIZE; //Move to next page in memory
	}
}

int findFreeSwap(int Fd,void* addrTemp)
{
	int pageCounter = 0;
	block* checkMeta;
	short int flip = 0;
	if(Fd == -1){
		fprintf(stderr, "File is not open, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	while(Fd != 16777216)
	{
		readAmount(Fd,addrTemp,sizeof(block));
		checkMeta = (block*)addrTemp;
		if(checkMeta->free == 0)
			return (4-((int)(3840/pageCounter)));
		pageCounter++;
	}
	return -1;
}

int findPageSwap(int Fd,void* addrTemp,int TID)
{
	int pageCounter = 0;
	block* checkMeta;
	short int flip = 0;
	if(Fd == -1){
		fprintf(stderr, "File is not open, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	while(Fd != 16777216)
	{
		readAmount(Fd,addrTemp,sizeof(block));
		checkMeta = (block*)addrTemp;
		if(checkMeta->TID == TID)//Master->current)
			return (4-((int)(3840/pageCounter)));
		pageCounter++;
	}
	return -1;
}
/*r
int main(int argc, char const *argv[])
{
	fileCreate();
	block a;
	int Fd = open("swapfile.txt", O_RDWR);
	

	close(Fd);

		return 0;
}*/