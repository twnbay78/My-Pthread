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
#include <limits.h>
#include <math.h>

// preprocessor 
#define HIGH_EXEC_TIMEOUT 25000
#define MEDIUM_EXEC_TIMEOUT 37000
#define LOW_EXEC_TIMEOUT 50000
#define T_STACK_SIZE 1048576


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

// Enum to represent queue types
typedef enum Queue_Type {
	MasterThreadHandler = 0, 
	Wait = 1,
	Cleaner = 2,
	level1 = 3,
	level2 = 4,
	level3 = 5
}queue_type;

// priority value 
enum special {
	removal = 26
};

//Holds variables to keep track of mutex locks
typedef struct _mutex {
	
	//responsible for locking the mutex: either 1 or 0.
	int lock;
	int guard;

	my_pthread_t thread;	// link this to the thread that owns this mutex
	my_pthread_t* waitQ;	// wait queue of other threads waiting to access
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
	queue_type queue;
	long int start_init;
	long int start_exec;
	long int start_ready;
	long int start_wait;
	long int total_exec;
	long int total_ready;
	long int total_wait;
	struct _tcb* parent;
	struct _tcb* next;
} my_pthread_t;


/* Special MTH struct that holds all of the queues
 * high -> 25ms quanta
 * medium -> 37ms quanta
 * low -> 50ms quanta
 * cleaner -> terminated threads that are waiting to be destroyed
 */
typedef struct _MTH{
   my_pthread_t* Low;
   my_pthread_t* Medium;
   my_pthread_t* High;
   my_pthread_t* Wait;
   my_pthread_t* Cleaner;
}MTH;

// - - - - - - - GLOBAL VARIABLES - - - - - - - - // 
extern unsigned int tid;
extern MTH* Master;

// - - - - - - - QUEUE FUNCTIONS - - - - - - - //

/* initializes a queue level 
 *
 */
void initQ(my_pthread_t * lead,char* header,int t){
  lead->tid = t;
  lead->t_priority = 0;
  lead->t_context = NULL;
  lead->next = NULL;
  lead->name = header;
  lead->parent = NULL;
 // lead->isTail=false;
}

/* Initializes MTH struct including all required queues
 *
 */
void initializeMTH(MTH* main)
{
	my_pthread_t* L1 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
 	my_pthread_t* L2 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
 	my_pthread_t* L3 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
 	my_pthread_t* WaitT = (my_pthread_t*)malloc(sizeof(my_pthread_t));
 	my_pthread_t* CleanerT = (my_pthread_t*)malloc(sizeof(my_pthread_t));

 	 initQ(L3,"Highest Q",level3);
 	printf("%s (%d)\n",L3->name,L3->tid);
 	 initQ(L2,"Medium Q",level2);
 	printf("%s (%d)\n",L2->name,L2->tid);
 	 initQ(L1,"Lowest Q",level1);
 	printf("%s (%d)\n",L1->name,L1->tid);
 	initQ(WaitT,"Wait Q",Wait);
 	printf("%s (%d)\n",WaitT->name,WaitT->tid);	
 	initQ(CleanerT,"Cleaner Q",Cleaner);
 	printf("%s (%d)\n",CleanerT->name,CleanerT->tid);	
	printf("\n");
 	main->High = L3;
 	main->Medium = L2;
 	main->Low = L1;
 	main->Cleaner = CleanerT;
 	main->Wait= WaitT;

	free(L1);
	free(L2);
	free(L3);
	free(WaitT);
	free(CleanerT);
}

// prints a queue given a ptr to the head
void printQueue(my_pthread_t* head) {
  printf("%d, ", head->tid);
  while (head->next != NULL) {
    head = head->next;
    printf("%d, ", head->tid);
  }
  printf("\n");
}

/* Prints ALL of the contents of the MTH
 */
void printMTH(MTH* main)
{
  printQueue(main->High);
  printQueue(main->Medium);
  printQueue(main->Low);
  printQueue(main->Wait);
  printQueue(main->Cleaner);
}


// traverses through the queue and returns the tail - O(n) 
my_pthread_t* runT(my_pthread_t* head)
{
 my_pthread_t* start = head;
  while(start->next!=NULL)
    start = start->next;
return start;
}



/* Adds a thread to the head of a queue
 * Input -> queue
 * 	 -> new thread
 * Output -> queue whose head is the new Thread
 */
void enqueue(my_pthread_t* head, my_pthread_t* newThread){

  //pQueue head is the front of the ArrayList with no context that keeps track of the "queue".

  my_pthread_t* ending = runT(head);    //Find end of ArrayList
  ending->next = newThread;       //Insertion of newThread
  newThread->next = NULL;
  newThread->parent = head;
  //newThread->isTail=true;

}


/* Re-prioritize/maintenance utility function - traverses the queue and removes threads with low priority values 
 * 
 */
void dequeuePriority(my_pthread_t* head) {
  
  my_pthread_t* prev = NULL;
  
  while (head != NULL) {
    
    if (head->t_priority == 26) {
      prev->next = head->next;
      head->next = NULL;
      free(head);
      
      head = prev->next;
    }
    
    else {
      prev = head;
      head = head->next;
    }
    
  }
  
}


/* Removes specific threads within a queue
 * 	CASE 1 -> Remove a thread to assign to a different queue due to priority change
 * 	CASE 2 -> Remove the thread on the tail and place it in the cleanup queue
 * 	CASE 3 -> Remove the running thread from the end of the high priority queue and place it at the front of the queue due to the thread being yielded (priority does not change)
 *
 */
void dequeueSpecific(my_pthread_t* head,my_pthread_t* thread) {
  
  my_pthread_t* prev = NULL;
  int temp = thread->tid;
  while (head != NULL) {
    
    if (head->tid == temp) {
      prev->next = head->next;
      head->next = NULL;
      //free(head);
      
      head = prev->next;
    }
    
    else {
      prev = head;
      head = head->next;
    }
    
  }
  
}

/* Searches for a thread within the queue and returns the thread
 *
 */
my_pthread_t* seek(my_pthread_t* head, my_pthread_t* findMe)
{
  int tempId = findMe->tid;
   while(head!=NULL)
  {
    if(head->tid==tempId)
      return head;
     head = head->next;
   }
   return NULL;

}

/* INPUT -> destination queue
 * 	 -> source queue
 * 	 -> thread
 * OUTPUT -> destination queue + thread
 * 	  -> source queue - thread
 * NOTES -> Leave moveFrom arg NULL if you do not want to dequeue
 *
 */
void move2Q(my_pthread_t* moveTo, my_pthread_t* moveFrom,my_pthread_t* thread)
{
  if(thread==NULL)
    return;
  if(moveFrom!=NULL)
  {
 dequeueSpecific(moveFrom,thread);
}
   enqueue(moveTo,thread);
    
}

// - - - - - - - FUNCTIONS - - - - - - - //

// signal handler for interrupted running threads
void schedule_handler (int signum){
	if(signum == SIGALRM){
		printf("timer went off!\n");
	}else{
		fprintf(stderr, "Caught wrong signal for some reason. Error message: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

/* Schedule handler to assign different priority levels to threads
 *
 */
void rePrioritize(my_pthread_t* head){

  my_pthread_t* start;
  my_pthread_t*finalSwapPos;
  my_pthread_t* maxPosPrev;
  my_pthread_t* maxPos;
  if(head->next!=NULL)
  {
  start = head;
  maxPosPrev = head;
  maxPos = head->next;
  finalSwapPos = head->next;
  }
  //Find max
  while (head->next != NULL) {
    if((head->next)->t_priority > maxPos->t_priority)
      {
        maxPos = head->next;
        maxPosPrev = head; 
      }
    head = head->next;
  }
  //swap
  my_pthread_t* temp;

  temp = maxPos->next;
  maxPos->next = finalSwapPos->next;
  finalSwapPos->next = temp;

  start->next = maxPos;
  maxPosPrev->next = finalSwapPos;


}


// Function that executes a thread function
int exec_thread(my_pthread_t* thread, void *(*function)(void*), void* arg){
	// execute thread function until termination
	thread-> t_retval = function(arg);
	printf("executed function\n");

	// timer set to 25ms quanta, once timer is met scheduler_handler will catch the thread and pass execution time to a new thread
	struct itimerval exec_timer;
	if(signal(SIGALRM, &schedule_handler) == SIG_ERR){
		fprintf(stderr, "Could not catch signal. Error message: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if(thread->queue == level2){
		exec_timer.it_value.tv_sec = 0;
		exec_timer.it_value.tv_usec = MEDIUM_EXEC_TIMEOUT;
		exec_timer.it_interval.tv_sec = 0;
		exec_timer.it_interval.tv_usec = 0;

	}
	else if(thread->queue == level3){
		exec_timer.it_value.tv_sec = 0;
		exec_timer.it_value.tv_usec = LOW_EXEC_TIMEOUT;
		exec_timer.it_interval.tv_sec = 0;
		exec_timer.it_interval.tv_usec = 0;
		
	}else {
		exec_timer.it_value.tv_sec = 0;
		exec_timer.it_value.tv_usec = HIGH_EXEC_TIMEOUT;
		exec_timer.it_interval.tv_sec = 0;
		exec_timer.it_interval.tv_usec = 0;
	}
	if(setitimer(ITIMER_REAL, &exec_timer, NULL) == -1){
		fprintf(stderr, "Could not set timer. Error message: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	printf("setup timer + signal\n");

	// thread terminates
	// move2Q THIS THREAD TO CLEANUP
	//move2Q(Master->Cleaner, CURRENT_QUEUE, thread);
	// RUN SCHEDULE HANDLER HERE
	// schedule_handler();
	
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

// Creates a thread for testing purposes
my_pthread_t* createThread(my_pthread_t* Thread,ucontext_t* Context)
{
  Thread->tid = rand(); // find a way to assign values
  Thread->t_priority = INT_MAX; // make priority highest
  Thread->name = NULL;
  Thread->t_context = Context;
  Thread->next = NULL;
  Thread->parent = NULL;
  //Thread->isTail=false;
  return Thread;
}

// - - - - - - - THEAD FUNCTIONS - - - - - - - //

// creates a pthread that executes function
// attr is ignored
// A my_pthread is passed into the function, the function then 
int my_pthread_create(my_pthread_t* thread, pthread_attr_t* attr, void *(*function)(void*), void* arg){

	printf("starting my_pthread_create\n");

	/*
	// create space for thread
	thread = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	*/

	// initialize context
	if(getcontext(thread->t_context) == -1){
		fprintf(stderr, "Could not get context. Error message: %s\n", strerror(errno));
		return -1;
	}
	printf("context is active\n");

	thread->state = NEW;
	printf("Thread state is now NEW\n");

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
	thread->name = (char*)malloc(sizeof("mth_high"));
	thread->name = "mth_high";
	printf("Thread initialized\n");

	// make execution context
	makecontext(thread->t_context, (void*)exec_thread, 3, thread, function, arg);
	printf("context made and thread executed\n");
	
	// enqueue process in MLPQ with highest priority
	enqueue(Master->High, thread);
	printf("thread enqueued\n");
	thread->state = READY;
	printf("thread state now READY\n");

	return 0;
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
	// Set state of running thread to YIELD and have a signal handler run
	// thread.state = EXITED
}

// Call to the my_pthread_t library ensuring that the calling thread will not continue execution until the one references exits
// If value_ptr is not null, the return value of the exiting thread will be passed back
int my_pthread_join(my_pthread_t thread, void** value_ptr){
	return -1;
}

// initializes a my_pthread_mutex_t created by the calling thread
// attributes are ignored
/*  
 * Return value for mutex: returns 0 if init is successful. If not, returns error int
 * pthread.h: EINVAL = value specified if mutex is invalid
 * 			  EBUSY = value returned if the mutex
*/
int my_pthread_mutex_init(my_pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr){

	if (mutex == NULL) {
		return EINVAL;
	}
}

// Locks a given mutex
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
