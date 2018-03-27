#include "my_pthread_t.h"

static my_pthread_t* Master;
unsigned int tid = 0;
static ucontext_t ctx_main;

typedef struct _JUNK {
	int x;
	char y;
}t_struct;


// - - - - - - - QUEUE FUNCTIONS - - - - - - - //
int start =0;
/* initializes a queue level 
 *
 */
void initQ(my_pthread_t * lead,char* header,int t){
	lead->tid = t;
	lead->t_priority = 0;
	//lead->t_context = NULL;
	lead->next = NULL;
	lead->name = header;
	lead->parent = NULL;
	// lead->isTail=false;
}

/* Initializes MTH struct including all required queues
 *
 */
void initializeMTH(my_pthread_t* main)
{
	my_pthread_t* L1 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	my_pthread_t* L2 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	my_pthread_t* L3 = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	my_pthread_t* WaitT = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	my_pthread_t* CleanerT = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	my_pthread_mutex_t* MutexT = (my_pthread_mutex_t*)malloc(sizeof(my_pthread_mutex_t));

	main->high_size=0;
	main->medium_size=0;
	main->low_size=0;
	main->wait_size=0;
	main->mutex_size=0 ;

	initQ(L3,"Highest Q",5);
	printf("%s (%d)\n",L3->name,L3->tid);
	initQ(L2,"Medium Q",level2);
	printf("%s (%d)\n",L2->name,L2->tid);
	initQ(L1,"Lowest Q",level1);
	printf("%s (%d)\n",L1->name,L1->tid);
	initQ(WaitT,"Wait Q",Wait);
	printf("%s (%d)\n",WaitT->name,WaitT->tid); 
	initQ(CleanerT,"Cleaner Q",Cleaner);
	printf("%s (%d)\n",CleanerT->name,CleanerT->tid); 

	main->High = L3;
	main->Medium = L2;
	main->Low = L1;
	main->Cleaner = CleanerT;
	main->Wait= WaitT;
	main->Mutex = MutexT;
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


void printThread(my_pthread_t* node)
{
	printf("Tid: %d Priority: %d Name: %s\n",node->tid,node->t_priority,node->name);
}


my_pthread_t* peak(my_pthread_t* head){
	if(head->next != NULL){
		return (head->next);
	}
	return NULL;
}

/* Prints ALL of the contents of the MTH
*/
void printMTH(my_pthread_t* main)
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
 *   -> new thread
 * Output -> queue whose head is the new Thread
 */
void enqueue(my_pthread_t* head, my_pthread_t* newThread){

	//pQueue head is the front of the ArrayList with no context that keeps track of the "queue".

	my_pthread_t* ending = runT(head);    //Find end of ArrayList
	ending->next = newThread;       //Insertion of newThread
	newThread->next = NULL;
	newThread->parent = head;
	printf("thread queue: %d\n", newThread->queue);
	if(newThread->queue == 3){
		printf("high size: %d\n", Master->high_size);
		Master->high_size++;
		printf("high size: %d\n", Master->high_size);
	}
	else if(newThread->queue == 4){
		printf("medium size: %d\n", Master->medium_size);
		Master->medium_size++;
		printf("medium size: %d\n", Master->medium_size);
	}
	else if(newThread->queue == 5){
		printf("low size: %d\n", Master->low_size);
		Master->low_size++;
		printf("low size: %d\n", Master->low_size);
	}

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
 *  CASE 1 -> Remove a thread to assign to a different queue due to priority change
 *  CASE 2 -> Remove the thread on the tail and place it in the cleanup queue
 *  CASE 3 -> Remove the running thread from the end of the high priority queue and place it at the front of the queue due to the thread being yielded (priority does not change)
 *
 */
void dequeueSpecific(my_pthread_t* head,my_pthread_t* thread) {

	my_pthread_t* prev = NULL;
	int temp = thread->tid;
	if(strcmp(thread->name, "mth_high") == 0)
		Master->high_size--;
	if(strcmp(thread->name, "mth_medium") == 0)
		Master->medium_size--;
	if(strcmp(thread->name, "mth_low") == 0)
		Master->low_size--;

	while (head != NULL) {

		if (head->tid == temp) {
			prev->next = head->next;
			head->next = NULL;
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
	my_pthread_t* ptr = head;
	int target = findMe->tid;
	while(ptr != NULL)
	{
		if(ptr->tid == target)
			return ptr;
		ptr = ptr->next;
	}
	return NULL;

}

/* INPUT -> destination queue
 *   -> source queue
 *   -> thread
 * OUTPUT -> destination queue + thread
 *    -> source queue - thread
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

/* Frees all elements in the queue and returns an empty queue
 *  -> Once a thread terminates, it is moved to the cleanup queue via mt_handler
 *  -> When the maint. cycle kicks in, the scheduler will be interrupted 
 *  -> This is when we free our cleanup queue and get rid of the terminated threads
 *  since terminating them during scheduling will take up CPU time
 */
my_pthread_t* emptyQueue(my_pthread_t* head){
	my_pthread_t* ptr = head;
	while(head != NULL){
		ptr = head;
		head = head->next;
		free(ptr);
	}
	head = NULL;
	return head;
}

// - - - - - - - FUNCTIONS - - - - - - - //

/* Takes in the MTH and decides what thread to run next using a FCFS implementation
 *  -> Executes the thread in the highest priority level
 *  -> If a priority level is empty, it checks the next queue
 *
 */
my_pthread_t* dispatcher(my_pthread_t* master){
	printf("\n\ncalling dispatcher\n");
	my_pthread_t* tmp;
	// if there is a thread in the high priority queue, run the next one in line
	if(Master->high_size > 0){
		printf("size of high priority: %d\n", Master->high_size);
		return peak(Master->High);
	}
	else if(Master->medium_size > 0){
		printf("size of medium priority: %d\n", Master->medium_size);
		return peak(Master->Medium);
	}
	else if(Master->low_size > 0 ){
		printf("size of medium priority: %d\n", Master->low_size);
		return peak(Master->Low);
	}
	else{
		printf("dispatcher returning NULL\n\n");
		printMTH(Master);
		return NULL;
	}
	printf("dispatcher returning thread\n\n");
	return tmp;
}

/* MASTER THREAD HANDLER -> MASTER OF ALL, BOW DOWN TO HER YOU IMPUDENT USER THREAD
 *  -> Contains the main execution context 
 *  -> Responsible for all aging 
 *  -> IF:
 *    - A thread uses its timeslice -> is demoted to the next queue
 *    - A thread yields -> is moved to the begining of the current queue
 *    - A thread terminates -> is moved to the cleanup queue
 *    - A queue is empty -> runs the next level non-empty queue
 *    - Maintanence cycle hits -> reprioritization (aging) occurs
 */
void mt_handler (){

	printf("\n\nsig handler starting\n");

	printf("\n");
	printMTH(Master);
	printf("\n");

	// if there is a thread in the ready queue, HANDLE IT
	if(Master->current != NULL){
		printf("currnent thread set\n");
		// Put thread in cleaner queue
		if(Master->current->state == TERMINATED){
			if(Master->current->queue == 3){
				Master->current->queue = 26;
				printf("high size: %d\n", Master->high_size);
				move2Q(Master->Cleaner, Master->High, peak(Master->High));    
				printf("high size: %d\n", Master->high_size);
			}
			else if(Master->current->queue == 4){
				Master->current->queue = 26;
				printf("medium size: %d\n", Master->medium_size);
				move2Q(Master->Cleaner, Master->Medium, peak(Master->Medium));    
				printf("medium size: %d\n", Master->medium_size);
			}
			else if(Master->current->queue == 5){
				Master->current->queue = 26;
				printf("low size: %d\n", Master->low_size);
				move2Q(Master->Cleaner, Master->Low, peak(Master->Low));    
				printf("low size: %d\n", Master->low_size);
			}
			printf("current thread terminated\n");
			Master->current = NULL;

		}
		else if(Master->current->state == YIELDED){
			move2Q(Master->current,Master->current,peak(Master->High));
			Master->current = NULL;
		}
		else if (Master->current->state == READY){
			printf("Thread attempting to execute\n");
			printf("tid of next thread: %d\n", Master->current->tid);
			if(swapcontext(&Master->t_context, &Master->current->t_context) == -1){
				fprintf(stderr, "Could not swap context to scheduled thread. Error msg: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
			if(Master->current->start_exec>=HIGH_EXEC_TIMEOUT)
			{
				free(Master->current->name);
				Master->current->name = (char*)malloc(sizeof("mth_medium"));
				Master->current->name = "mth_medium";
				move2Q(Master->High,Master->current->parent,Master->current);
				printf("time exec = %li",Master->current->total_exec);
				Master->current->total_exec = 0; 

			} 
			else if(Master->current->start_exec>=MEDIUM_EXEC_TIMEOUT)
			{
				free(Master->current->name);
				Master->current->name = (char*)malloc(sizeof("mth_low"));
				Master->current->name = "mth_low";
				move2Q(Master->Low,Master->current->parent,Master->current);
				Master->current->total_exec = 0; 
			}
		}

	}

	printf("why is this happening\n");
	// thread needs to be scheduled
	if(Master->current == NULL){
		printf("no current thread set, setting current thread\n");
		Master->current = dispatcher(Master);
		if(Master->current == NULL){
			printf("\n");
			printMTH(Master);
			printf("\n");
			printf("No more threads to be executed\n");
			return;
		}
		else if(swapcontext(&Master->t_context, &Master->current->t_context) == -1){
			fprintf(stderr, "Could not swap context to scheduled thread. Error msg: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	printf("idk\n");
	printf("sig handler returning\n");
	printMTH(Master);
	/*
	if(swapcontext(&Master->t_context, &ctx_main) == -1){
		fprintf(stderr, "Could not swap context to scheduled thread. Error msg: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}*/
}


/* Function that executes a thread function
 * The return value is saved in thread->retval
 * Time quanta is also set and is dependent on the priority 
 */
void exec_thread(my_pthread_t* thread, void *(*function)(void*), void* arg){

	thread->state = READY;

	printf("\n\nexec function starting\n");

	// timer set to 25ms quanta, once timer is met scheduler_handler will catch the thread and pass execution time to a new thread
	struct itimerval exec_timer;
	if(signal(SIGALRM, &mt_handler) == SIG_ERR){
		fprintf(stderr, "Could not catch signal. Error message: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if(thread->queue == level3){
		exec_timer.it_value.tv_sec = 0;
		exec_timer.it_value.tv_usec = HIGH_EXEC_TIMEOUT;
		exec_timer.it_interval.tv_sec = 0;
		exec_timer.it_interval.tv_usec = 0;
	}
	else if(thread->queue == level2){
		exec_timer.it_value.tv_sec = 0;
		exec_timer.it_value.tv_usec = MEDIUM_EXEC_TIMEOUT;
		exec_timer.it_interval.tv_sec = 0;
		exec_timer.it_interval.tv_usec = 0;
	}
	else
	{
		exec_timer.it_value.tv_sec = 0;
		exec_timer.it_value.tv_usec = LOW_EXEC_TIMEOUT;
		exec_timer.it_interval.tv_sec = 0;
		exec_timer.it_interval.tv_usec = 0;
	}

	if(setitimer(ITIMER_REAL, &exec_timer, NULL) == -1){
		fprintf(stderr, "Could not set timer. Error message: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	// execute thread function until termination
	printf("setup timer + signal\n");
	thread->state = RUNNING;
	thread-> t_retval = function(arg);
	printf("function executed\n");

	// thread terminates
	thread->state = TERMINATED; 

	// RUN SCHEDULE HANDLER HERE
	printf("entering handler\n");

	//mt_handler(0);

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
	//Thread->t_context = Context;
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

	

	// enqueue process in MLPQ with highest priority
	if(start == 0)
	{
		printf("main thread ready\n\n");
		start = 1;
		Master = (my_pthread_t*)malloc(sizeof(my_pthread_t));
		initializeMTH(Master);
		
		
		if(getcontext(&Master->t_context) == -1){
			fprintf(stderr, "Could not get context. Error message: %s\n", strerror(errno));
			return -1;
		}
		Master->t_context.uc_stack.ss_sp = (void*)malloc(T_STACK_SIZE);
		if(!Master->t_context.uc_stack.ss_sp){
			printf("Malloc didn't work :(\n");
			exit(EXIT_FAILURE);
		}
		Master->t_context.uc_stack.ss_size = T_STACK_SIZE; 
		Master->t_context.uc_link = &ctx_main;
		makecontext(&Master->t_context, (void(*))mt_handler, 0);
		Master->state = READY;
		Master->queue = 3;
		enqueue(Master->High,Master);


		/*
		ctx_handler.uc_stack.ss_sp = (void*)malloc(T_STACK_SIZE);
		if(!ctx_handler.uc_stack.ss_sp){
			printf("Malloc didn't work :(\n");
			free(thread);
			return -1;
		}
		ctx_handler.uc_stack.ss_size = T_STACK_SIZE; 
		ctx_handler.uc_link = &ctx_main;
		makecontext(&ctx_handler, (void(*))mt_handler, 0);

		printf("mt_handler and mt_handler context made\n");

		//Master->tid= 0;
		//Master->t_priority = INT_MAX;
		//Master->t_context = ctx_main;
		*/
	}

	thread = (my_pthread_t*)malloc(sizeof(my_pthread_t));
	printf("\n\nallocated space for thread\n");

	// initialize context
	if(getcontext(&(thread->t_context)) == -1){
		fprintf(stderr, "Could not get context. Error message: %s\n", strerror(errno));
		return -1;
	}
	printf("context is active\n");

	thread->state = NEW;
	printf("Thread state is now NEW\n");

	// allocating stack for thread
	thread->t_context.uc_stack.ss_sp = (void*)malloc(T_STACK_SIZE);
	if(!thread->t_context.uc_stack.ss_sp){
		printf("Malloc didn't work :(\n");
		free(thread);
		return -1;
	}
	thread->t_context.uc_stack.ss_size = T_STACK_SIZE; 
	printf("stack allocated\n");

	// setting my_pthread_t elements
	thread->tid = ++tid;
	thread->t_priority = 7;
	thread->joinid = -1;
	thread->name = (char*)malloc(sizeof("mth_high"));
	thread->name = "mth_high";
	thread->queue = 3;
	printf("%d\n",thread->tid);

	printf("Thread initialized\n");

	// make execution context
	makecontext(&(thread->t_context), (void(*))exec_thread, 3, thread, function, arg);
	printf("context made\n");



	enqueue(Master->High, thread);
	printf("thread enqueued\n");
	thread->state = READY;
	printf("thread state now READY\n\n");
	ctx_main.uc_link = &Master->t_context;
	if(swapcontext(&ctx_main, &(Master->t_context)) == -1){
		fprintf(stderr, "Could not swap context to scheduled thread. Error msg: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	printf("returning\n");
	//mt_handler(0);

	return 0;
}

// explicit call to the my_pthread_t scheduler requesting that the current context can be swapped out
// another context can be scheduled if one is waiting
void my_pthread_yield(){
	Master->current->state = YIELDED;
	mt_handler(0);
	// Set state of running thread to YIELD and have a signal handler run
	// thread.state = YIELDED
}

// explicit call to the my_pthread_t library to end the pthread that called it
// If the value_ptr isn't NULL, any return value from the thread will be saved
// FIX RETURN VAL
void my_pthread_exit(void* value_ptr){
	// Set state of running thread to YIELD and have a signal handler run
	// thread.state = EXITED
	sigprocmask(SIG_BLOCK, &sigProcMask, NULL);
	printf("Thread Exited\n");

	if(Master->current->joinid != -1){
		enqueue(Master->Wait,Master->current);
	}

	Master->current->state = TERMINATED;

	free(Master->current->t_context.uc_stack.ss_sp);//clears stack in thread's context
	dequeueSpecific(Master->current->parent,Master->current);
	sigprocmask(SIG_BLOCK, &sigProcMask, NULL);

	mt_handler(0);
}

int my_pthread_mutex_init(my_pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr){

	//Return error code if cannot init mutex
	if (mutex == NULL) {
		return EINVAL;
	}

	//Enqueue this mutex into the MTH list
	my_pthread_mutex_t* ptr = Master->Mutex;
	while(ptr->next != NULL) {  //find the end of the list
		if(ptr == mutex)
		{
			perror("EINVAL"); 
			return EINVAL;
		}
		ptr = Master->Mutex->next;
	}

	ptr->next = mutex;  //store mutex at the end of MTH mutex list

	mutex->flag = 0;
	mutex->guard = 0;

	return 0;   // return if successful
}
// Call to the my_pthread_t library ensuring that the calling thread will not continue execution until the one references exits
// If value_ptr is not null, the return value of the exiting thread will be passed back
int my_pthread_join(my_pthread_t thread, void** value_ptr){
	if(thread.joinid != -1){
		perror("EINVAL"); return EINVAL;}
	if(thread.tid == Master->current->joinid && Master->current->joinid != -1){
		perror("EDEADLK"); return EDEADLK;}
	if(thread.state == TERMINATED){
		return 1;
	}
	thread.joinid = Master->current->tid;
	my_pthread_yield();
	printf("Joined\n");

	return 1;
}
void yieldD (my_pthread_t* thread) {

	if (thread->queue == level3) {
		move2Q(Master->High, Master->Medium, thread);
		my_pthread_yield();
	}

	else if (thread->queue == level2) {
		move2Q(Master->Medium, Master->Low, thread);
		my_pthread_yield();
	}

	else {    //if already in level 3
		my_pthread_yield();
	}

}
// initializes a my_pthread_mutex_t created by the calling thread
// attributes are ignored

int my_pthread_mutex_lock(my_pthread_mutex_t* mutex){

	if (mutex == NULL) return EINVAL;

	//TODO: Compare atomic jump NEEDS TO BE IMPLEMENTED
	//Check if mutex isn't locked, we can bind to a thread
	if(mutex->guard==0)
	{
		int tide = Master->current->tid;
		int tmp = 1;
		__asm__(
				"xchgl %0, %1;\n"
				: "=r"(mutex->guard), "+m"(tmp)
				: "0"(mutex->guard)
				:"memory");
		mutex->flag = tide;
	}
	else
	{
		yieldD(Master->current);
	}

	return 0;   //return 0 if mutex can be locked
}

// Loccks a given mutex
// Other threads attempting to access this mutex will not run until it is unlocked


// Unlocks a given mutex
int my_pthread_mutex_unlock(my_pthread_mutex_t* mutex){

	//TODO: TEST AND SET FOR CHECK (NEED TO BE DONE)
	if(mutex->guard==1)
	{
		if(mutex->flag == Master->current->tid)
		{
			int a = 0;
			__asm__(
					"xchgl %0, %1;\n"
					: "=r"(mutex->flag), "+m"(a)
					: "0"(mutex->flag)
					:"memory");
		}

		else {
			exit(1);
		}
	}

	else
	{
		exit(1);
	}

	return 0;
}

// Destroys a given mutex
// Mutex should be unlocked before doing so (or deadlock)
int my_pthread_mutex_destroy(my_pthread_mutex_t* mutex){

	printf("Mutex Destroyed\n");
	if(!mutex)return EINVAL;
	mutex->guard = 0;
	mutex->flag = 0;
	return 0;
}



void* t1(void* arg){
	printf("Thread has been created\n");
	int x = 10;
	printf("x = %d\n", x);
	return NULL;
}
void main_test(){


	// allocating stack for thread
	printf("test threads and structs initialized\n");
	my_pthread_t test1;
	my_pthread_t test2;
	printf("\n\nCreating thread 1\n\n");
	my_pthread_create(&test1, NULL, (void*)t1, NULL);
	printf("\n\nCreating thread 2\n\n");
	my_pthread_create(&test2, NULL, (void*)t1, NULL);
	printf("\n");
	printMTH(Master);

	// free
	free(Master);
	printf("\nmain here, returning\n");
}

int main(int argc, char* argv[]){

	main_test();
}
