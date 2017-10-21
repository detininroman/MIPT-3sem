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
    /* get common sources */
    key_t key = ftok ("sem.h", 0);
    if (key == -1) ERROR();

    int shmid = shmget (key, shmem_size, 0600 | IPC_CREAT);
    if (shmid == -1) ERROR();
    
    char* shm_buf = (char*) shmat (shmid, NULL, 0);
    if (shm_buf == (char*) -1) ERROR();

    int semid = semget (key, SEMNUM, 0600 | IPC_CREAT);
    if (semid == -1) ERROR();
    
    struct sembuf sb [SEMNUM] = {};

    if (argc == 1)
        reader (STDOUT_FILENO, semid, shmid, shm_buf, sb);
    else if (argc == 2)  
        writer (argv [1], semid, shmid, shm_buf, sb);
    else {
        perror ("Incorrect arguments!\n");
        exit (EXIT_FAILURE);
    }

    PRINT("Semid: %d\nShmid: %d\n", semid, shmid);

    /* remove semaphores and shared memory */
    shmdt  (shm_buf);
    shmctl (shmid, IPC_RMID, NULL);
    semctl (semid, 0, IPC_RMID);
    return 0;
}

/* fill sembuf with required values */
void fill_sb (struct sembuf* sb, int sembuf_num, 
        unsigned short num, short op, short flg) 
{
    sb[sembuf_num].sem_num = num;  
    sb[sembuf_num].sem_op  = op; 
    sb[sembuf_num].sem_flg = flg;
}

void writer (char* input, int semid, int shmid, 
        char* shm_buf, struct sembuf* sb) 
{
    /* All cases of writer's death are considered */

    if (!input) ERROR();
 
    /* open input file */
    int inp_fd = open (input, O_RDONLY);
    if (inp_fd == -1) ERROR();
 
    exit (0);

    /* get file size */
    struct stat inp_buf;
    stat (input, &inp_buf);
    long file_size = inp_buf.st_size;
 
    //exit (0);

    /* read from input file */
    if (read (inp_fd, shm_buf, file_size) <= 0) ERROR();
    close (inp_fd);
   
    //exit (0);

    /* allow reader to continue */ 
    fill_sb (sb, 0, WFWR, +1, 0);  
    semop (semid, sb, 1);
    
    //exit (0);

    /* change mutex */
    fill_sb (sb, 0, MUT,  -1, SEM_UNDO);
    semop (semid, sb, 1);
}

void reader (int output, int semid, int shmid, 
        char* shm_buf, struct sembuf* sb)
{
    /* if reader is first           */
    /* wait for writer's permission */
    fill_sb (sb, 0, WFWR, -1, 0);
    semop (semid, sb, 1);

    //exit (0);

    /* write to stdout */
    if (write (output, shm_buf, shmem_size) <= 0) ERROR();
   
    //exit (0);

    /* change semaphore */
    fill_sb (sb, 0, MUT, +1, SEM_UNDO);
    semop (semid, sb, 1);
}
