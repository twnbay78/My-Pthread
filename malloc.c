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

#define malloc(x) myallocate(x, __FILE__, __LINE__)
#define free(x) mydeallocate(x, __FILE__, __LINE__)

#define TOTAL_MEM 8388608	//8mb = 8388608
#define META_SIZE sizeof(struct _block)

char mem[TOTAL_MEM];
char* memory = mem;
void* global_base = NULL;
static int start = 0;
static int curr = 0;
static int numThreads = 0;

// System page size: sysconf(_SC_PAGE_SIZE) == 4kb
// 8MB char array/4kb page size = 2048 pages

/*
 * Malloc Struct
 * sizeof(struct _block) == 24
 */
struct _block {
	size_t size;
	struct _block* next;
	int free;
	int threadOwner;
} block;

void* myallocate(int size,char FILE[],int LINE);
struct _block* findSpace(struct _block** last, size_t size);
struct _block* reqSpace(struct _block* last, size_t size);
struct _block *get_block_ptr(void *ptr);

// Used for debugging purposes
void printBlock(struct _block* b) {
	printf("Size: %zu | Free: %d | tid: %d\n",
		b->size, b->free, b->threadOwner);
}

void printList(void* global_base) {

	struct _block* ptr = global_base;
	while (ptr != NULL) {
		printBlock(ptr);
		ptr = ptr->next;
	}
}

//Prints the 8mb memory out as %d. Parameter indicates range
void printRawMem(int start, int end) {

	while (start != end) {
		printf("%d, ", mem[start]);
		start++;
	}

	printf("\n");
}

/*
 * skbrk() is a system call that malloc uses to request/allocate memory.
 * This function is similar to that call, but requests memory from our 8mb array.
 */
void *sbrk1(int size)
{	
	printf("sbrk1: allocating\n");
	void* base;
	int index;

	//If the size is just 0, our pointer in the array stays the same
	if (size == 0)
		return (void*)memory;

	//Check if our index pointer and our requested size will go over 8mb
	if( (curr + size) > 8388608 )
		return NULL;

	//If all conditionals pass, then we can increment the pointer
	index = curr;
	curr = curr + size;
	base = (void*)(memory+index);

	printf("Index: %d | Size: %d | Curr: %d\n", index, size, curr);
	
	//Return a pointer to the front of the allocated memory.
	return base;
}

/*
 * Iterate through the linked list to see if there is a block that's large enough.
 * Function returns a pointer to the block in our linked list that can be allocated.
 * This function finds a free block that we have already allocated before in the past.
 * If we cannot find such block, we call reqSpace() to request for another block of memory.
 */
struct _block* findSpace(struct _block **last, size_t size) {

	struct _block* current = global_base;

	//Iterate through the list, and return pointer to front of free space
	while (current && !(current->free && current->size >= size)) {
		*last = current;
		current = current->next;
	}

	//Return the block that has free space.
	return current;
}

/*
 * If we don't find a free block, request space from the OS via "sbrk1"
 * and add new block to end of linked list
 */
struct _block* reqSpace(struct _block* last, size_t size) {

	//Ask for size of our actual malloc and our block struct
	struct _block* metadata = sbrk1(size + META_SIZE);

	//Used for sbrk() system call. sbrk1() takes care of this error.
	//sbrk() returns (void*) -1 when request does not go through
	//if (metadata == (void*) -1) {
	//	return NULL;
	//}

	printf("Request successful\n\n");

	if (last) { //Null on first request
		last->next = metadata;
	}

	//Setup metadata inside block
	metadata->size = size;
	metadata->next = NULL;
	metadata->free = 0;

	return metadata;
}

/*
 * Given a pointer, will find the block and set it's free flag to 1.
 */
void mydeallocate(void *ptr, char FILE[], int LINE) {
  if (!ptr) {
    return;
  }

  // TODO: consider merging blocks once splitting blocks is implemented.
  struct _block* block_ptr = get_block_ptr(ptr);
  block_ptr->free = 1;
}

/*
 * Malloc Implementation
 * TODO: thread malloc stuff
 */
void* myallocate(int size, char FILE[], int LINE) {

	struct _block* metadata;

	//Error if our malloc'd size is either nothing or negative.
	if (size <= 0) {
		return NULL;
	}

	// First call to place the first block into the linked list.
	if (global_base == NULL) {	//First call
		
		printf("Base is null. First malloc:\n");
		metadata = reqSpace(NULL, size);	//request space from "OS"	
		
		if (!metadata) {		//check if allocated successfully
			return NULL;
		}

		printBlock(metadata);	//debug
		global_base = metadata;	//store to linked list
	}

	// After we create the first block, we will always default to this.
	else {
		printf("non-first\n");
		struct _block* last = global_base;

		//We have a pointer pointing to the linked list, and we need to
		//search for a free block
		metadata = findSpace(&last, size);

		//If we do not have such free block in the linked list, we need to request it
		if (!metadata) {
			metadata = reqSpace(last, size);
			if (!metadata) {
				return NULL;
			}
		}

		//If we do have such free block, set metadata free flag to 0.
		else {	//found free block
			metadata->free = 0;
		}
	}

	//Return metadata+1, since metadata is pointer of type struct _block,
	//we increment the size of one sizeof(struct _block)
	return (metadata + 1);
}

struct _block *get_block_ptr(void *ptr) {
 	return (struct _block*)ptr - 1;
}

int main() {

	printf("Beginning Mem Array:\n");
	printRawMem(0, 15);
	printf("\n");

	/* Initial memory allocation */
	char* str = (char *) malloc(15);
	strcpy(str, "tutorialspoint");
	printf("String = %s,  Address = %u\n", str, *str);

	//Print status so far
	printList(global_base);
	printf("---------\n");

	char* str1 = (char *) malloc(2);
	strcpy(str1, "th");
	printf("String = %s,  Address = %u\n", str1, *str1);

	free(str);

	printf("\nList:------------------------\n");
	printList(global_base);
	printRawMem(0, 15);

	return 0;
}
