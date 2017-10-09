#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

//#define PRINT(...) printf (__VA_ARGS__)
#define PRINT(...)

const char* main_fifo_name = "./temp/main_fifo";

enum
{
	NAME_LENGTH = 32,
	READ_SIZE = 16
};

int sender   (char* filename);
int reciever (int output);

int main (int argc, char** argv)
{
	if (argc == 2) {
		 return sender (argv[1]);
	}

	else if (argc == 1) {
		 return reciever (STDOUT_FILENO);
	}

	perror ("Wrong arguments!\n");
	return 0;
}

int sender (char* filename)
{
	int mkfifo_ret_val = -1;
	int cur_fifo_ID = 0;
	char id_fifo_path [NAME_LENGTH] = {};
	
	//creating main fifo 
	int main_mkfifo_ret_val = mkfifo (main_fifo_name, 0666);
	int fd_main_fifo = open (main_fifo_name, O_WRONLY);
	if (fd_main_fifo == -1) {
		perror ("Can't open main fifo!\n");
		exit (EXIT_FAILURE);
	}

	//creating n fifos for n senders
	while (mkfifo_ret_val == -1) {
		cur_fifo_ID++;
		sprintf (id_fifo_path, "./temp/%d", cur_fifo_ID);
		mkfifo_ret_val = mkfifo (id_fifo_path, 0666);
	}
	
    //filling main fifo with the ID 
	int* cur_fifo_ID_ptr = &cur_fifo_ID;
	write (fd_main_fifo, (char *) cur_fifo_ID_ptr, sizeof (int));
    // exit (0);
	
    //opening the input file
	int input = open (filename, O_RDONLY);
	if (input == -1) {
		perror ("Can't open input file!\n");
		exit (EXIT_FAILURE);
	}

	//creating name for current fifo
	int sprintf_ret_val = sprintf (id_fifo_path, "./temp/%d", cur_fifo_ID);
	if (sprintf_ret_val <= 0) {
		perror ("Can't make id_fifo_path!\n");
		exit (EXIT_FAILURE);
	}
	
    int fd_ID_fifo = open (id_fifo_path, O_WRONLY /*| O_NONBLOCK*/);
    if (fd_ID_fifo == -1) {
		perror ("Can't open current fifo!\n");
        exit (EXIT_FAILURE);
	}
    /*
    //changing flags
    int flag = fcntl (fd_ID_fifo, F_GETFL, 0);
    flag = flag & ~O_NONBLOCK;
    fcntl (fd_ID_fifo, F_SETFL, flag);
    */
   	//writing from input to current fifo
	ssize_t count = 0;
	char buf [NAME_LENGTH] = {};
	while ((count = read (input, buf, READ_SIZE)) > 0) {
		write (fd_ID_fifo, buf, count);
	}
	
	close (input);
	close (fd_ID_fifo);
	close (fd_main_fifo);

	PRINT ("cur_fifo_ID: %d\n", cur_fifo_ID);
	return cur_fifo_ID;
}

int reciever (int output)
{
	int cur_fifo_ID = 0;
	char buf [NAME_LENGTH] = {};
	int count = 0;

	int fd_main_fifo = open (main_fifo_name, O_RDONLY);
	
    //reading current fifo name 
	read (fd_main_fifo, (char *) &cur_fifo_ID, sizeof (int));
    // exit (0);

    //translating name to string
	char id_fifo_path [NAME_LENGTH]= {};
	int sprintf_ret_val = sprintf (id_fifo_path, "./temp/%d", cur_fifo_ID);
	if (sprintf_ret_val <= 0) {
		perror ("Can't make id_fifo_path!\n");
		exit (EXIT_FAILURE);
	}

	//opening current fifo 
	int fd_ID_fifo = open (id_fifo_path, O_RDONLY | O_NONBLOCK);

    //changing flags
    int flag = fcntl (fd_ID_fifo, F_GETFL, 0);
    flag = flag & ~O_NONBLOCK;
    fcntl (fd_ID_fifo, F_SETFL, flag);

	//writing text to stdout
	while ((count = read (fd_ID_fifo, buf, READ_SIZE)) > 0) {
        write (output, buf, count);
	}

	close  (fd_ID_fifo);
	unlink (id_fifo_path);
	close  (fd_main_fifo);

	PRINT ("cur_fifo_ID: %d\n", cur_fifo_ID);
	return cur_fifo_ID;
}

