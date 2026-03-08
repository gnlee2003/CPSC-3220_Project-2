#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h> 
#include <string.h>
#include <unistd.h>
#include "mythreads.h"
#include <signal.h>

#define BIG 100000
#define TIMER_INTERVAL_US 100

//#define USE_VIRTUAL_TIMER

//this function will be called
//everytime that the timer fires
//...then I call yield.
void timer_handler (int signum) 
{  
    //printf(".\n");
	if (!interruptsAreDisabled)
	{
		printf("--------->$$$$ interupting\n");
		threadYield();
	}
} 

//setup the timer that will preempt your thread
//library
void set_timer()
{
	struct sigaction sa; 
	struct itimerval timer; 

	/* Install timer_handler as the signal handler for SIGVTALRM.  */ 
	memset (&sa, 0, sizeof (sa)); 
	sa.sa_handler = &timer_handler;

#ifdef USE_VIRTUAL_TIMER
	sigaction (SIGPROF, &sa, NULL);
#else 
	sigaction (SIGALRM, &sa, NULL); 
#endif

  	/* Configure the timer to expire after TIMER_INTERVAL_US usec...  */ 
	timer.it_value.tv_sec = 0; 
	timer.it_value.tv_usec = TIMER_INTERVAL_US; 

	/* ... and every TIMER_INTERVAL_US usec after that.  */ 
	timer.it_interval.tv_sec = 0; 
	timer.it_interval.tv_usec = TIMER_INTERVAL_US; 

  	/* Start a virtual timer. It counts down whenever this process is executing.  */ 
#ifdef USE_VIRTUAL_TIMER
	if (0 != setitimer (ITIMER_PROF, &timer, NULL))
#else
	if (0 != setitimer (ITIMER_REAL, &timer, NULL))
#endif
	{
		perror("setitimer error");
	} 
}

void *t1 (void *arg)
{
	int param = *((int*)arg);
	printf("t1 started %d\n",param);

	int* result = malloc(sizeof(int));
	
	*result = param;
	for (int i=0; i < (param * BIG); i++)
	{
		*result += 1;
	}

	
	printf ("finished with math! (%d)\n",*result);
	
	sleep(1);
	printf("t1: done result=%d (%p)\n",*result,result);
	return result;
}



int main(void)
{
	int id1, id2;
	int p1;
	int p2;

	p1 = 23;
	p2 = 2;

	int *result1, *result2;

	//initialize the threading library. DON'T call this more than once!!!
	threadInit();

	//I will always start the timer after threadInit
	set_timer();

	id1 = threadCreate(t1,(void*)&p1);
	printf("created thread 1.\n");	
	
	for (int i=0; i < 100; i++)
	{
		id2 = threadCreate(t1,(void*)&p2);
		printf("created thread %d.\n",id2);
	}

	

	threadJoin(id1, (void*)&result1);
	printf("joined #1 --> %d (%p).\n",*result1,result1);

	threadJoin(id2, (void*)&result2);
	printf("joined #%d --> %d (%p).\n",id2,*result2,result2);

}

