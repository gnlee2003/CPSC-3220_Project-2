/* An example of how to use function pointers */

#include <stdio.h>

//declare some functions

int iadd(int a, int b){
	return a+b;
}

int isub(int a, int b){
	return a-b;
}

int imult(int a, int b){
	return a*b;
}

int idiv(int a, int b){
	return a/b;
}

int do_op(int (*op)(int,int), int a, int b)
{
	return op(a,b);
}

int main()
{
	int (*op)(int,int) = iadd;


	printf("a: %d\n",iadd(50,7));
	printf("a: %d\n",do_op(iadd, 50,7));
	printf("s: %d\n",do_op(isub, 50,7));
	printf("m: %d\n",do_op(imult, 50,7));
	printf("d: %d\n",do_op(idiv, 50,7));
	printf("a: %d\n",do_op(op, 50,7));
	
}
