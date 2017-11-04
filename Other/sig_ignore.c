#include <stdio.h>
#include <signal.h>
#include <assert.h>

void sig_handler (int snum)
{
    fprintf (stderr, "signal...\n");
}

int main ()
{
    struct sigaction act;
    sigemptyset (&act.sa_mask);
    act.sa_handler = &sig_handler;
    act.sa_flags = 0;
    
    while (1) {
    if (sigaction (SIGINT, &act, NULL) == -1) 
        assert (0);
    }
    return 0;
}
