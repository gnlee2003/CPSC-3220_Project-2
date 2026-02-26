#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>

#include "mythreads.h"

#define RUNNING 1
#define WAITING 2
#define READY 3
#define EXITED 4

#define LOCKED 1
#define UNLOCKED 0

#define ADDINGLIST 1
#define REMOVINGLIST 0
#define NOID 0

typedef struct mutexlock{
    int locked;
    int *waitingThreads;
    int numWaiting;
    void *thread;
}mutexlock_t;

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

typedef struct{ //going to use this as the header for threads and locks
    thread_t *front;
    thread_t *back;
    int structCount;
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
void waitingListHandler(int **list, int *num, int op, int id){
    if (op == 1){ // adding to list
        (*num)++;
        *list = realloc(*list, sizeof(int) * (*num));
        (*list)[(*num) - 1] = id;
    }else{ // removing from list (removes first element)
        if (*num <= 0) return;

        (*list)[0] = 0;
        for (int i = 0; i < (*num) - 1; i++){
            (*list)[i] = (*list)[i + 1];
        }
        (*num)--;

        if (*num == 0){
            free(*list);
            *list = NULL;
        } else{
            *list = realloc(*list, sizeof(int) * (*num));
        }
    }
}

void threadInit(void) { 
    Header = (threadHeader_t *)malloc(sizeof(threadHeader_t));
    Header -> front = NULL;
    Header -> back = NULL;
    Header -> structCount = 0;
    threadCreate(NULL, NULL);
}

int threadCreate(thFuncPtr funcPtr, void *argPtr) {
    thread_t *newThread = (thread_t *)malloc(sizeof(thread_t));
    if (Header -> structCount != 0) newThread -> stack = malloc(STACK_SIZE);
    else newThread -> stack = NULL;

    newThread -> threadId = Header -> structCount;
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
    Header -> structCount++;
    if (Header -> structCount == 1){
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

    while (rover != NULL){
        if (rover -> threadId == thread_id){
            if(rover -> status == EXITED) return;

            waitingListHandler(&rover -> waitingThreads, &rover -> numWaiting, ADDINGLIST, currentThread -> threadId);
            currentThread -> status = WAITING;
            rover -> status = RUNNING;
            thread_t *temp = currentThread;
            currentThread = rover;
            swapcontext(&temp -> context, &rover -> context);
            
            assert(rover -> status == EXITED);
            if (result != NULL){
                *result = rover -> result;
            }
            return;
        }
        rover = rover -> next;
    }
    return; //thread not found
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
        waitingListHandler(&rover -> waitingThreads, &rover -> numWaiting, REMOVINGLIST, NOID);
        thread_t *temp = currentThread;
        currentThread = rover;
        swapcontext(&rover -> context, &temp -> context);
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
    mutexlock_t *newLock = (mutexlock_t *)malloc(sizeof(mutexlock_t));
    newLock -> locked = UNLOCKED;
    newLock -> thread = NULL;

    return newLock;
}

void lockDestroy(mutexlock_t *lock) {
    free(lock);
}

void threadLock(mutexlock_t *lock){
    if (lock -> locked == LOCKED){
        waitingListHandler(&lock -> waitingThreads, &lock -> numWaiting, 1, currentThread -> threadId);
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
                currentThread -> status = WAITING;
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
    }else{
        lock -> thread = currentThread;
        lock -> locked = LOCKED;
    }
}
void threadUnlock(mutexlock_t *lock){
    lock -> locked = UNLOCKED;
    lock -> thread = NULL;
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
