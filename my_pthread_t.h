#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <ucontext.h>
#include <errno.h>
#include <limits.h>
#include <sys/time.h>
#include <string.h>

// preprocessor 
#define EXEC_TIMEOUT 25000
#define T_STACK_SIZE 1048576

// Method signatures
double get_time();

// variables 
extern unsigned int tid;

// - - - - - - - STRUCTS - - - - - - - - //

/*
 * THREAD STATES
 * NEW -> Thread created but not initialized
 * READY -> Thread initalized and ready to be executed
 * RUNNING -> Thread begain execution and did not terminate yet
 * WAITING -> Thread is waiting for I/O 
 * YIELDED -> Thread yielded to another thread
 * BLOCKED -> Thread sitting in a wait queue waiting for resources
 * TERMINATED -> Thread terminated, is in a termination queue, and is waiting for scheudle maintanence to run to have it cleared
 */
typedef enum t_state{
	NEW,
	READY,
	RUNNING,
	WAITING,
	YIELDED,
	BLOCKED,
	TERMINATED,
}t_state;

typedef struct _mutex {
	
}my_pthread_mutex_t;

/*  Thread Control Block - holds metadata on structs 
 *
 * tid: thread id
 * t_priority ->  priority level of the thread - determines queue level
 * name -> indicates which queue the thread is currently in (debugging/output purposes)
 * 	- mth = master thread handler aka highest level ready queue
 * t_retval -> the return value of the thread function
 * t_context -> the context in which the thread holds
 * 	- registers
 * 	- stack 
 * 	- sigmasks
 * start_init -> timestamp of when the thread is initialized
 * start_exec -> timestamp of when the thread first begins execution
 * start_read -> timestamp of when the thread first enters ready queue 
 * start_wait -> timestamp of when the thread first enters the wait queue
 *
 */

typedef struct _tcb {
	int tid;
	int t_priority;
	char* name; 
	void* t_retval;
	ucontext_t* t_context;
	t_state state;
	long int start_init;
	long int start_exec;
	long int start_ready;
	long int start_wait;
} my_pthread_t;


// - - - - - - - HELPER FUNCTIONS - - - - - - - //

// signal handler for interrupted running threads
void signal_handler (int signum){
	if(signum == SIGALRM){
		printf("timer went off!\n");
	}else{
		fprintf(stderr, "Caught wrong signal for some reason. Error message: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

// Function that executes a thread function
int exec_thread(my_pthread_t* thread, void *(*function)(void*), void* arg){
	// execute thread function until termination
	thread-> t_retval = function(arg);

	// timer set to 25ms quanta, once timer is met scheduler_handler will catch the thread and pass execution time to a new thread
	struct itimerval exec_timer;
	if(signal(SIGALRM, &signal_handler) == SIG_ERR){
		fprintf(stderr, "Could not catch signal. Error message: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	exec_timer.it_value.tv_sec = 0;
	exec_timer.it_value.tv_usec = EXEC_TIMEOUT;
	exec_timer.it_interval.tv_sec = 0;
	exec_timer.it_interval.tv_usec = 0;
	if(setitimer(ITIMER_REAL, &exec_timer, NULL) == -1){
		fprintf(stderr, "Could not set timer. Error message: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	
	return -1;
}


// function that grabs the current time in microseconds
double get_time(){
	struct timeval time;
	gettimeofday(&time, NULL);
	double return_val = (time.tv_sec * 1000 + time.tv_usec/1000);
	printf("time: %lf\n", return_val);
	return return_val;
}


// - - - - - - - THEAD FUNCTIONS - - - - - - - //

// creates a pthread that executes function
// attr is ignored
// A my_pthread is passed into the function, the function then 
int my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, void *(*function)(void*), void* arg){

	// initialize context
	if(getcontext(thread->t_context) == -1){
		fprintf(stderr, "Could not get context. Error message: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	thread->state = NEW;

	// allocating stack for thread
	thread->t_context->uc_stack.ss_sp = (void*)malloc(T_STACK_SIZE);
	if(!thread->t_context->uc_stack.ss_sp){
		printf("Malloc didn't work :(\n");
		free(thread);
		return -1;
	}
	thread->t_context->uc_stack.ss_size = T_STACK_SIZE; 
	printf("stack allocated\n");

	// setting my_pthread_t elements
	thread->tid = ++tid;
	thread->t_priority = INT_MAX;
	thread->name = (char*)malloc(sizeof("mth"));
	thread->name = "mth";
	printf("Thread initialized\n");

	// make execution context
	makecontext(thread->t_context, (void*)exec_thread, 3, thread, function, arg);
	printf("context made and thread executed\n");

	thread->state = READY;
	
	// enqueue process in MLPQ with highest priority
	//Enqueue(mth_thread, read);
	printf("thread enqueued\n");

	return -1;
}

// explicit call to the my_pthread_t scheduler requesting that the current context can be swapped out
// another context can be scheduled if one is waiting
void my_pthread_yield(){
	// Set state of running thread to YIELD and have a signal handler run
	// thread.state = YIELDED
}

// explicit call to the my_pthread_t library to end the pthread that called it
// If the value_ptr isn't NULL, any return value from the thread will be saved
// FIX RETURN VAL
void my_pthread_exit(void* value_ptr){
}

// Call to the my_pthread_t library ensuring that the calling thread will not continue execution until the one references exits
// If value_ptr is not null, the return value of the exiting thread will be passed back
int my_pthread_join(my_pthread_t thread, void** value_ptr){
	return -1;
}

// initializes a my_pthread_mutex_t created by the calling thread
// attributes are ignored
int my_pthread_mutex_init(my_pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr){

	return -1;
}

// Loccks a given mutex
// Other threads attempting to access this mutex will not run until it is unlocked
int my_pthread_mutex_lock(my_pthread_mutex_t* mutex){

	return -1;
}

// Unlocks a given mutex
int my_pthread_mutex_unlock(my_pthread_mutex_t* mutex){

	return -1;
}

// Destroys a given mutex
// Mutex should be unlocked before doing so (or deadlock)
int my_pthread_mutex_destroy(my_pthread_mutex_t* mutex){

	return -1;
}





#endif
