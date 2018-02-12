#define MY_PTHREAD_T_H
#ifndef MY_PTHREAD_T_H

// STRUCTS
// Thread Control Block - holds metadata on structs 
typedef struct _tcb {
	int tid;
	int t_priority;
	ucontext_t* thread_context;
	struct my_pthread_t* next;
} my_pthread_t;

typedef struct _tcb_attr {

} pthread_attr_t;

// FUNCTIONS

// creates a pthread that executes function
// attr is ignored
// A my_pthread is passed into the function, the function then 
int my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, void *(*function)(void*), void* arg){
	// initializing 
	tcb* temp;
	temp = malloc(sizeof(my_pthread));
	if(!tmp){
		printf("Malloc didn't work :(\n");
		return -1;
	}

	// allocating memory for thread_context (is a ptr)
	memset(&temp, 0, sizeof(my_pthread));
	temp->thread_context = malloc(sizeof(ucontext_t));
	if(!tmp->thread_context){
		printf("Malloc didn't work :(\n");
		free(temp);
		return -1;
	}

	getcontext(temp->thread_context);
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
