#include "malloc.h"

#define malloc(x) myallocate(x, __FILE__, __LINE__)
#define free(x) mydeallocate(x, __FILE__, __LINE__)

#define THREADREQ 1
#define TOTAL_MEM 8388608	//8mb = 8388608, 2048 total pages
#define NUM_OF_TABLE_PAGES 15
#define NUM_OF_SHARED_PAGES 112
#define NUM_OF_PAGES 1920
#define META_SIZE sizeof(block)
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)
#define PAGE_TABLE_ENTRY_SIZE sizeof(membook)
#define PAGE_TABLE_SIZE NUM_OF_TABLE_PAGES * PAGE_SIZE
#define TEMP_PAGE_SIZE 4096
#define SHARED_MEMORY_SIZE NUM_OF_SHARED_PAGES * PAGE_SIZE
#define MASTER_TEMP (void*)&mem + PAGE_TABLE_SIZE
#define TEMP_PTR (void*)&mem + PAGE_TABLE_SIZE
#define MEMORY_PTR (void*)&mem + PAGE_TABLE_SIZE + TEMP_PAGE_SIZE + SHARED_MEMORY_SIZE

char mem[TOTAL_MEM];
static void* memory = mem;
membook* page_table = NULL;
//static int start = 0;
//static int curr = 0;
//static int numThreads = 0;
int first_time = 0;
master* Master;

//error function to print message and exit
void fatalError(int line, char* file) {
	printf("Error:\n");
	printf("ERROR= %s\n", strerror(errno));
	printf("Line number= %i \nFile= %s\n", line, file);
	exit(EXIT_FAILURE);
}

// Used for debugging purposes
void printBlock(block* b) {
	printf("Size: %zu | Free: %d\n",
		b->size, b->free);
}

void printList(void* global_base) {

	block* ptr = global_base;
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

//Creates Swap File
void fileCreation()
{
	int Fd = open("swapfile.txt", O_RDWR | O_TRUNC | O_CREAT);
	off_t offset = lseek(Fd,(off_t)16777216, SEEK_SET);

	;
	if(Fd == -1){
		fprintf(stderr, "Could not create file, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if(offset == -1){
		fprintf(stderr, "Could not lseek file, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	write(Fd,"/0",1);

	close(Fd);

}

int unProtectMem()// function to un protect mem
{
	
	if(mprotect(memory,2048*PAGE_SIZE,PROT_READ | PROT_WRITE) == -1){
		fprintf(stderr, "Could not unprotect mem, Error num: %d, Error msg: %s\n", errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	return 0;

}


int protectMem() //function to protect mem
{

	unProtectMem();
	printf("\nunprotected memory - ready to protect\n");

	// For memprotect, we need start of thead's first page and total size of all contiguously owned pages (addr and size)
	int pages_owned = 0;
	void* addr;
	size_t size;
 	 // start of page table - ready to traverse
	membook* ptr = page_table;
	printf("page_table addy: %p\n", &page_table);
	printf("mem addy: %p\n", &mem);
	printf("ptr addy: %p\n", ptr);

	if(ptr->TID == Master->tid){
		printf("first page belongs to thread - setting addy\n");
		addr = (void*)ptr->page;
		printf("addr: %p\n", addr);
		print_page_table_entry(ptr);
		pages_owned++;
		ptr = ptr->next;
		while(ptr->TID == Master->tid){
			print_page_table_entry(ptr);
			pages_owned++;
			ptr = ptr->next;
		}
	}

	printf("pages owned: %d\n", pages_owned);


  // Calculate how much memory we need to protect 
	size = pages_owned * PAGE_SIZE;

  // If the thread owns any pages, memprotect those pages
	if(size > 0)
	{
		int i = mprotect(addr, size, PROT_NONE);
		if(i==-1)
		{
			perror("Mprotect Fail");
			fatalError(__LINE__,__FILE__);
		}
	}
	return 0;
}

// Function to move all free pages in memory to the front of memory
// returns the number of free pages 
// FURTHER OPTIMIZATION: store ptr's of each free page in an array of ptrs and don't swap pages unless there are enough free pages to satisfy the swap
int mem_mov_free(){
	// distance between start of memory and current ptr to left-most unfree'd page
	int pd = 0;
	int first_iter = 1;
	membook* ptr = page_table;
	while(ptr != NULL){
		// case for the first page being free
		if(first_iter == 1 && ptr->isFree == 1){
			pd++;
		}
		else if(ptr->isFree == 1){
			swap_in_memory(ptr->page, MEMORY_PTR + (pd * PAGE_SIZE));
			pd++;
		}
		ptr = ptr->next;
	}
}

void swap_mov_free(){

}

void swap_in_file(void* page1, void* page2,int pd){

	memcpy(TEMP_PTR, page2, PAGE_SIZE);
	memcpy(page2, page1+pd, PAGE_SIZE);
	memcpy(page1+pd, TEMP_PTR, PAGE_SIZE);
}

/* Function to swap pages within memory
 Takes in two addresses, the front of each page we want to swap
 Will use temp page to store intermident data
*/
void swap_in_memory(void* page1, void* page2){
	// Perform swaps
	memcpy(TEMP_PTR, page2, PAGE_SIZE);
	memcpy(page2, page1, PAGE_SIZE);
	memcpy(page1, TEMP_PTR, PAGE_SIZE);

	// Change the ptrs to memory located in the page table entries
	membook* ptr = page_table;
	while(ptr != NULL){
		if(ptr->page == page1){
			ptr->page = page2;
		}else if (ptr->page == page2){
			ptr->page = page1;
		}
	}

}

/* FUNCTION TO RETRIEVE THREAD'S PAGES AND PUT THEM IN THE FRONT POSITION OF MEMORY
	CASE 1: Thread's pages are in memory
		- traverse page table
		- when we find a page that belongs to the thread, move it to the front + page_distance
	CASE 2: Thread's pages are in the swap file
		- Traverse swap file and perform CASE 1 algo
		- Swap thread's pages with free pages in memory
	CASE 3: Thread doesn't own any pages
		- return -1 and let findSpace allocate new page  
*/
int memAllignPages() //Function to call when there is a page fault
{
	// distance between front of memory and current position (aka left-most page not owned by thread)
	int pd = 0;
	// Find out if the thread's pages are in memory,
	// If so, move them to the front
	membook* ptr = page_table;
	while (ptr != NULL){
		if(ptr->TID == Master->tid){
			// Start moving pages to the front
			swap_in_memory(ptr->page, MEMORY_PTR + (pd * PAGE_SIZE));
			pd++;
		}
		ptr = ptr->next;
	}

	// If there were no threads found, must check swap file - if threads were found, just return
	if(pd == 0){
		int blockCount = 0;
		int pd = 0;
		int fp = open("swapfile.txt",O_RDWR);
		lseek(fp, 0, SEEK_SET);
		swap_block* block = TEMP_PTR;
		while(read(fp,block,PAGE_SIZE) > 0 && blockCount < 4095)
		{
			if(block->TID == Master->tid)
			{
   			   //write Temp
				lseek(fp,0,SEEK_SET);
				lseek(fp,(off_t)4095*PAGE_SIZE,SEEK_SET);
				write(fp,block,PAGE_SIZE);

			//write start +pd to spot
				lseek(fp,0,SEEK_SET);
				lseek(fp,(off_t)pd*PAGE_SIZE,SEEK_SET);
				read(fp,block,PAGE_SIZE);

				lseek(fp,0,SEEK_SET);
				lseek(fp,(off_t)blockCount*PAGE_SIZE,SEEK_SET);
				write(fp,block,PAGE_SIZE);

			//write temp to start + pd
				lseek(fp,0,SEEK_SET);
				lseek(fp,(off_t)4095*PAGE_SIZE,SEEK_SET);
				read(fp,block,PAGE_SIZE);

				lseek(fp,0,SEEK_SET);
				lseek(fp,(off_t)pd*PAGE_SIZE,SEEK_SET);
				write(fp,block,PAGE_SIZE);
				pd++;
			}
			blockCount++;
			lseek(fp,(off_t)blockCount*PAGE_SIZE,SEEK_SET);
		}
		
		int free_mem_pages = mem_mov_free();
		if(free_mem_pages > pd && pd != 0){
			// HERE IS WHERE WE SWAP THE PAGES FROM THE FRONT OF THE SWAP FILE TO THE FRONT OF THE MEMORY
		}

		close(fp);
		if(pd > 0){
			return 1;
		}
	}else if (pd > 0){
		return 1;
	}

	// At this point, no pages were found in the swap file 
	return -1;
}

// Given a page table entry, this function prints
// out all of the page table entry fields
// Used for debugging findSpace and page table traversal
void print_page_table_entry(membook* entry){
	printf("\n- - - - - - - - - - - - -\n");
	printf("TID: %d\n", entry->TID);
	printf("page num: %d\n", entry->page_num);
	printf("isFree: %d\n", entry->isFree);
	printf("isSpan: %u\n", entry->isSpan);
	printf("this page entry: %p\n", entry);
	printf("next page entry: %p\n", entry->next);
	printf("mem address of page start: %p\n", entry->page);
	printf("- - - - - - - - - - - - -\n\n");
}

// Function to give a thread another page 
void add_page(){

}


/* Function to give a thread a ptr to space
  CASE 1: The thread already owns a page and it's in the front position of memory
   - Advance next* ptr by the size of allocation request
   - Check to see if next* overflows into next page
   - If next* overflows, give the thread another contiguous page
   - If it doesn't overflow, return next*

  CASE 2: The thread owns a page but it's page is out of position
   - Call memalign to get all of the thread's pages in the front position
   - Traverse metadata blocks until you find an open block and give block to user

  CASE 3: Memallign returns NULL and the thread doesn't own any pages
   - Find a free page and put it in the front position

 */

void* findSpace(size_t size) {
	block* block_ptr;
	membook* table_ptr = page_table;

	// CASE 1
	if(table_ptr->TID == Master->tid){
		block_ptr = (block*)table_ptr->page
	}
	
	int memAllign_ret_value = memAllignPages();

	// CASE 2 - pages are now alligned
	if(memAllign_ret_value == 1){
		block_ptr = (block*)table_ptr->page;
		while(block_ptr != NULL){
			if(block_ptr->free == 1){
				// found free block - setup metadata
				block_ptr->free = 0;
				block_ptr->size = size;
				block_ptr->next = (void*)block_ptr + size;
				// setup next metadata
				block_ptr->next->isFree = 1;
				return (void*)block_ptr;
			}
		}
		// At this point, there is no free block and we must give the thread another page

	}
	
	// CASE 3 - give thread a free page + write metadata block for request
	else if(memAllign_ret_value == -1){
		membook* ptr = page_table;
		while (ptr != NULL){
			if(ptr->isFree == 1){
			// set page table entry
				ptr->isFree = 0;
				ptr->TID = Master->tid;
				print_page_table_entry(ptr);
			// write metadata
				block* metadata = (block*)ptr->page;
				metadata->size = size;
				metadata->free = 0;
				metadata->next = (void*)metadata + size;
			// setup next metadata block
				metadata->next->free = 1;
				return ptr->page + sizeof(block);
			}
			ptr = ptr->next;
		}o
	}
	// Could not find a free block :(
	return NULL;

}


block* reqSpace(size_t size) {

	// // Ask for size of our actual malloc and our block struct
	// // block* metadata = sbrk1(size + META_SIZE);

	// // Used for sbrk() system call. sbrk1() takes care of this error.
	// // sbrk() returns (void*) -1 when request does not go through
	// // if (metadata == (void*) -1) {
	// // 	return NULL;
	// // }

	// printf("Request successful\n\n");

	// //Setup metadata inside block
	// metadata->size = size;
	// metadata->next = NULL;
	// metadata->free = 0;

	return NULL;
}

/*
 * Given a pointer, will find the block and set it's free flag to 1.
 FURTHER OPTIMIZATIONS
 - actually coalesce
 - explicit free lists
 - buddy system allocator
 */
void mydeallocate(void *ptr, char FILE[], int LINE) {
	if (!ptr) {
		return;
	}
	
	membook* page_ptr = page_table;
	membook* page_ptr_next = page_ptr->next;
	while(page_ptr_next! = NULL)
	{
		if(page_ptr->TID == Master->tid)
		{
			block* b1 = (block*)page_prt->page
			block* b2 = (block*)page_prt_next->page

			if(ptr > b2-b1)
			{
						//memset and other bit freeing
				b1->free = 1;

			}
		}
		page_ptr = page_ptr->next;
		page_ptr_next = page_ptr->next;
	}
}

// Special function to add all page entries to the page table
// Only used once during the first malloc request
void add_page(membook* ptr, int page_num, void* mem_ptr){
	membook* new_entry = ptr;
	new_entry->TID = -1;
	new_entry->isFree = 1;
	new_entry->isSpan = 0;
	new_entry->page_num = page_num;
	new_entry->page = mem_ptr;
	new_entry->next = page_table;
	page_table = new_entry;
}


// Function to instatiate the page table in the begining of memory
// Page table will store pointers to the begining of each page (that will be given to users)
// Page table is a LL
void initialize_page_table(){

	printf("size of page table entry %ld\n", PAGE_TABLE_ENTRY_SIZE);
	printf("will take up %ld pages\n", PAGE_TABLE_SIZE / PAGE_SIZE);
	printf("original memory address: %p\n", &mem);

	int i;
	for(i = NUM_OF_PAGES-1; i >= 0; i--){
		//printf("page_table_entry_size * i: %ld\n", PAGE_TABLE_ENTRY_SIZE*i);
		add_page((membook*)&mem + i, i, (void*)&mem + PAGE_TABLE_SIZE + TEMP_PAGE_SIZE + SHARED_MEMORY_SIZE +(i*PAGE_SIZE));
	}
	printf("\n");
}

/*
 * Malloc Implementation
 * TODO: thread malloc stuff
 */
void* myallocate(int size, char FILE[], int LINE) {

	//Error if our malloc'd size is either nothing or negative.
	if (size <= 0) {
		return NULL;
	}


	if(first_time == 0)
	{

		sa_mem.sa_flags = SA_SIGINFO;
		sigemptyset(&sa_mem.sa_mask);
		sa_mem.sa_sigaction = handler;

		first_time =1;

		memory = memalign(PAGE_SIZE,2048*PAGE_SIZE);
		//int x = posix_memalign((void*)&mem,PAGE_SIZE,TOTAL_MEM);
		if(memory < 0) fatalError(__LINE__, __FILE__);
		

		// initialize page table
		printf("\ninitializing page table\n");
		initialize_page_table();
		// creating swap file
		fileCreation();


	}

	block* metadata;
	// Finds space for allocation request
	printf("\nlooking through page table for free pages\n");
	metadata = findSpace(size);
	printf("attempting to write metadata in requested page\n");
	metadata->size = size;
	metadata->next = metadata + sizeof(block) + size;
	metadata->free = 0;


	//if we can't find any free blocks, we're going to have to request
	//space from the swap file
	if (metadata == NULL) {
		metadata = reqSpace(size);	//request space from "OS"		
	}

	//Return metadata+1, since metadata is pointer of type block,
	//we increment the size of one sizeof(block) to return the pointer
	//to the beginning of where we can start writing data.
	int i = protectMem();
	printf("protectMem ret val: %d\n", i);

	return (void*)metadata + sizeof(block);
}

block *get_block_ptr(void *ptr) {
	return (block*)ptr - 1;
}

// Sig handler to catch page faults
static void handler(int sig, siginfo_t *si, void *unused) {
	printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);

	membook* ptr = (membook*)&mem;
  // If the first page already belongs to the thread, let the thread do its thaaang
	if(ptr->TID == Master->tid){
		unProtectMem();
  } else { // else, grab pages for user and put them in the front
  	memAllignPages();
  }
}

int main() {


	Master = (master*)MASTER_TEMP;
	Master->tid = 2;

	printf("Beginning Mem Array:\n");
	printRawMem(0, 15);
	printf("\n");

	/* Initial memory allocation */
	char* str = (char *) malloc(sizeof(char)*15);
	strcpy(str, "tutorialspoint");
	printf("String = %s,  Address = %p\n", str, str);

	
	printf("---------\n");

	char* str1 = (char *) malloc(sizeof(char)*2);
	strcpy(str1, "th");
	printf("String = %s,  Address = %p\n", str1, str1);

	free(str);

	printf("\nList:------------------------\n");
	printRawMem(0, 40);

	return 0;
}
