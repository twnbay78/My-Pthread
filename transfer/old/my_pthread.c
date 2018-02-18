#include "my_pthread_t.h"

unsigned int tid = 0;
MTH* Master;

typedef struct _JUNK {
	int x;
	char y;
}t_struct;

void* t1(void* arg){
	printf("Thread has been created\n");
	int x = 10;
	printf("x = %d\n", x);
	return NULL;
}

int main(int argc, char* argv[]){

	
 	Master = (MTH*)malloc(sizeof(MTH));
 	initializeMTH(Master);
	printf("init'd mth\n");
	
	//t_struct* t_elem = (t_struct*)malloc(sizeof(t_struct));
 	my_pthread_t* test1 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
 	enqueue(Master->Medium,createThread(test1, NULL) );
	//my_pthread_create(test1, NULL, (void*)t1, NULL);
 	//printMTH(Master);

	/*
  	my_pthread_t* test2 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
 	my_pthread_t* test3 = (my_pthread_t*)malloc(sizeof(my_pthread_t));

	my_pthread_t tid;
	printf("test threads and structs initialized\n");
	my_pthread_create(&tid, NULL, (void*)t1, NULL);
 	enqueue(Master->Medium,createThread(test2, NULL) );
 	enqueue(Master->High,createThread(test3, NULL) );
 	printMTH(Master);
 	move2Q(Master->Medium,Master->Low,seek(test1->parent,test1));
 	printf("\n");
 	printMTH(Master);
 	move2Q(Master->Medium,Master->Medium,seek(test2->parent,test2));
 	printf("\n");
 	printMTH(Master);
	*/

	// free
	free(Master);
	return 0;
}
