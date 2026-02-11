
#include <ucontext.h>

#define STACK_SIZE (16*1024)  

//the type of function used to run your threads
typedef void *(*thFuncPtr) (void *); 

extern void threadInit(void);
extern int threadCreate(thFuncPtr funcPtr, void *argPtr); 
extern void threadYield(void); 
extern void threadJoin(int thread_id, void **result);

//exits the current thread -- closing the main thread, will terminate the program
extern void threadExit(void *result); 

//mutex lock types and functions.
struct mutexlock; //opaque type -- you need to implement this in your source file
typedef struct mutexlock mutexlock_t;

extern mutexlock_t * lockCreate(void);
extern void lockDestroy(mutexlock_t * lock);
extern void threadLock(mutexlock_t *lock); 
extern void threadUnlock(mutexlock_t *lock); 

//condition variable types and functions
struct condvar; //opaque type -- you need to implement this in your source file
typedef struct condvar condvar_t;

extern condvar_t * condvarCreate(void);
extern void condvarDestroy(condvar_t * cv);
extern void threadWait(mutexlock_t* lock, condvar_t *cv); 
extern void threadSignal(mutexlock_t* lock, condvar_t *cv); 

//this 
extern int interruptsAreDisabled;