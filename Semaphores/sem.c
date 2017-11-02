#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>

#include "sem.h"

int main (int argc, char** argv)
{
    key_t key = ftok ("sem.h", 0);
    if (key == -1) _ERROR();

    int shmid = shmget (key, shmem_size, 0600 | IPC_CREAT);
    if (shmid == -1) _ERROR();
    
    char* shm_buf = (char*) shmat (shmid, NULL, 0);
    if (shm_buf == (char*) -1) _ERROR();

    int semid = semget (key, SEMNUM, 0600 | IPC_CREAT);
    if (semid == -1) _ERROR();
    
    struct sembuf sb [SEMNUM] = {};

    if (argc == 1) {
        client (STDOUT_FILENO, semid, shmid, shm_buf, sb);
    } else if (argc == 2) {  
        server (argv [1], semid, shmid, shm_buf, sb);
    } else {
        perror ("Incorrect arguments!\n");
        exit (EXIT_FAILURE);
    }

    _PRINT("Semid: %d\nShmid: %d\n", semid, shmid);

    shmdt  (shm_buf);
    shmctl (shmid, IPC_RMID, NULL);
    semctl (semid, 0, IPC_RMID);
    return 0;
}

void fill_sb (struct sembuf* sb, int sembuf_num, 
        unsigned short num, short op, short flg) 
{
    sb[sembuf_num].sem_num = num;  
    sb[sembuf_num].sem_op  = op; 
    sb[sembuf_num].sem_flg = flg;
}

void server (char* input, int semid, int shmid, 
        char* shm_buf, struct sembuf* sb) 
{
    if (!input || !shm_buf || !sb) _ERROR();
 
    /* check if server is alone & begin */
    fill_sb (sb, 0, SRV,  0, IPC_NOWAIT);
    fill_sb (sb, 1, SRV, +1, SEM_UNDO);
    semop (semid, sb, 2);

    /* wait for client */
    fill_sb (sb, 0, CLI, -1, 0);
    fill_sb (sb, 1, CLI, +1, 0);
    semop (semid, sb, 2);

    /* open input file */
    int inp_fd = open (input, O_RDONLY);
    if (inp_fd == -1) _ERROR();
 
    /* get file size */
    struct stat inp_buf;
    stat (input, &inp_buf);
    long file_size = inp_buf.st_size;
 
    //_EXIT();

    /* check if client is still alive */
    fill_sb (sb, 0, CLI, -1, IPC_NOWAIT);
    fill_sb (sb, 1, CLI, +1, 0);
    fill_sb (sb, 2, MUT,  0, 0);
    semop (semid, sb, 3);

    //_EXIT();

    /* read from input file */
    if (read (inp_fd, shm_buf, file_size) <= 0) _ERROR();
    close (inp_fd);
  
    /* increase mutex */
    fill_sb (sb, 0, MUT, +1, 0);
    semop (semid, sb, 1);
}

void client (int output, int semid, int shmid, 
        char* shm_buf, struct sembuf* sb)
{
    /* check if client is alone & begin */
    fill_sb (sb, 0, CLI,  0, IPC_NOWAIT);
    fill_sb (sb, 1, CLI, +1, SEM_UNDO);
    semop (semid, sb, 2);

    /* wait for server */
    fill_sb (sb, 0, SRV, -1, 0); 
    fill_sb (sb, 1, SRV, +1, 0);
    semop (semid, sb, 2); 
    
    //_EXIT();

    /* check if server is still alive */
    fill_sb (sb, 0, SRV, -1, IPC_NOWAIT);
    fill_sb (sb, 1, SRV, +1, 0);
    /* wait for permission */
    fill_sb (sb, 2, MUT, -1, 0);
    semop (semid, sb, 3);

    //_EXIT();
    
    /* write to stdout */
    if (write (output, shm_buf, shmem_size) <= 0) _ERROR();
   
    /* data is read */
    fill_sb (sb, 0, MUT, +1, SEM_UNDO);
    semop (semid, sb, 1);
}
