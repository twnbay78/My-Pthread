#include "my_pthread_t.h"

unsigned int tid = 0;
MTH* Master;

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
 	intializeBaseQ(Master);

 	my_pthread_t* test1 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
  	my_pthread_t* test2 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
 	my_pthread_t* test3 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
 	enqueue(Master->Low,createThread(test1, NULL) );
 	enqueue(Master->Medium,createThread(test2, NULL) );
 	enqueue(Master->High,createThread(test3, NULL) );
 	printMTH(Master);
 	move2Q(Master->Medium,Master->Low,seek(test1->parent,test1));
 	printf("\n");
 	printMTH(Master);
 	move2Q(Master->Medium,Master->Medium,seek(test2->parent,test2));
 	printf("\n");
 	printMTH(Master);

	return 0;
}
