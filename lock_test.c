
#include <stdio.h>
#include <stdlib.h>
#include "mythreads.h"


mutexlock_t *mylock;
int count = 0;

void *testthread (void *arg)
{
	printf("testthread (start): %d\n",count);

	threadLock(mylock);

	for (int i=0; i < 10; i++) {
		threadYield();
		count++;
	}

	threadUnlock(mylock);
	
	printf("testthread (done): %d\n",count);
	return NULL;
}



int main(void)
{
	int tid;

	//initialize the threading library. DON'T call this more than once!!!
	threadInit();

	//initialize the lock
	mylock = lockCreate();

	
	tid = threadCreate(testthread,NULL);
	count++;
	printf("created thread: %d\n", count);
	
	threadYield();
	threadLock(mylock);
		printf("got lock: %d\n", count);
	threadUnlock(mylock);

	//make sure the thread is done.
	threadJoin(tid, NULL);
	lockDestroy(mylock);

	return EXIT_SUCCESS;
}

