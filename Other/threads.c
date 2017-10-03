#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

void* stream (void* param)
{
	printf ("Thread: %ld\n", (long)param);
	return 0;
}


int main(int argc, char* argv[])
{
	int base;
	char* endptr;
	
	if (argc < 2) 
	{
		printf ("Incorrect number of arguments\n");
		return 0;
	}
	
	base = 10;

	long max = strtol (argv[1], &endptr, base);

	if (endptr == argv[1]) 
	{
		printf ("You didn't entered a number\n");
		return 0;
	}

	if (max < 1)
	{
		printf ("Wrong number\n");
		return 0;
	}
	
	if ((errno == ERANGE && (max == LONG_MAX || max == LONG_MIN))
	                   || (errno != 0 && max == 0)) 
	{
	      printf ("Too big number\n");
	      return 0;
	}
	
	if (*endptr != '\0')
	{
		printf ("Wrong number\n");
		return 0;
	}
	
	printf ("Argument: %ld \n", max);
	
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	
	pthread_t tid; 
	
	for (long i = 0; i < max; i++)
	{
		pthread_create (&tid, NULL, stream, (void*) i);
	}

	return 0;
}

