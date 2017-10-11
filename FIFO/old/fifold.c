//DEAD READER - IMPROVE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define ERROR() {                                   \
    printf ("ERROR!\n");                            \
    printf ("FUNCTION: %s\n", __PRETTY_FUNCTION__); \
    printf ("LINE: %d\n", __LINE__);                \
    exit   (EXIT_FAILURE);                          \
}

const char* main_fifo_name = "./temp/main_fifo";

enum
{
	NAME_LENGTH = 32,
	READ_SIZE   = 16
};

int writer (char* filename);
int reader (int output);

int main (int argc, char** argv)
{
	if      (argc == 2) return writer (argv [1]);
	else if (argc == 1) return reader (STDOUT_FILENO);

	printf ("Wrong arguments!\n");
	return 0;
}

int writer (char* filename)
{
	//creating main fifo 
	int main_mkfifo_ret_val = mkfifo (main_fifo_name, 0666);
    int fd_main_fifo = open (main_fifo_name, O_WRONLY);
	if (fd_main_fifo == -1) ERROR();

	//creating n fifos for n writers
	int mkfifo_ret_val = -1;
   	int cur_fifo_ID = 0;
 	char id_fifo_path [NAME_LENGTH] = {};
    while (mkfifo_ret_val == -1) {
		cur_fifo_ID++;
		sprintf (id_fifo_path, "./temp/%d", cur_fifo_ID);
		mkfifo_ret_val = mkfifo (id_fifo_path, 0666);
	}
	
    //filling main fifo with the ID 
	int* cur_fifo_ID_ptr = &cur_fifo_ID;
	write (fd_main_fifo, (char *) cur_fifo_ID_ptr, sizeof (int));
    //return 0; //FIXED
	
    //opening the input file
	int input = open (filename, O_RDONLY);
    if (input == -1) ERROR();

	//creating name for current fifo
    if (sprintf (id_fifo_path, "./temp/%d", cur_fifo_ID) <= 0)
        ERROR();
	
    int fd_ID_fifo = open (id_fifo_path, O_WRONLY);
	if (fd_ID_fifo == -1) ERROR();

	//writing from input to current fifo
	ssize_t count = 0;
	char buf [NAME_LENGTH] = {};
	while ((count = read (input, buf, READ_SIZE)) > 0) {
		write (fd_ID_fifo, buf, count);
	}
	
	close (input);
	close (fd_ID_fifo);
	close (fd_main_fifo);

	return 0;
}

int reader (int output)
{
    int fd_main_fifo = open (main_fifo_name, O_RDONLY);
	if (fd_main_fifo == -1) ERROR();

    //reading current fifo name 
    int cur_fifo_ID = 0;
    if (read (fd_main_fifo, (char*) &cur_fifo_ID, 
                                sizeof (int)) <= 0) ERROR();
    //return 0;

    //translating name to string
	char id_fifo_path [NAME_LENGTH]= {};
	if (sprintf (id_fifo_path, "./temp/%d", cur_fifo_ID) <= 0)
        ERROR();

	//opening current fifo && changing flags 
	int fd_ID_fifo = open (id_fifo_path, O_RDONLY | O_NONBLOCK);
    if (fd_ID_fifo == -1) ERROR();
    int flag = fcntl (fd_ID_fifo, F_GETFL, 0);
    fcntl (fd_ID_fifo, F_SETFL, flag & ~O_NONBLOCK);

	//writing text to stdout
    int count = 0;
	char buf [NAME_LENGTH] = {};
    while ((count = read (fd_ID_fifo, buf, READ_SIZE)) > 0) {
        if (write (output, buf, count) <= 0) ERROR();
	}

	close  (fd_ID_fifo);
	unlink (id_fifo_path);
	close  (fd_main_fifo);

	return 0;
}
