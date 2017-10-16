#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <string.h>

#define ERROR() {                                   \
    printf ("ERROR!\n");                            \
    printf ("FUNCTION: %s\n", __PRETTY_FUNCTION__); \
    printf ("LINE: %d\n", __LINE__);                \
    exit   (EXIT_FAILURE);                          \
}

#define SHMEM_SIZE 4096
#define SEM_KEY 2107
#define SHM_KEY 2107

/*
 * struct sembuf
 * {
 *      unsigned short int sem_num;
 *      short int sem_op;
 *      short int sem_flg;
 * };
 *
 */

union semnum {
    int val;
    struct semid_ds* buf;
    unsigned short* array;
} sem_arg;

int writer (char* input) 
{
    int shm_id = shmget (SHM_KEY, SHMEM_SIZE, 
            IPC_CREAT | IPC_EXCL | 0600);
    if (shm_id == -1) ERROR();
 
    int sem_id = semget (SEM_KEY, 1, 
            IPC_CREAT | IPC_EXCL | 0600);
    if (sem_id == -1) ERROR();
   
    printf ("Semaphore: %d\n", sem_id);
    
    unsigned short sem_vals [1] = {};
    sem_vals [0] = 1;
    sem_arg.array = sem_vals;
    
    if (semctl (sem_id, 0, SETALL, sem_arg) == -1) ERROR();

    char* shm_buf = (char*) shmat (shm_id, NULL, 0);
    if (shm_buf == (char*) -1) ERROR();

    struct shmid_ds ds = {};
    if (shmctl (shm_id, IPC_STAT, &ds) == -1) ERROR();
    
    int inp_fd = open (input, O_RDONLY);
    if (inp_fd == -1) ERROR();
 
    struct stat inp_buf;
    stat (input, &inp_buf);
    long file_size = inp_buf.st_size;

    if (ds.shm_segsz < file_size) ERROR();
    if (read (inp_fd, shm_buf, file_size) <= 0) ERROR();
 
    close (inp_fd);
 
    printf ("ID: %d\n", shm_id);

    struct sembuf sb [1];

    sb [0].sem_num = 0;
    sb [0].sem_flg = SEM_UNDO; 
    sb [0].sem_op = -1;
    semop (sem_id, sb, 1);

    sb [0].sem_op = -1;
    semop (sem_id, sb, 1);

    semctl (sem_id, 1, IPC_RMID, sem_arg);
    shmdt (shm_buf);
    shmctl (shm_id, IPC_RMID, NULL);
    
    return 0;
}

int reader ()
{
    int shm_id = shmget (SHM_KEY, 1, 0600);
    if (shm_id == -1) ERROR();
 
    int sem_id = semget (SEM_KEY, 1, 0600);
    if (sem_id == -1) ERROR();

    char* shm_buf = (char*) shmat (shm_id, 0, 0);
    if (shm_buf == (char*) -1) ERROR();

    printf ("MSG: %s", shm_buf);
    
    struct sembuf sb [1];
    sb[0].sem_num = 0;
    sb[0].sem_flg = SEM_UNDO;
    sb[0].sem_op = 1;
    semop (sem_id, sb, 1);

    shmdt (shm_buf);
    return 0;
}

int main (int argc, char** argv)
{
    if (argc == 1) return reader ();
    if (argc == 2) return writer (argv [1]);
    
    printf ("Incorrect arguments!\n");
    return 0;
}
