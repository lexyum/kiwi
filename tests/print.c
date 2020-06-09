#include <stdio.h>

void slave(int s)
{
	printf("slave process, fd %d\n", s);
}

void master(int m)
{
	printf("master process, fd %d\n", m);
}
