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

// - - - - - - - - STRUCTS - - - - - - - - - //
// struct that triggers signal handler
struct sigaction sa_mem; 

/*
 * Malloc Struct
 * sizeof(struct _block) == 24
 */
typedef struct _block {
	size_t size;
	void* next;
	int free;
} block;

typedef struct _membook {
	int TID;
	int page_num;
	int isFree;
	short int isSpan;
	struct _membook* next;
	struct _block* page;
} membook;

typedef struct _swapbook {
	int TID;
	size_t size;
	void* next;
	int isFree;
	int isSpan;
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