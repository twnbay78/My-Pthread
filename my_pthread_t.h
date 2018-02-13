#define MY_PTHREAD_T_H
#ifndef MY_PTHREAD_T_H

// preprocessor 
#define T_STACK_SIZE 1048576

// variables 
extern static unsigned long int tid = 0;

// - - - - - - - STRUCTS - - - - - - - - //
// Thread Control Block - holds metadata on structs 
typedef struct _tcb {
	int tid;
	int t_priority;
	char* name; // indicates queue
	ucontext_t* t_context;
	long int start_init;
	long int start_ready;
	long int start_wait;
	long int start_exec;
} my_pthread_t;

typedef struct _tcb_attr {

} pthread_attr_t;

// - - - - - - - HELPER FUNCTIONS - - - - - - - //

int exec_thread(my_pthread_t* thread, void *(*function)(void*), void* arg){
	// execute thread function until termination
	// set timer for 25 ms quanta
	struct itimerval exec_timer;

}



// - - - - - - - THEAD FUNCTIONS - - - - - - - //

// creates a pthread that executes function
// attr is ignored
// A my_pthread is passed into the function, the function then 
int my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, void *(*function)(void*), void* arg){

	// initialize context
	if(getcontext(thread->t_context) == -1){
		fprintf(stderr, "An error has occured: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// allocating stack for thread
	thread->uc_stack->ss_sp = (void*)malloc(T_STACK_SIZE);
	if(!thread->uc_stack->ss_sp){
		printf("Malloc didn't work :(\n");
		free(thread);
		return -1;
	}
	thread->uc_stack->ss_size = T_STACK_SIZE; 
	printf("stack allocated\n");

	// setting my_pthread_t elements
	thread->tid = ++tid;
	thread->t_priority = INT_MAX;
	thread->name = (char*)malloc(sizeof("mth"));
	thread->name = "mth";
	printf("Thread initialized\n");
	
	// make execution context
	makecontext(thread->t_context, exec_thread, 3, thread, function, arg);
	printf("context made and thread executed\n");
	
	// enqueue process in MLPQ with highest priority
	Enqueue(mth_thread, read);
	printf("thread enqueued\n");

}

// explicit call to the my_pthread_t scheduler requesting that the current context can be swapped out
// another context can be scheduled if one is waiting
void my_pthread_yield(){
	
}

// explicit call to the my_pthread_t library to end the pthread that called it
// If the value_ptr isn't NULL, any return value from the thread will be saved
void pthread_exit(void* value_ptr){

}

// Call to the my_pthread_t library ensuring that the calling thread will not continue execution until the one references exits
// If value_ptr is not null, the return value of the exiting thread will be passed back
int my_pthread_join(my_pthread_t thread, void** value_ptr){

}

// initializes a my_pthread_mutex_t created by the calling thread
// attributes are ignored
int my_pthread_mutex_init(my_pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr){

}

// Loccks a given mutex
// Other threads attempting to access this mutex will not run until it is unlocked
int my_pthread_mutex_lock(my_pthread_mutex_t* mutex){

}

// Unlocks a given mutex
int my_pthread_mutex_unlock(my_pthread_mutex_t* mutex){

}

// Destroys a given mutex
// Mutex should be unlocked before doing so (or deadlock)
int my_pthread_mutex_destroy(my_pthread_mutex_t* mutex){

}





#endif
