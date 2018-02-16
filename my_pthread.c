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
	
	/* TESTING TIMES
	double start = get_time();
	int x = 0;
	while (x < 10000){
		printf("doing stuff\n");
		x++;
	}
	double end = get_time();
	printf("It took %lf ms to do stuff\n", end - start);
	*/

 	Master = (MTH*)malloc(sizeof(MTH));
 	initializeMTH(Master);
	printf("init'd mth\n");
	
	/*
	//t_struct* t_elem = (t_struct*)malloc(sizeof(t_struct));
 	my_pthread_t* test1 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
  	my_pthread_t* test2 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
 	my_pthread_t* test3 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	*/

	my_pthread_t tid;

	printf("test threads and structs initialized\n");
	if(my_pthread_create(&tid, NULL, (void*)t1, NULL) == -1){
		fprintf(stderr, "could not create thread. Error message: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}	
	/*
 	enqueue(Master->Low,createThread(test1, NULL) );
 	enqueue(Master->Medium,createThread(test2, NULL) );
 	enqueue(Master->High,createThread(test3, NULL) );
 	printMTH(Master);
 	move2Q(Master->Medium,Master->Low,seek(test1->parent,test1));
 	printf("\n");
 	printMTH(Master);
 	move2Q(Master->Medium,Master->Medium,seek(test2->parent,test2));
	*/
 	printf("\n");
 	printMTH(Master);

	// free
	free(Master);
	return 0;
}
