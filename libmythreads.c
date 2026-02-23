#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>

#include "mythreads.h"

#define RUNNING 1
#define WAITING 2
#define READY 3
#define EXITED 4

struct mutexlock_t{
    int lockId;
    int locked;
    void *thread;
};

struct condvar_t{
    int condvarId;
    int state;
    void *thread;
};

typedef struct thread{
    int threadId;
    int status;
    void *stack;
    struct thread *next;
    struct thread *prev;
    ucontext_t context;
    void *(*funcptr)(void *);
    void *argptr;
    void *result;
    int *waitingThreads;
    int numWaiting;
} thread_t;

typedef struct{
    thread_t *front;
    thread_t *back;
    int threadCount;
}threadHeader_t;

int interruptsAreDisabled = 0;

int lockCount = 0;
int condvarCount = 0;
threadHeader_t *Header = NULL;

thread_t *currentThread = NULL;

void myThreadHandler(){
    if (currentThread != NULL && currentThread -> funcptr != NULL){
        currentThread -> result = currentThread -> funcptr(currentThread -> argptr);        
    }
    threadExit(currentThread -> result);
}

/*
Handles the waiting List
Takes three arguments
thread = pointer to thread to be worked
op = 1 adding to list
op = 0 removing the first
ID = ID to be queued
ID = 0 if op == 0
*/
void waitingListHandler(thread_t *thread, int op, int id){
    if (op == 1){ //adding to list
        thread -> numWaiting++;
        thread -> waitingThreads = realloc(thread -> waitingThreads, sizeof(int) * thread -> numWaiting);
        thread -> waitingThreads[thread -> numWaiting - 1] = id;
    }else{ //removing from list
        thread -> waitingThreads[0] = 0;
        for (int i = 0; i < thread -> numWaiting - 1; i++){
            thread -> waitingThreads[i] = thread -> waitingThreads [i + 1];
        }
        thread -> numWaiting--;
        thread -> waitingThreads = realloc(thread -> waitingThreads, sizeof(int) * thread -> numWaiting);
    }
}

void threadInit(void) { 
    Header = (threadHeader_t *)malloc(sizeof(threadHeader_t));
    Header -> front = NULL;
    Header -> back = NULL;
    Header -> threadCount = 0;
    threadCreate(NULL, NULL);
}

int threadCreate(thFuncPtr funcPtr, void *argPtr) {
    thread_t *newThread = (thread_t *)malloc(sizeof(thread_t));
    if (Header -> threadCount != 0) newThread -> stack = malloc(STACK_SIZE);
    else newThread -> stack = NULL;

    newThread -> threadId = Header -> threadCount;
    newThread -> status = RUNNING;
    newThread -> next = NULL;
    getcontext(&newThread -> context);
    newThread -> context.uc_stack.ss_sp = newThread -> stack;
    newThread -> funcptr = funcPtr;
    newThread -> argptr = argPtr;
    newThread -> result = NULL;
    newThread -> waitingThreads = NULL;
    newThread -> numWaiting = 0;

    thread_t *rover = Header -> front;
    Header -> threadCount++;
    if (Header -> threadCount == 1){
        currentThread = newThread;
        Header -> front = newThread;
        Header -> back = newThread;
        return newThread -> threadId;
    }else{
        newThread -> context.uc_stack.ss_size = STACK_SIZE;
        rover = Header -> back;
        newThread -> prev = rover;
        rover -> next = newThread;
        Header -> back = newThread;
        makecontext(&newThread -> context, myThreadHandler, 0);
        thread_t *temp = currentThread;
        currentThread = newThread;
        swapcontext(&temp -> context, &newThread -> context);
        return newThread -> threadId;
    }
}

void threadYield(void){
    //Check after the current thread in the LL
    thread_t *rover = currentThread -> next;
    if (rover != NULL){
        while(rover != currentThread){
            if (rover -> status == READY){
                break;
            }
            if (rover == NULL) rover = Header -> front; //Check before currentThread in LL
            rover = rover -> next;
        }

        if (rover == currentThread){
            return;
        }else{
            currentThread -> status = READY;
            rover -> status = RUNNING;
            thread_t *temp = currentThread;
            currentThread = rover;
            swapcontext(&temp -> context, &currentThread -> context);
            return;
        }
    }else{
        getcontext(&currentThread -> context);
        return;
    }
}

void threadJoin(int thread_id, void **result) {
    thread_t *rover = Header -> front;

    while(rover != NULL){ 
        if (rover -> threadId == thread_id){ //Search for desired thread_id
            if (rover -> status == EXITED){
                return;
            }else{
                currentThread -> status = WAITING;
                rover -> numWaiting++;
                rover -> waitingThreads = realloc(rover -> waitingThreads, rover -> numWaiting);
                rover -> waitingThreads[rover -> numWaiting - 1] = currentThread -> threadId;
                rover -> status = RUNNING;
                thread_t *temp = currentThread;
                currentThread = rover;
                swapcontext(&temp -> context, &currentThread -> context);
            }
        }
        rover = rover -> next;
    }
    return;
}

void threadExit(void *result) {
    currentThread -> status = EXITED;
    currentThread -> result = result;

    if (currentThread -> numWaiting > 0){
        int id = currentThread -> waitingThreads[0];
        thread_t *rover = Header -> front;
        while(rover != NULL){
            if (rover -> threadId == id) break;
            rover = rover -> next;
        }
        assert(id == rover -> threadId);
        assert(rover != NULL);

        rover -> status = RUNNING;
        waitingListHandler(rover, 0, 0);
        swapcontext(&rover -> context, &currentThread -> context);
    }else{
        thread_t *rover = Header -> front;
        while(rover != currentThread){
            if (rover -> status == READY){
                break;
            }
        if (rover == NULL) rover = Header -> front; //Check before currentThread in LL
            rover = rover -> next;
        }

        if (rover == currentThread){
            return;
        }else{
            currentThread -> status = READY;
            rover -> status = RUNNING;
            thread_t *temp = currentThread;
            currentThread = rover;
            swapcontext(&temp -> context, &currentThread -> context);
            return;
        }
    }
    return;
}

mutexlock_t *lockCreate(void) {
    return NULL;
}

void lockDestroy(mutexlock_t *lock) {
    return;
}

void threadLock(mutexlock_t *lock){

}
void threadUnlock(mutexlock_t *lock){

}

condvar_t *condvarCreate(void) {
    return NULL;
}

void condvarDestroy(condvar_t *cv) {
    return;
}

void threadWait(mutexlock_t *lock, condvar_t *cv) {
    
}

void threadSignal(mutexlock_t *lock, condvar_t *cv) {
    
}
