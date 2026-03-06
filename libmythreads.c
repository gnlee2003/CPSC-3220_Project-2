#include <stdlib.h>
#include <stdio.h>
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
    int locked; //0 Unlocked - 1 Locked
    int *waitingThreads;
    int numWaiting;
    void *thread;
}mutexlock_t;

typedef struct condvar{
    int *waitingThreads;
    int numWaiting;
} condvar_t;

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
    int threadCount;
}threadHeader_t;

int interruptsAreDisabled = 0;

int lockCount = 0;
int condvarCount = 0;
threadHeader_t *Header = NULL;
thread_t *currentThread = NULL;

static void interruptDisable(void){
    assert(!interruptsAreDisabled);
    interruptsAreDisabled = 1;
}

static void interruptEnable(void){
    assert(interruptsAreDisabled);
    interruptsAreDisabled = 0;
}

void myThreadHandler(){
    interruptEnable();
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
        for (int i = 0; i < (*num); i++){ //check for duplicates
            if ((*list)[i] == id){
                return;
            }
        }
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

thread_t *NextReadyThread(){
    thread_t *rover = Header -> front;

    while(rover != NULL){
        if (rover -> status == READY && rover -> threadId != currentThread -> threadId){
            return rover;
        }
        rover = rover -> next;
    }

    return NULL;
}

void RemoveLLElement(thread_t *element){
    assert(element != NULL);

    if (element->prev != NULL){
        element->prev->next = element->next;
    } else {
        Header->front = element->next;
    }

    if (element->next != NULL){
        element->next->prev = element->prev;
    } else {
        Header->back = element->prev;
    }

    Header->threadCount--;

    if (element->threadId != 0 && element->stack != NULL){
        free(element->stack);
    }
    free(element->waitingThreads);
    free(element);
}

void threadInit(void) { 
    Header = (threadHeader_t *)malloc(sizeof(threadHeader_t));
    Header -> front = NULL;
    Header -> back = NULL;
    Header -> threadCount = 0;
    threadCreate(NULL, NULL);
}

int threadCreate(thFuncPtr funcPtr, void *argPtr) {
    interruptDisable();
    thread_t *newThread = (thread_t *)malloc(sizeof(thread_t));
    newThread -> threadId = Header -> threadCount;
    newThread -> waitingThreads = NULL;
    newThread -> numWaiting = 0;
    newThread -> funcptr = funcPtr;
    newThread -> argptr = argPtr;
    newThread -> next = NULL;

    if(Header -> threadCount == 0){ //If this is the main function
        newThread -> stack = NULL;
        Header -> front = newThread;
        Header -> back = newThread;
        newThread -> prev = NULL;
        newThread -> status = RUNNING;
        Header -> threadCount++;
        currentThread = newThread;
        getcontext(&newThread -> context);
        interruptEnable();
        return newThread -> threadId;
    }else{
        newThread->prev = Header->back;
        Header->back->next = newThread;
        Header->back = newThread;
        Header -> threadCount++;
        newThread -> stack = malloc(STACK_SIZE);
        getcontext(&newThread -> context);
        newThread -> context.uc_stack.ss_sp = newThread -> stack;
        newThread -> context.uc_stack.ss_size = STACK_SIZE;
        makecontext(&newThread -> context, myThreadHandler, 0);
        currentThread -> status = READY;
        newThread -> status = RUNNING;
        thread_t *temp = currentThread;
        currentThread = newThread;
        swapcontext(&temp -> context, &newThread ->context);
        if (interruptsAreDisabled) interruptEnable();
        return newThread -> threadId;
    }
}

void threadYield(void){
    if (!interruptsAreDisabled) interruptDisable();
    thread_t *next = NextReadyThread();

    if (next != NULL) {
        thread_t *temp = currentThread;
        currentThread = next;
        next -> status = RUNNING;
        temp -> status = READY;
        swapcontext(&temp -> context, &next -> context);
        temp -> status = RUNNING;
        currentThread = temp;
    }

    //If none were found
    if(interruptsAreDisabled) interruptEnable();
    return;
} 

void threadJoin(int thread_id, void **result) {
    interruptDisable();
    thread_t *rover = Header -> front;

    while (rover != NULL){
        if (rover -> threadId == thread_id){
            if (rover -> status == EXITED){ //If thread has already exited
                if (result != NULL) *result = rover -> result;
                interruptEnable();
                return;
            }  

            currentThread -> status = WAITING;
            waitingListHandler(&rover -> waitingThreads, &rover -> numWaiting, ADDINGLIST, currentThread -> threadId); //Add to waiting list
            thread_t *readyThread = NextReadyThread();
            thread_t *temp = currentThread;
            currentThread = readyThread;
            readyThread -> status = RUNNING;
            temp -> status = WAITING;
            swapcontext(&temp -> context, &readyThread -> context); //Swap to next Ready thread
            if(interruptsAreDisabled) interruptEnable();

            rover = Header -> front;
            while (rover -> threadId != thread_id){
                rover = rover -> next;
            }

            assert(rover != NULL);
            assert(rover -> status == EXITED);
            if (result != NULL) *result = rover -> result;
            RemoveLLElement(rover);
            return;
        }
        rover = rover -> next;
    }
    if(interruptsAreDisabled) interruptEnable();
    return; //thread not found
}

void threadExit(void *result) {
    interruptDisable();
    thread_t *temp = currentThread;

    currentThread -> status = EXITED;
    currentThread -> result = result;


    while (temp -> numWaiting > 0){
        int id = temp -> waitingThreads[0];

        thread_t *rover = Header -> front;
        while (rover != NULL){
            if (rover -> threadId == id){
                rover -> status = READY;
                break;
            }
            rover = rover -> next;
        }
        waitingListHandler(&temp -> waitingThreads, &temp -> numWaiting, REMOVINGLIST, NOID);
    }

    thread_t *next = NextReadyThread();

    next -> status = RUNNING;
    currentThread = next;

    interruptEnable();
    setcontext(&next -> context);

    return;
}

mutexlock_t *lockCreate(void) {
    interruptDisable();
    mutexlock_t *newLock = (mutexlock_t *)malloc(sizeof(mutexlock_t));
    newLock -> locked = UNLOCKED;
    newLock -> thread = NULL;
    newLock -> waitingThreads = NULL;
    newLock -> numWaiting = 0;

    interruptEnable();
    return newLock; 
}

void lockDestroy(mutexlock_t *lock) {
    interruptDisable();
    free(lock -> waitingThreads);
    free(lock);
    interruptEnable();
}

void threadLock(mutexlock_t *lock){
    interruptDisable();

    while (lock -> locked == LOCKED){
        waitingListHandler(&lock -> waitingThreads, &lock -> numWaiting, ADDINGLIST, currentThread -> threadId);

        currentThread -> status = WAITING;
        thread_t *next = NextReadyThread();
        if (next != NULL){
            thread_t *temp = currentThread;
            currentThread = next;

            next -> status = RUNNING;
            swapcontext(&temp -> context, &next -> context);
            temp -> status = RUNNING;
            currentThread = temp;
        }
    }

    assert(lock -> locked == UNLOCKED);
    lock -> locked = LOCKED;
    lock -> thread = currentThread;

    if(interruptsAreDisabled) interruptEnable();
    return;
}

void threadUnlock(mutexlock_t *lock){
    interruptDisable();
    lock -> locked = UNLOCKED;
    lock -> thread = NULL;

    if (lock -> numWaiting > 0){
        int id = lock -> waitingThreads[0];
        waitingListHandler(&lock -> waitingThreads, &lock -> numWaiting, REMOVINGLIST, NOID);

        thread_t *rover = Header -> front;
        while(rover != NULL && rover -> threadId != id) rover = rover -> next;

        if (rover != NULL) rover -> status = READY;
    }

    interruptEnable();
}

condvar_t *condvarCreate(void) {
    condvar_t *newVar = (condvar_t *)malloc(sizeof(condvar_t));
    newVar -> numWaiting = 0;
    newVar -> waitingThreads = NULL;

    return newVar;
}

void condvarDestroy(condvar_t *cv) {
    interruptDisable();
    free(cv -> waitingThreads);
    free(cv);
    interruptEnable();
}

void threadWait(mutexlock_t *lock, condvar_t *cv) {
    interruptDisable();
    waitingListHandler(&cv -> waitingThreads, &cv -> numWaiting, ADDINGLIST, currentThread -> threadId);

    //threadUnlock Logic here to avoid interrupt issues
    lock -> locked = UNLOCKED;
    lock -> thread = NULL;
    if (lock -> numWaiting > 0){
        int id = lock -> waitingThreads[0];
        waitingListHandler(&lock -> waitingThreads, &lock -> numWaiting, REMOVINGLIST, NOID);
        thread_t *rover = Header -> front;
        while(rover != NULL && rover -> threadId != id) rover = rover -> next;
        if (rover != NULL) rover -> status = READY;
    }

    currentThread -> status = WAITING;
    thread_t *next = NextReadyThread();

    if (next != NULL){
        thread_t *temp = currentThread;
        next -> status = RUNNING;
        currentThread = next;

        swapcontext(&temp -> context, &next -> context);

        temp -> status = RUNNING;
        currentThread = temp;
    }

    //ThreadLock logic here to avoid interrupt issues
    while(lock -> locked == LOCKED){
        waitingListHandler(&lock -> waitingThreads, &lock -> numWaiting, ADDINGLIST, currentThread -> threadId);
        currentThread -> status = WAITING;
        thread_t *next = NextReadyThread();
        if(next != NULL){
            thread_t *temp = currentThread;
            currentThread = next;
            next -> status = RUNNING;
            swapcontext(&temp -> context, &next -> context);
            temp -> status = RUNNING;
            currentThread = temp;
        }
    }
    lock -> locked = LOCKED;
    lock -> thread = currentThread;

    if (interruptsAreDisabled) interruptEnable();
    return;
}

void threadSignal(mutexlock_t *lock, condvar_t *cv) {
    interruptDisable();
    if (cv -> numWaiting > 0){
        int id = cv -> waitingThreads[0];
        waitingListHandler(&cv -> waitingThreads, &cv -> numWaiting, REMOVINGLIST, NOID);

        thread_t *rover = Header -> front;
        while(rover != NULL){
            if (rover -> threadId == id){
                rover -> status = READY;
                interruptEnable();
                return;
            }
            rover = rover -> next;
        }
    }

    interruptEnable();
    return;
}
