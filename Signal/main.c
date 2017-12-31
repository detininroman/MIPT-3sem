#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

/* ----------------------------------------------------- */
#define CHECK(cond, func) {                               \
    if (!(cond)) {                                        \
        printf ("Error: %s\nLine: %d\n", func, __LINE__); \
        exit (EXIT_FAILURE);                              \
    }                                                     \
}
/* ----------------------------------------------------- */

enum { 
    INIT_MASK = 1 << 7,
    BUF_SIZE  = 1 << 12
};

char bit = -1;

void set_handler (void (*handler)(int), int sig)
{
    struct sigaction act = {};
    act.sa_handler = handler;
    CHECK(sigaction(sig, &act, NULL) != -1, "sigaction");
}

void bit_handler (int sig)
{
    bit = (sig == SIGUSR1)? 1: 0;
}

void dummy_handler (int sig) {}

void parent (int child)
{
    set_handler (exit, SIGCHLD);
    set_handler (bit_handler, SIGUSR1);
    set_handler (bit_handler, SIGUSR2);

    sigset_t set;
    CHECK(sigemptyset(&set) != -1, "sigemptyset");
 
    int mask = INIT_MASK;
    char received_char = 0;
    
    for (;;) {
        if (!mask) {
            CHECK(write (STDOUT_FILENO, &received_char, 1) != -1,
                    "write");
            fflush (stdout);
            mask = INIT_MASK;
            received_char = 0;
        }
        sigsuspend (&set);
     
        if (bit == 1) 
            received_char |= mask;
        mask >>= 1;

        CHECK(kill(child, SIGUSR1) != -1, "kill");
    }
}

void child (char* input)
{
    CHECK(input, "NULLPTR")

    set_handler (exit, SIGALRM);
    set_handler (dummy_handler, SIGUSR1);

    sigset_t set;
    CHECK(sigemptyset (&set) != -1, "sigemptyset");

    pid_t parent = getppid ();

    int src = open (input, O_RDONLY);
    CHECK(src != -1, "open");
 
    ssize_t rbytes = 0;
    char buf[BUF_SIZE] = {};
    while ((rbytes = read (src, buf, BUF_SIZE)) > 0) {
        alarm (1);
        for (int byte_cnt = 0; byte_cnt < rbytes; byte_cnt++) {
            for (int c_mask = INIT_MASK; c_mask > 0; c_mask >>= 1) {
                if (c_mask & buf [byte_cnt]) {
                    kill (parent, SIGUSR1); // bit 1
                } else {                      
                    kill (parent, SIGUSR2); // bit 0
                }
                /* wait a signal from parent */
                sigsuspend (&set);
            }
        }
    }
    exit (EXIT_SUCCESS);
}

int main (int argc, char** argv)
{
    CHECK(argc == 2, "Invalid argument");

    sigset_t set;

    //int child_pid = fork ();

    CHECK(sigemptyset (&set) != -1, "sigemptyset");
    CHECK(sigaddset (&set, SIGUSR1) != -1, "sigaddset");
    CHECK(sigaddset (&set, SIGUSR2) != -1, "sigaddset");
    CHECK(sigaddset (&set, SIGCHLD) != -1, "sigaddset");
    CHECK(sigprocmask (SIG_BLOCK, &set, NULL) != -1, "sigprocmask");
    
    int child_pid = fork ();
 
    switch (child_pid) {
        case -1: perror ("Fork:");
        case  0: child  (argv[1]);
        default: parent (child_pid);
    }
    return 0;
}
