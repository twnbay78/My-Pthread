#include "my_pthread_t.h"

unsigned int tid = 0;

int main(int argc, char* argv[]){
	
	double start = get_time();
	int x = 0;
	while (x < 10000){
		printf("doing stuff\n");
		x++;
	}
	double end = get_time();
	printf("It took %lf ms to do stuff\n", end - start);
	return 0;
}
