#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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
	ssize_t file_size = 0;

	int mkfifo_result = mkfifo (pipe_name, 0777);
	if (mkfifo_result == -1) return 0;
	
	//printf ("mkfiforesult = %d \n", mkfifo_result);

	int pipe = open (pipe_name, O_WRONLY);
	if (pipe == -1) {
		perror ("Can't open the pipe!\n");
		exit (EXIT_FAILURE);
	}

	while ((file_size = read (input, buf, PIPE_SIZE)) > 0) {
		write (pipe, buf, file_size);
	}
	
	close (input);
	close (pipe);

	return 0;
}

int recieve (int output)
{
	char buf [PIPE_SIZE] = {};
	int file_size = 0;

	int pipe = open (pipe_name, O_RDONLY);
	if (pipe == -1)  {
		perror ("Can't open the pipe!\n");
		exit (EXIT_FAILURE);
	}

	while ((file_size = read (pipe, buf, PIPE_SIZE)) > 0) {
		write (output, buf, file_size);
	}
	
	close (pipe);
	close (output);
	unlink (pipe_name);

	return 0;
}
