#include <stdlib.h>
#include "my_pthread_t.h"

void intializeBaseQ(MTH* main);
int main(int argc, char* argv[]){
 	MTH* Master = malloc(sizeof(MTH));
 	intializeBaseQ(Master);

 	my_pthread_t* test1 = malloc(sizeof(my_pthread_t));
  	my_pthread_t* test2 = malloc(sizeof(my_pthread_t));
 	my_pthread_t* test3 = malloc(sizeof(my_pthread_t));
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
}

void intializeBaseQ(MTH* main)
{
	my_pthread_t* L1 = malloc(sizeof(my_pthread_t));
 	my_pthread_t* L2 = malloc(sizeof(my_pthread_t));
 	my_pthread_t* L3 = malloc(sizeof(my_pthread_t));
 	my_pthread_t* WaitT = malloc(sizeof(my_pthread_t));
 	my_pthread_t* CleanerT = malloc(sizeof(my_pthread_t));

 	initQ(WaitT,"Wait Q",Wait);
 	printf("%s (%d)\n",WaitT->name,WaitT->tid);	
 	initQ(CleanerT,"Cleaner Q",Cleaner);
 	printf("%s (%d)\n",CleanerT->name,CleanerT->tid);	
 	 initQ(L1,"Lowest Q",level1);
 	printf("%s (%d)\n",L1->name,L1->tid);
 	 initQ(L2,"Medium Q",level2);
 	printf("%s (%d)\n",L2->name,L2->tid);
 	 initQ(L3,"Highest Q",level3);
 	printf("%s (%d)\n",L3->name,L3->tid);



 	main->High = L3;
 	main->Medium = L2;
 	main->Low = L1;
 	main->Cleaner = CleanerT;
 	main->Wait= WaitT;
}
/*
 	my_pthread_t* test1 = malloc(sizeof(my_pthread_t));
  	my_pthread_t* test2 = malloc(sizeof(my_pthread_t));
 	my_pthread_t* test3 = malloc(sizeof(my_pthread_t));

 	enqueue( lead, createThread(test1, NULL) );
 	enqueue( lead, createThread(test2, NULL) );
  	enqueue( lead, createThread(test3, NULL) ); 

  	test3->t_priority = 26;
  	//test1->t_priority = 1;
  	//test2->t_priority = 1;

  	printQueue(lead);
  	printf("\n");
		move2Q(lead,lead,seek(lead,test1));
		move2Q(leadWait,lead,seek(lead,test2));
		move2Q(leadWait,lead,seek(lead,test1));
		dequeue(lead);
 	printQueue(lead);
 	printf("\n");

 	printQueue(leadWait);
 	 	printf("\n");

	return 0;
*/