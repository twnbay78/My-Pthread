#include <stdio.h>
#include <signal.h>
#include <ucontext.h>
#include <string.h>
#include <limits.h>
#include <math.h>


enum Queue_Type {MasterThreadHandler = 0, Wait = 1,Cleaner = 2,level1 = 3,level2 = 4,level3 = 5};
enum special {removal = 26};
//enum boo {true=1,false=0};
typedef struct _tcb {
  int tid;
  int t_priority;
  char *name;
  struct _tcb* parent;
  ucontext_t* thread_context;
  struct _tcb* next;
 // enum boo isTail;
} my_pthread_t;



typedef struct _MTH{
   struct _tcb* Low;
   struct _tcb* Medium;
   struct _tcb* High;
   struct _tcb* Wait;
   struct _tcb* Cleaner;
}MTH;

//Queue Programs

my_pthread_t* runT(my_pthread_t* head)
{
 my_pthread_t* start = head;
  while(start->next!=NULL)
    start = start->next;
return start;
}

void printQueue(my_pthread_t* head) {

  printf("%d, ", head->tid);
  while (head->next != NULL) {
    head = head->next;
    printf("%d, ", head->tid);
  }

  printf("\n");
}
void enqueue(my_pthread_t* head, my_pthread_t* newThread){

  //pQueue head is the front of the ArrayList with no context that keeps track of the "queue".

  my_pthread_t* ending = runT(head);    //Find end of ArrayList
  ending->next = newThread;       //Insertion of newThread
  newThread->next = NULL;
  newThread->parent = head;
  //newThread->isTail=true;

}
void printMTH(MTH* main)
{
  printQueue(main->Low);
  printQueue(main->Medium);
  printQueue(main->High);
  printQueue(main->Wait);
 printQueue(main->Cleaner);
}

void printThread(my_pthread_t* node)
{
  printf("Tid: %d Priority: %d Name: %s\n",node->tid,node->t_priority,node->name);
}
my_pthread_t* peek(my_pthread_t* head){
  if(head->next !=NULL)
    return (head->next);
  return NULL;
}

void dequeue(my_pthread_t* head) {
  
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
/*void changePriority(my_pthread_t* p , long int tw, long int tr){
   
   long double calcWait = log((long double)tw);
   double calcRun = 1/(sqrt((double)tr));

   if(calcRun < INT_MAX && calcWait < INT_MA && calcRun>26 && calcWait>26 )
    {
      if(calcRun > calcWait)
        p->t_priority = calcRun;
      else
        p->t_priority = calcWait;
    }
    else{
      p->t_priority = INT_MAX;
    }
}*/
my_pthread_t* createThread(my_pthread_t* Thread,ucontext_t* Context)
{
  Thread->tid = rand(); // find a way to assign values
  Thread->t_priority = INT_MAX; // make priority highest
  Thread->name = NULL;
  Thread->thread_context = Context;
  Thread->next = NULL;
  Thread->parent = NULL;
  //Thread->isTail=false;
  return Thread;
}

void initQ(my_pthread_t * lead,char* header,int t){
  lead->tid = t;
  lead->t_priority = 0;
  lead->thread_context = NULL;
  lead->next = NULL;
  lead->name = header;
  lead->parent = NULL;
 // lead->isTail=false;
}
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
