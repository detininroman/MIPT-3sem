#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

enum { PIPE_SIZE = 16 * 1024 };

int main (int argc, char* argv[])
{
	int input            = 0;
	ssize_t input_size   = 0;
	ssize_t output_size  = 0;;
	int pipefd [2]       = {};
	char buf [PIPE_SIZE] = {};

	if (argc != 2) {
		fprintf (stderr, "Wrong arguments!\n");
		exit (EXIT_FAILURE);
	}

	if (pipe (pipefd)  == -1) {
		perror ("pipe");
		exit (EXIT_FAILURE);
	}

	pid_t cpid = fork ();
	
	if (cpid == -1) {
		perror ("fork");
		exit (EXIT_FAILURE);
	}


	if (cpid == 0) {              // Child writes to pipe 
		close(pipefd[0]);		
	        
		input = open (argv [1], O_RDONLY);
		
		if (input == 0) {
			perror ("input file");
			exit (EXIT_FAILURE);
		}

		while ((input_size = read (input, buf, PIPE_SIZE)) > 0) {
			write (pipefd [1], buf, input_size);
		}
		
		//printf ("input_size = %d\n", input_size);
			
		close (pipefd [1]);
		close (input);
		exit (EXIT_SUCCESS);

	} else {                     // Parent reads from pipe 
		close(pipefd[1]);	
		
		while ((output_size = 
			read (pipefd [0], buf, PIPE_SIZE)) > 0) {
			write (STDOUT_FILENO, buf, output_size);
		}

		close (pipefd [0]);
		wait (NULL);
		exit (EXIT_SUCCESS);
	}	
	return 0;	
}
