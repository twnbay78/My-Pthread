#ifndef MALLOC_H
#define MALLOC_H


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h> 
#include <sys/mman.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "my_pthread_t.h"


#define malloc(x) myallocate(x, __FILE__, __LINE__)
#define free(x) mydeallocate(x, __FILE__, __LINE__)

#define THREADREQ 1
#define TOTAL_MEM 8388608	//8mb = 8388608, 2048 total pages
#define NUM_OF_TABLE_PAGES 15
#define NUM_OF_SHARED_PAGES 111
#define NUM_OF_PAGES 1920
#define META_SIZE sizeof(block)
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define PAGE_TABLE_ENTRY_SIZE sizeof(membook)
#define PAGE_TABLE_SIZE NUM_OF_TABLE_PAGES * PAGE_SIZE
#define TEMP_PAGE_SIZE PAGE_SIZE
#define TCB_TABLE_SIZE PAGE_SIZE
#define SHARED_MEMORY_SIZE NUM_OF_SHARED_PAGES * PAGE_SIZE
#define MASTER_TEMP (void*)&mem + PAGE_TABLE_SIZE
#define TEMP_PTR (void*)&mem + PAGE_TABLE_SIZE
#define MEMORY_PTR (void*)&mem + PAGE_TABLE_SIZE + TEMP_PAGE_SIZE + SHARED_MEMORY_SIZE
#define TCB_PTR (void*)&mem + PAGE_TABLE_SIZE + TEMP_PTR
#define swapStart 1048576
#define segSwap 1

// - - - - - - - - STRUCTS - - - - - - - - - //
// struct that triggers signal handler
struct sigaction sa_mem; 

/*
 * Malloc Struct
 * sizeof(struct _block) == 24
 */
typedef struct _block {
	size_t size;
	struct _block* next;
	short int free;
	short int TID;
} block;

typedef struct _membook {
	int TID;
	int page_num;
	int isFree;
	short int isSpan;
	struct _membook* next;
	struct _block* page;
} membook;

typedef struct _t_info {
	int TID;
	unsigned short int num_of_pages;
}t_info;

// NOT USED IN NEW IMPLEMENTATION
typedef struct _swapbook {
	int TID;
	short int isFree;
} swap_block;

typedef struct _master {
	int tid;
}master;
// - - - - - - - - METHOD DECLARATIONS - - - - - - - - - //

void* myallocate(int size,char FILE[],int LINE);
void* findSpace(size_t size);
struct _block* reqSpace(size_t size);
struct _block *get_block_ptr(void *ptr);
static void handler(int sig, siginfo_t *si, void *unused);
void print_page_table_entry(membook* entry);

//extern my_pthread_t* Master;

#endif