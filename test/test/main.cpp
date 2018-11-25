#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <malloc.h>

void malloc_test(struct A *x);
void free_test(struct A *x);
int * malloc_test1(struct A *x);
struct A
{
	int a1;
	int a2;
	int *pa;
};
struct B
{
	int b1;
	int b2;
	int *pb;
};
int main()
{
	struct A *aa1 = (struct A *)malloc(sizeof(A));
	aa1->a1 = 1;
	aa1->a2 = 2;
	//aa1.pa =(int *) malloc(10);
	aa1->pa = malloc_test1( aa1 );
	//aa1->pa = (int *)malloc(10);
	aa1->pa[0] = 8;
	
	struct B *bb1 = (struct B *)malloc(sizeof(B));;
	bb1->b1 = aa1->a1;
	bb1->b2 = aa1->a2;
	//bb1->pb = aa1.pa;

	printf("%d\n", aa1->a1*bb1->b2);
	free_test(aa1);
	printf("%d\n", aa1->a1*bb1->b2);
	int cc = 0;
	return 0;
}

void malloc_test(struct A *x)
{
	x = (struct A *)malloc(sizeof(A));
}

int *  malloc_test1(struct A *x)
{
	return (int *)malloc(10);
}

void free_test(struct A *x)
{
	free(x);
	x = NULL;
}