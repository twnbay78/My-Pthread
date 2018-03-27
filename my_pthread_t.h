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
#define hanbdle_error(msg) \
  do { fprintf(stderr, "There was an error. Error message: %s\n", strerror(msg); exit(EXIT_FAILURE); } while(0)
#define TESTING
#define HIGH_EXEC_TIMEOUT 25000
#define MEDIUM_EXEC_TIMEOUT 37000
#define LOW_EXEC_TIMEOUT 50000
#define T_STACK_SIZE 1048576
#define MAINTENANCE_THRESHOLD 2000000


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
sigset_t sigProcMask;
// Enum to represent queue types
typedef enum Queue_Type {
  MasterThreadHandler = 0, 
  Wait = 1,
  Cleaner = 2,
  level1 = 3,
  level2 = 4,
  level3 = 5,
  mutex = 6,
  removal = 26
}queue_type;


typedef struct _mutex {
  int flag;
  int guard;
  struct _mutex* next;
}my_pthread_mutex_t;

/*  Thread Control Block - holds metadata on structs 
 *
 * tid: thread id
 * t_priority ->  priority level of the thread - determines queue level
 * name -> indicates which queue the thread is currently in (debugging/output purposes)
 *  - mth = master thread handler aka highest level ready queue
 * t_retval -> the return value of the thread function
 * t_context -> the context in which the thread holds
 *  - registers
 *  - stack 
 *  - sigmasks
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
  ucontext_t t_context;
  t_state state;
  queue_type queue;
  int joinid;
  long int start_init;
  long int start_exec;
  long int start_ready;
  long int start_wait;
  long int total_exec;
  long int total_ready;
  long int total_wait;
  struct _tcb* parent;
  struct _tcb* next;
   struct _tcb* High;
   struct _tcb* Medium;
   struct _tcb* Low;
  struct _tcb* Wait;
   struct _tcb* Cleaner;
  my_pthread_mutex_t* Mutex;   //single wait list for mutexes
   unsigned int high_size;
   unsigned int medium_size;
   unsigned int low_size;
   unsigned int wait_size;
   unsigned int mutex_size;
   struct _tcb* current;
} my_pthread_t;


/* Special MTH struct that holds all of the queues
 * high -> 25ms quanta
 * medium -> 37ms quanta
 * low -> 50ms quanta
 * cleaner -> terminated threads that are waiting to be destroyed
 */
typedef struct _MTH{

}MTH;

// - - - - - - - GLOBAL VARIABLES - - - - - - - - // 
extern unsigned int tid;

//extern my_pthread_t* Master;
//extern ucontext_t ctx_main;
//extern ucontext_t ctx_handler;

#endif
