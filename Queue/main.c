#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>

typedef struct {
	long mtype;
} msg_buf;

long get_number (char* arg);

int main (int argc, char** argv) 
{
    if (argc != 2) {
        perror ("Incorrect number of arguments!\n");
        exit (EXIT_FAILURE);
    }
    
    long frk_num = get_number (argv [1]);
    key_t key    = ftok (argv [0], 0);
    int msgid    = msgget (key, IPC_CREAT | 0600);
    msg_buf buf  = {};
    pid_t pid    = 0;

	for (int i = 0; i < frk_num; i++) 
    {
		if ((pid = fork()) == 0) // child 
        {
            msgrcv (msgid, &buf, 0, buf.mtype, 0);
            printf ("%ld\n", buf.mtype);
            buf.mtype++;
            msgsnd (msgid, &buf, 0, 0);
            return 0;
        }
		if (pid == -1) exit (EXIT_FAILURE);
    }   

    buf.mtype = 1;
    msgsnd (msgid, &buf, 0, 0);
    msgrcv (msgid, &buf, 0, frk_num + 1, 0);
    msgctl (msgid, IPC_RMID, NULL);
    return 0;
}
