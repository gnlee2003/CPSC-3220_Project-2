
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

// note in your project, you don't want these contexts to be
//  local or global but allocated on the heap instead.
ucontext_t first_context, second_context;
int x = 0;

int foo(int value) {
	printf("foo(%d) x = %d\n", value, x);
	if (x < 1) {
		x++;
		// save current context to second_context, //switch to first_context
		printf("before swap (v = %d, x == %d)\n", value, x);
		swapcontext(&second_context, &first_context);
		printf("after swap (v = %d, x == %d)\n", value, x);
	}
	return x;
}

int main(int argc, char **argv) {
	int v = 0;
	// save the context --- we'll jump back here later.
	getcontext(&first_context);
	v++;
	int r = foo(v);
	printf("done. (r = %d).\n", r);
	setcontext(&second_context); // you would never want to do this.
	printf("The end (r = %d).\n", r);
}
