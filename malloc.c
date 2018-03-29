#include "malloc.h"
#include "thread.h"

char mem[TOTAL_MEM];
static void* memory = mem;
membook* page_table = NULL;
int first_time = 0;
master* Master;
static ucontext_t uctx_main, uctx_func1, uctx_func2;

/* OPTIMIZATIONS (back-burner)
	- Write coalescing algorithm discussed
	- allow allocation requests > 1 page size
	- Write shared memory initialization
	- Write shalloc
	- LESS IO
	- Free up space -> eventually to allow threads to own more pages
	
	- CACHEABLE SEGMENTS
		- Since our memory manager presumes the existence of our scheduler, our memory manager will know what the next thread to be executed will be.
		|
		Let's face it, IO operations are SLOW. If we are going to optimize anything, it will involve making optimizing concurrency with our swap file 
		| 
		Since our memory manager manages its pages in segments, we can theoretically cache our next thread in our second segment of memory (since our first segment contains the currently running thread's pages, we can't touch it) - this will increase response time greatly because it will not be limited by the swap from disk as much - our next scheduled thread's pages will ALWAYS be in memory by the time it even gets a chance to request memory
*/

//	-	-	-	-	- DEBUG/PRINT FUNCTIONS	-	-	-	-	//

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

// Function to test memory initialization
void print_init_memory (){
	membook* ptr = page_table;
	int count = 0;

	printf("size of page table entry %ld\n", PAGE_TABLE_ENTRY_SIZE);
	printf("will take up %ld pages\n", PAGE_TABLE_SIZE / PAGE_SIZE);
	printf("original memory address: %p\n", &mem);
	ptrdiff_t ptdiff1 = (char*)page_table - (char*)&mem;
	printf("distance between page table and start of mem: %td\n", ptdiff1);

	while(ptr != NULL){
		if(count < 3){
			print_page_table_entry(ptr);
			count++;
		}
		ptr = ptr->next;
	}


}

//	-	-	-	-	- INITIALIZATION FUNCTIONS	-	-	-	-	//

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

//	-	-	-	-	- SWAP FILE FUNCTIONS	-	-	-	-	//

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

/* Function to locate a free page in the swap file and return the page distance from the start of the start of the first pageable segment in memory 
*/
int findFreeSwap(int Fd,void* addrTemp)
{
	int pageCounter = 0;
	block* checkMeta;
	//short int flip = 0;
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
	//short int flip = 0;
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

//	-	-	-	-	- CORE HELPER FUNCTIONS	-	-	-	-	//

int protectMem() //function to protect mem
{

	unProtectMem();
	printf("in unProtectMem() function - ready to protect\n");

	// For memprotect, we need start of thead's first page and total size of all contiguously owned pages (addr and size)
	int pages_owned = 0;
	void* addr;
	size_t size;
 	 // start of page table - ready to traverse
	membook* ptr = page_table;
	//printf("page_table addy: %p\n", &page_table);
	//printf("mem addy: %p\n", &mem);
	//printf("ptr addy: %p\n", ptr);

	if(ptr->TID == Master->tid){
		printf("first page in memory belongs to thread, getting end ptr of pages\n");
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
		printf("calling mprotect\n");
		int i = mprotect(addr, size, PROT_NONE);
		if(i == -1)
		{
			perror("Mprotect Fail");
			fatalError(__LINE__,__FILE__);
		}
	}
	printf("returning from memprotect\n\n");
	return 0;
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
			printf("page 1 pre-swap: %p\n", ptr->page);
			ptr->page = page2;
			printf("page 1 post-swap: %p\n", ptr->page);
		}else if (ptr->page == page2){
			printf("page 2 prev: %p\n", ptr->page);
			ptr->page = page1;
			printf("page 2 prev: %p\n", ptr->page);
		}
	}

}

/* FUNCTION TO RETRIEVE THREAD'S PAGES AND PUT THEM IN THE FRONT POSITION OF MEMORY - FUNCTION ASSUMES THE THREAD HAS NOT EXCEEDED MAXIMUM ALLOCATION SIZE AND MAXIMUM PAGE COUNT

Our algorithm guarentees that a thread's pages will be found in a single segment (960 pages), so we can swap segments of memory/swap file

	CASE 1: Thread's pages are in memory
		- traverse page table
		- when we find a page that belongs to the thread, calculate which segment of memory it is in
		- if the segment is in the first half of memory, perform necessary swaps to get the contiguous pages in the front of memory
		- If the segment is in the scond half of memory, perform a swap of the first segment and the second segment, then perform thenecessary swaps to get the contiguous pages in the front of memory
		- Sort the thread's contiguosly alligned pages by page number -> since page numbers are maintained throughout the program, the thread's page order will always be increasing in page number
	CASE 2: Thread's pages are in the swap file
		- Traverse swap file and locate thread's pages
		- Calculate which segment the thread's pages are in and perform a swap with the first segment in memory 
		- Sort the thread's contiguously alligned pages by page number > since page numbers are maintained throughout the program, the thread's page order will always be increasing in page number
	CASE 3: Thread doesn't own any pages
		- return -1 and let findSpace allocate new page  

RETURN VALUES

	1 -> the thread's pages are now contiguously alligned in the front of memory
	-1 -> the thread does not own any pages in the swap file OR memory
*/

int memAllignPages() //Function to call when there is a page fault
{
	printf("in memAllignPages() function\n");
	printf("returning from memAllignPages() function\n\n");
	return -1;
}
	/*
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
}*/


/* Function to give a thread another page - assumes

 FUTURE OPTIMIZATIONS
 - as of now, we traverse to find the last contiguous page owned by the current thread even if there is no free page available - don't traverse unless there is a free page

 Returns -1 if it cannot find a page

 CASE 1: Free page is in memory
 	- Find free page
 	- we swap it with the left-most page that is not owned by the current thread
 	- Initialize metadata for the new requested page
 	- Return 1 indicating successful grab from memory

 CASE 2: Free page is in the swap file
 	- Call findFreeSwap() that returns the distance from the start of the segmented swawp file region and the start of the free page
 	- Calculate which of the four segments of the swap file the free page lies in
 	- Swap the segment with the second segment of memory (because the first segment holds the pages that belong to our current running thread)
 	- Initialize metadata for the requested page
 	- Return 1 indicating successful grab from swap file

 CASE 3: There is no free page in memory or the swap file

 */
int give_new_page(){
	printf("in give_new_page() function\n");
	/*
	membook* ptr = page_table;
	//void* page1;
	//void* page2;
	int page_count = 1;

	// find last owned contiguous page by current thread
	while(ptr->TID == Master->tid){
		ptr = ptr->next;
	}

	// CASE 1: Free page is in memory - let's find out!!!
	while(ptr != NULL){
		if(ptr->isFree == 1){

		}
		ptr = ptr->next;
		page_count++;
	}


	// Thread owns a page - need to traverse to last owned
	if(ptr->TID == Master->tid){
		while(ptr->TID == Master->tid){
			ptr = ptr->next;
		}
	}*/
	printf("Returning from give_new_page() function\n\n");
	return -1;
}


/* Function that assumes the current thread owns a page in the front of memory and tries to alocate space in the page for the thread

RETURN VALUES: 
	NULL -> there is no free page available for the thread
	!NULL -> space allocated for thread within it's last non-full page, ptr to the space (after it's metadata) is returned

*/
void* give_allocation(size_t size){
	printf("in give_allocation() function\n");
	membook* table_ptr = page_table;
	int curr_page_size = sizeof(block);
		// go to last allocated block of page
	block* block_ptr = (block*)table_ptr->page;
	while(block_ptr->next->next != NULL){
		curr_page_size += (block_ptr->size + sizeof(block));
		block_ptr = block_ptr->next;
	}
	printf("used page size: %d\n", curr_page_size);
	if((curr_page_size + size + sizeof(block)) >= PAGE_SIZE){
		printf("the thread has exceeded a page size, and needs a new page to be allocated contiguously\nEntering give_new_page() function\n\n");
			// need to give page 
		int new_page_ret_val = give_new_page();
		if(new_page_ret_val == -1){
				// CASE 4 - no new page available
			printf("no page was available to give, returning NULL\n\n");
			return NULL;
		} else{
			printf("A new page was given to the user, returning ptr to data segment\n\n");
			block_ptr->next->next = (block*)table_ptr->next;
			return (void*)block_ptr->next->next + sizeof(block);
		}

	} else {
			// need to give new allocation 
		printf("there is still space left in the thread's current page, allocating data segment and returning ptr to the segment\n\n");
		return ((void*)block_ptr->next) + sizeof(block);
	}
}

/* Function to give a thread a ptr to space
  CASE 1: The thread already owns a page and it's in the front position of memory
   - Check to see if next* + size of allocation request overflows into next page
   - If next* overflows, give the thread another contiguous page
   - If it doesn't overflow, setup current + next metadata blocks
   - return ptr + block

  CASE 2: The thread owns a page but it's page is out of position
   - Call memalign to get all of the thread's pages in the front position
   - Traverse metadata blocks until you find an open block and give block to user

  CASE 3: Memallign returns -1 and the thread doesn't own any pages
   - Find a free page (call give_new_page function)

  CASE 4: give_new_page function returns -1 and can't find a page
  	- Return NULL - no pages are available 

  CASE 5: thread tries to request space but already has reached the maximum page limit

     -> see give_allocation() for behavior and return values
 */

void* findSpace(size_t size) {
	printf("In findSpace()\n");
	membook* table_ptr = page_table;

	// CASE 1 - thread's page is in front already
	if(table_ptr->TID == Master->tid){
		printf("thread's page is alredy in place, giving space\nGoing into give_allocation() function\n\n");
		void* give_allocation_ret_val = give_allocation(size);
		printf("Returning from findSpace() function\n\n");
		return give_allocation_ret_val;
	}else {
		printf("thread's page is not in place - must call memAllignPages() to put them in place\ngoing into memAllignPages() function\n\n");
		int memAllign_ret_val = memAllignPages();

	// CASE 3 - thread doesn't own any pages
		if(memAllign_ret_val == -1){
			printf("memAllignPages determined that the thread does not own any pages, giving new page\nGoing into give_new_page() function\n\n");
			int new_page_ret_val = give_new_page();
			if(new_page_ret_val == -1){
			// CASE 4 - no new page available
				printf("no page was available to give, returning NULL\n\n");
				return NULL;
			} else {
				printf("a page was found for the user, returning ptr to data segment within the threads page\n\n");
				return (void*)table_ptr->page + sizeof(block);
			}
		} else {
			// CASE 2 - page is now in the front position
			printf("memAllignPages() placed the thead's pages in position, allocating a data segment\nGoing into give_allocation() function\n\n");
			void* give_allocation_ret_val = give_allocation(size);
			printf("Returning from findSpace() function\n\n");
			return give_allocation_ret_val;
		}
	}
	/*
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

	}Sam wuz here
	
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
	*/
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
	/*
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
	*/
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
void initialize_memory(){

	// Initialize page table

	int i;
	for(i = NUM_OF_PAGES-1; i >= 0; i--){
		//printf("page_table_entry_size * i: %ld\n", PAGE_TABLE_ENTRY_SIZE*i);
		add_page((membook*)&mem + i, i, (void*)&mem + PAGE_TABLE_SIZE + TEMP_PAGE_SIZE + TCB_TABLE_SIZE + SHARED_MEMORY_SIZE + (i*PAGE_SIZE));
	}
	printf("\n");

	// creating swap file
	fileCreation();
}

void* shalloc(size_t size){
	return NULL;
}

/*
 * Malloc Implementation
 * - if the allocation size is greater than a page size - (2*metadata blocks) then myallocate will return NULL. This is because each page must at least require two metadata blocks, one for 
   - See findspace() for return behavior and return values
 */
void* myallocate(int size, char FILE[], int LINE) {

	printf("in myallocate()\n");
	//Error if our malloc'd size is either nothing or negative.
	if (size <= 0) {
		return NULL;
	} else if (size > (PAGE_SIZE - (2*sizeof(block)))){
		return NULL;
	}


	if(first_time == 0)
	{

		sa_mem.sa_flags = SA_SIGINFO;
		sigemptyset(&sa_mem.sa_mask);
		sa_mem.sa_sigaction = handler;

		first_time = 1;

		printf("memalligning all of memory\n");
		memory = memalign(PAGE_SIZE,2048*PAGE_SIZE);
		//int x = posix_memalign((void*)&mem,PAGE_SIZE,TOTAL_MEM);
		if(memory < 0) fatalError(__LINE__, __FILE__);
		

		// initialize page table
		printf("initializing page table\n");
		initialize_memory();

		// creating swap file
		printf("creating swap file\n");
		fileCreation();

		//print_init_memory();

	}


	printf("calling our core function findSpace()\n\n");
	void* metadata = findSpace(size);
	//printf("find space ret val: %p\n", metadata);
	// protecting memory
	printf("mydeallocate() calling protectMem() function\n\n");
	int i = protectMem();
	printf("protectMem ret val: %d\n", i);

	printf("mydeallocate() returning ptr (or NULL) to user thread\n\n");
	return metadata;
	//return (void*)metadata + sizeof(block);
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
  	//memAllignPages();
  }
}


//	-	-	-	-	- THREAD FUNCTION	-	-	-	-	//


static void
func1(void)
{
	printf("func1: started\n");
	printf("malloc'ing now\n\n");
	/* Initial memory allocation */
	char* str = (char *) malloc(sizeof(char)*15);
	//strcpy(str, "tutorialspoint");
	printf("String = %s,  malloc ret value = %p\n", str, str);
	printf("func1: swapcontext(&uctx_func1, &uctx_func2)\n\n");
	if (swapcontext(&uctx_func1, &uctx_func2) == -1)
		handle_error("swapcontext");
	printf("func1: returning\n");
}

static void
func2(void)
{
	printf("func2: started\n");
	printf("calling malloc again\n\n");
	char* str1 = (char *) malloc(sizeof(char)*2);
	//strcpy(str1, "th");
	printf("String = %s,  malloc ret val = %p\n", str1, str1);
	printf("func2: swapcontext(&uctx_func2, &uctx_func1)\n\n");
	if (swapcontext(&uctx_func2, &uctx_func1) == -1)
		handle_error("swapcontext");
	printf("func2: returning\n");
}


int main(int argc, char* arv[]) {

	//printf("size of block %ld\n", sizeof(block));
	//printf("\nsize of size_t: %ld\nsize of int: %ld\nsize of short int: %ld\n", sizeof(size_t), sizeof(int), sizeof(short int));
	Master = (master*)MASTER_TEMP;
	Master->tid = 1;

	printf("main\n");
	printf("Printing first 40 bytes of raw memory just to see 0 contiguous memory\n");
	printRawMem(0, 40);
	printf("\n");
	
	printf("\n---------\n\n");

	char func1_stack[16384];
	char func2_stack[16384];

	if (getcontext(&uctx_func1) == -1)
		handle_error("getcontext");
	uctx_func1.uc_stack.ss_sp = func1_stack;
	uctx_func1.uc_stack.ss_size = sizeof(func1_stack);
	uctx_func1.uc_link = &uctx_main;
	makecontext(&uctx_func1, func1, 0);

	if (getcontext(&uctx_func2) == -1)
		handle_error("getcontext");
	uctx_func2.uc_stack.ss_sp = func2_stack;
	uctx_func2.uc_stack.ss_size = sizeof(func2_stack);
    /* Successor context is f1(), unless argc > 1 */
	uctx_func2.uc_link = (argc > 1) ? NULL : &uctx_func1;
	makecontext(&uctx_func2, func2, 0);

	printf("main: swapcontext(&uctx_main, &uctx_func2)\n\n");


	if (swapcontext(&uctx_main, &uctx_func2) == -1)
		handle_error("swapcontext");


	printf("main: exiting\n");
	exit(EXIT_SUCCESS);


	return 0;
}
