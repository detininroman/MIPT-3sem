#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

/* ------------------------------------ */
#define CHECK(cond, func) {              \
    if (!(cond)) {                       \
        printf ("Error: %s\n", func);    \
        printf ("Line: %d\n", __LINE__); \
        exit (EXIT_FAILURE);             \
    }                                    \
}
/* ------------------------------------ */

enum { MAX_NUM = 1 << 7 };

pid_t pid = 0;
sigset_t set = 0;
int mask = MAX_NUM;
int received_char = 0;

/* HANDLER is to control the reaction on SIG signal */
void init_sig_action (void (*handler)(int), int sig)
{
    struct sigaction act = {};
    act.sa_flags = 0;
    act.sa_handler = handler;
    CHECK(sigaction(sig, &act, NULL) != -1, "sigaction");
}

/* ------------------------ */
/* -- SIGUSR1 equal to 1 -- */
/* -- SIGUSR2 equal to 0 -- */
/* ------------------------ */

void bit_handler (int sig)
{
    if (sig == SIGUSR1) {
        received_char |= mask;
    }
    mask >>= 1;
    CHECK(kill(pid, SIGUSR1) != -1, "kill");
}

/* confirmation from parent */
void dummy_handler (int sig) {}

void parent ()
{
    init_sig_action (exit, SIGCHLD);
    init_sig_action (bit_handler, SIGUSR1);
    init_sig_action (bit_handler, SIGUSR2);

    CHECK(sigemptyset(&set) != -1, "sigemptyset");

    for (;;)
    {
        if (!mask) {
            CHECK(write (STDOUT_FILENO, &received_char, 1) != -1,
                    "write");
            fflush (stdout);
            mask = MAX_NUM;
            received_char = 0;
        }
        sigsuspend (&set);
    }
}

void child (char* input)
{
    CHECK(input, "NULLPTR")

    init_sig_action (exit, SIGALRM);
    init_sig_action (dummy_handler, SIGUSR1);

    CHECK(sigemptyset (&set) != -1, "sigemptyset");

    pid_t parent = getppid ();

    int src = open (input, O_RDONLY);
    CHECK (src != -1, "open");
 
    char byte = 0;
    int cnt = 0;
    while (read (src, &byte, 1) > 0) {
        alarm (1);
        cnt = MAX_NUM;
        while (cnt > 0) { 
            if (cnt & byte) // bit 1
                kill (parent, SIGUSR1);
            else            // bit 0
                kill (parent, SIGUSR2);
            
            cnt >>= 1;
            /* wait a signal from parent */
            sigsuspend (&set);
        }
    }
    exit (EXIT_SUCCESS);
}

int main (int argc, char** argv)
{
    CHECK(argc == 2, "Invalid argument");

    CHECK(sigemptyset (&set) != -1, "sigemptyset");
    CHECK(sigaddset (&set, SIGUSR1) != -1, "sigaddset");
    CHECK(sigaddset (&set, SIGUSR2) != -1, "sigaddset");
    CHECK(sigaddset (&set, SIGCHLD) != -1, "sigaddset");
    CHECK(sigprocmask (SIG_BLOCK, &set, NULL) != -1, "sigprocmask");

    pid = fork();

    if (pid > 0) {
        parent ();
    } else if (pid == 0) {
        child (argv[1]);
    } else { 
        perror ("Fork:");
    }
    return 0;
}
