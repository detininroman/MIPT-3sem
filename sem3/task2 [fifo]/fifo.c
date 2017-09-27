#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

const char* pipe_name = "pipe";
enum { PIPE_SIZE = 16384 };

int send    (int input);
int recieve (int output);

int main (int argc, char* argv[])
{
	if (argc == 1)
	{	
		return recieve (STDOUT_FILENO);
	}

	else if (argc == 2) 
	{
		int input = open (argv[1], O_RDONLY);
		if (input == -1) {
			perror ("Can't open the file!\n");
			exit (EXIT_FAILURE);
		}
		
		return send (input);
	}
	
	perror ("Incorrect arguments!\n");
	return 0;
}

int send (int input)
{
	char buf [PIPE_SIZE] = {};
	ssize_t count = 0;

	int mkfifo_ret_val = mkfifo (pipe_name, 0666);
	if (mkfifo_ret_val == -1) return 0;
	
	//printf ("mkfifo_ret_val = %d \n", mkfifo_ret_val);

	int pipe = open (pipe_name, O_WRONLY);
	if (pipe == -1) {
	perror ("Can't open the pipe!\n");
		exit (EXIT_FAILURE);
	}

	while ((count = read (input, buf, PIPE_SIZE)) > 0) {
		write (pipe, buf, count);
	}
	
	close (input);
	close (pipe);

	return 0;
}

int recieve (int output)
{
	char buf [PIPE_SIZE] = {};
	int counter = 0;

	int pipe = open (pipe_name, O_RDONLY);
	if (pipe == -1)  {
		perror ("Can't open the pipe!\n");
		exit (EXIT_FAILURE);
	}

	while ((counter = read (pipe, buf, PIPE_SIZE)) > 0) {
		write (output, buf, counter);
	}
	
	close (pipe);
	close (output);
	unlink (pipe_name);

	return 0;
}
