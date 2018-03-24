// - - - - - - - - STRUCTS - - - - - - - - - //

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

struct _membook {
	int TID;
	int page_num;
	int free;
}
// - - - - - - - - METHOD DECLARATIONS - - - - - - - - - //

void* myallocate(int size,char FILE[],int LINE);
struct _block* findSpace(struct _block** last, size_t size);
struct _block* reqSpace(struct _block* last, size_t size);
struct _block *get_block_ptr(void *ptr);

