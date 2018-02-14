#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

void signal_handler (int signum){
	printf("timer went off!\n");
}

double get_time(){
	struct timeval time;
	gettimeofday(&time, NULL);
	double return_val = (time.tv_sec * 1000 + time.tv_usec/1000);
	printf("time: %lf\n", return_val);
	return return_val;
}

// saves main context with getcontext, returns to the main context in func() with setcontext
int main(int argc, char* argv[]){
	struct itimerval exec_timer;

	if(signal(SIGALRM, (void(*)(int)) signal_handler) == SIG_ERR){
		fprintf(stderr, "An error has occured: %s\n: ", strerror(errno));
		exit(EXIT_FAILURE);
	}

	exec_timer.it_value.tv_sec = 0;
	exec_timer.it_value.tv_usec = 25000;
	exec_timer.it_interval.tv_sec = 0;
	exec_timer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &exec_timer, NULL);
	double start = get_time();

	int x = 0;
	while(x < 10){
		printf("Doing stuff\n");
		x++;
	}
	
	double end = get_time();

	printf("Took %lf microseconds for the stuff to do\n", end - start);

	return 0;
}
