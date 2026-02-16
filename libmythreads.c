#include <stdlib.h>
#include "mythreads.h"

int interruptsAreDisabled = 0;

/* Define the opaque types */
struct mutexlock { int dummy; };
struct condvar   { int dummy; };

void threadInit(void) { }

int threadCreate(thFuncPtr funcPtr, void *argPtr) {
    return 0;
}

void threadYield(void) { }

void threadJoin(int thread_id, void **result) {
    return;
}

void threadExit(void *result) {
    return;
}

mutexlock_t *lockCreate(void) {
    return (mutexlock_t *)calloc(1, sizeof(struct mutexlock));
}

void lockDestroy(mutexlock_t *lock) {
    return;
}

void threadLock(mutexlock_t *lock) { (void)lock; }
void threadUnlock(mutexlock_t *lock) { (void)lock; }

condvar_t *condvarCreate(void) {
    return (condvar_t *)calloc(1, sizeof(struct condvar));
}

void condvarDestroy(condvar_t *cv) {
    return;
}

void threadWait(mutexlock_t *lock, condvar_t *cv) { (void)lock; (void)cv; }
void threadSignal(mutexlock_t *lock, condvar_t *cv) { (void)lock; (void)cv; }
