#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

enum {
    SRV = 0,
    SRV_PREV,
    CLI,
    CLI_PREV,
    SHM,
    MUT,
    SEMNUM
};

enum { BUF_SIZE = 4096 };

struct shmem {
    ssize_t bytes_read;
    char buffer[BUF_SIZE];
};

int shmid = -1;
int semid = -1;
struct shmem* shmem = (struct shmem*) -1;
struct sembuf sbuf[SEMNUM] = {};
short sbuf_cnt = 0;

void fill_sb (unsigned short num, short op, short flg) 
{
    sbuf[sbuf_cnt].sem_num = num;
    sbuf[sbuf_cnt].sem_op  = op;
    sbuf[sbuf_cnt].sem_flg = flg;
    sbuf_cnt++;
}

/* --------------- SOME DEFINES --------------- */

#ifdef DEBUG
#define PRINT(...)   printf (__VA_ARGS__)
#define PRINT_LINE() printf ("LINE: %d\n", __LINE__);
#else
#define PRINT(...)
#define PRINT_LINE()
#endif

#define EXIT() {                                    \
    printf ("Exit in line: %d\n", __LINE__);        \
    printf ("Function: %s\n", __PRETTY_FUNCTION__); \
    exit   (EXIT_SUCCESS);                          \
} 

#define ERROR() {                                   \
    perror ("ERROR!");                              \
    printf ("Function: %s\n", __PRETTY_FUNCTION__); \
    printf ("Line: %d\n", __LINE__);                \
    exit   (EXIT_FAILURE);                          \
}

/* --------------- MORE DEFINES --------------- */

#define _SEMOP_ {                              \
    if (semop (semid, sbuf, sbuf_cnt) == -1) { \
        perror ("Semop");                      \
        exit (EXIT_FAILURE);                   \
    }                                          \
    sbuf_cnt = 0;                              \
}

#define _WAIT_FOR_(side) { \
    fill_sb(side, -1, 0);  \
    fill_sb(side, +1, 0);  \
}

#define _CHECK_IF_ALIVE_(side) {   \
    fill_sb(side, -1, IPC_NOWAIT); \
    fill_sb(side, +1, 0);          \
}

#define _MUTEX_UP_ {            \
    fill_sb(MUT,  0, 0);        \
    fill_sb(MUT, +1, SEM_UNDO); \
}

#define _MUTEX_DOWN_ { fill_sb(MUT, -1, SEM_UNDO); }

#define _CHECK_UNIQUENESS_(side) {fill_sb(side, 0, IPC_NOWAIT);}
   
#define _EXIST_(side) { fill_sb(side, +1, SEM_UNDO); } 

#define _WAIT_UNTIL_SERVER_READS_ { fill_sb(SHM, -1, 0); }    
       
#define _WAIT_UNTIL_CLIENT_WRITES_ { fill_sb(SHM,  0, 0); }

#define _SERVER_HAVE_READ_ { fill_sb(SHM, +1, 0); }    

/*-------------------------------------------------------------*/

void client () 
{
    _CHECK_UNIQUENESS_(CLI);
    _CHECK_UNIQUENESS_(CLI_PREV);
    _EXIST_(CLI);
    _SEMOP_;

    PRINT_LINE();

    _WAIT_FOR_(SRV);
    _EXIST_(SRV_PREV);
    _SEMOP_;

    PRINT_LINE();

    ssize_t wbytes = 1;
    while (wbytes)
    {
        _CHECK_IF_ALIVE_(SRV);
        _WAIT_UNTIL_SERVER_READS_;
        _MUTEX_UP_;
        _SEMOP_;

        wbytes = shmem->bytes_read;
        if (write (STDOUT_FILENO, shmem->buffer, wbytes) == -1)
            ERROR();

        _MUTEX_DOWN_;
        _SEMOP_;
    }
}

void server (char* input) 
{
    if (!input) ERROR();
   
    _CHECK_UNIQUENESS_(SRV);
    _CHECK_UNIQUENESS_(SRV_PREV);
    _EXIST_(SRV);
    _SEMOP_;

    PRINT_LINE();

    _WAIT_FOR_(CLI);
    _EXIST_(CLI_PREV);
    _SEMOP_;

    PRINT_LINE();

    int inp_fd = open (input, O_RDONLY);
    if (inp_fd == -1) ERROR();

    ssize_t rbytes = 1;
    while (rbytes) 
    {
        _CHECK_IF_ALIVE_(CLI);
        _WAIT_UNTIL_CLIENT_WRITES_;
        _MUTEX_UP_;
        _SEMOP_;

        rbytes = read (inp_fd, shmem->buffer, BUF_SIZE);
        if (rbytes == -1) ERROR();
        shmem->bytes_read = rbytes;

        _SERVER_HAVE_READ_;
        _MUTEX_DOWN_;
        _SEMOP_;
    } 
    close (inp_fd);
}

int main (int argc, char** argv)
{
    key_t key = ftok ("sem.c", 0);
    if (key == -1) ERROR();
   
    shmid = shmget (key, sizeof(struct shmem), 0600 | IPC_CREAT);
    if (shmid == -1) ERROR();
    
    shmem = (struct shmem*) shmat (shmid, NULL, 0);
    if (shmem == (struct shmem*) -1) ERROR();
    
    semid = semget (key, SEMNUM, 0600 | IPC_CREAT);
    if (semid == -1) ERROR();

    if (argc == 1) {
        client ();
        shmdt (shmem);
        shmctl (shmid, IPC_RMID, NULL);
        semctl (semid, 0, IPC_RMID);
    } else if (argc == 2) {
        server (argv[1]);
    } else {
        ERROR();
    }
}
