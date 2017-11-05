#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>

//#define D_PRINT(...) printf (__VA_ARGS__)
#define D_PRINT

#define ERROR() {                                   \
    printf ("ERROR!\n");                            \
    printf ("FUNCTION: %s\n", __PRETTY_FUNCTION__); \
    printf ("LINE: %d\n", __LINE__);                \
    exit   (EXIT_FAILURE);                          \
}

const char* main_fifo = "./temp/main_fifo";

enum 
{
    SLEEPTIME = 100000,
    MAX_LEN   = 16
};

int writer (char* input) 
{
    // Create & open main FIFO
    mkfifo (main_fifo, 0666);
    int main_fifo_fd = open (main_fifo, O_RDWR);
    if (main_fifo_fd == -1) ERROR();
    pid_t pid = 0;
    read (main_fifo_fd, &pid, sizeof (pid_t));
     
    // enter critical section
   
    close (main_fifo_fd);

    // exit (0);
    D_PRINT("LINE: %d\n", __LINE__);

    // Open current FIFO & change flags
    char cur_fifo_name [MAX_LEN] = {};
    if (sprintf (cur_fifo_name, "./temp/%d.fifo", pid) <= 0)
        ERROR();
 
    int cur_fifo_fd = open (cur_fifo_name, O_WRONLY|O_NONBLOCK);
    if (cur_fifo_fd == -1) ERROR();
    int flag = fcntl (cur_fifo_fd, F_GETFL, 0);
    fcntl (cur_fifo_fd, F_SETFL, flag & ~O_NONBLOCK);

    // exit (0);
    D_PRINT("LINE: %d\n", __LINE__);

    // Open input file
    int inp_fd = open (input, O_RDONLY);
    if (inp_fd == -1) ERROR();

    // exit (0);
    D_PRINT("LINE: %d\n", __LINE__);

    // Write from input to current FIFO 
    ssize_t bytes = 0;
    char buf [PIPE_BUF] = {};

    // leave critival section
   
    while ((bytes = read (inp_fd, buf, PIPE_BUF)) > 0) 
    {
        write (cur_fifo_fd, buf, bytes);
    }

    close (inp_fd);
    close (cur_fifo_fd);
    return 0;
}

int reader (int output)
{
    pid_t pid = getpid();

    // exit (0);
    D_PRINT("LINE: %d\n", __LINE__);

    // Create & open current FIFO
    char cur_fifo_name [MAX_LEN] = {};
    sprintf (cur_fifo_name, "./temp/%d.fifo", pid);
    int ret_mkfifo = mkfifo (cur_fifo_name, 0666);
    if (ret_mkfifo == -1) ERROR();
    int cur_fifo_fd = open (cur_fifo_name, O_RDONLY|O_NONBLOCK);
    if (cur_fifo_fd == -1) ERROR();

    // exit (0);
    D_PRINT("LINE: %d\n", __LINE__);

    // Create & open main FIFO
    mkfifo (main_fifo, 0666);
    int main_fifo_fd = open (main_fifo, O_WRONLY);
    write (main_fifo_fd, &pid, sizeof (pid_t));
    close (main_fifo_fd);

    // exit (0);
    D_PRINT("LINE: %d\n", __LINE__);

    // Wait for synchronization
    usleep (SLEEPTIME);
    D_PRINT("LINE: %d\n", __LINE__);
    int flag = fcntl(cur_fifo_fd, F_GETFL, 0);
    fcntl(cur_fifo_fd, F_SETFL, flag & ~O_NONBLOCK);
    unlink(cur_fifo_name);
    
    // exit (0);
    D_PRINT("LINE: %d\n", __LINE__);

    // Write text to stdout
    char buf [PIPE_BUF];
    ssize_t bytes = 0;
    while ((bytes = read (cur_fifo_fd, buf, PIPE_BUF)) > 0) {
        write (output, buf, bytes);
    }

    close (cur_fifo_fd);
    return 0;
}

int main (int argc, char** argv) 
{
    if      (argc == 1) return reader (STDOUT_FILENO);
    else if (argc == 2) return writer (argv[1]);
        
    printf ("ERROR! Wrong arguments!\n");
    return 0;
}
