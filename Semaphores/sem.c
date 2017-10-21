#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define NDEBUG

#ifdef DEBUG
#define PRINT(...) printf(__VA_ARGS__)
#else
#define PRINT(...)
#endif

#define ERROR() {                                   \
    printf ("ERROR!\n");                            \
    printf ("FUNCTION: %s\n", __PRETTY_FUNCTION__); \
    printf ("LINE: %d\n", __LINE__);                \
    exit   (EXIT_FAILURE);                          \
}


/* fill sembuf with required values */
void fill_sb (struct sembuf* sb, int sembuf_num, 
        int num, int op, int flg) 
{
    sb[sembuf_num].sem_num = num;  
    sb[sembuf_num].sem_op  = op; 
    sb[sembuf_num].sem_flg = flg;
}

const size_t shmem_size = 16384;

int writer (char* input, int semid, int shmid, 
        char* shm_buf, struct sembuf* sb) 
{
    if (!input) ERROR();
    PRINT("Semid: %d\nShmid: %d\n", semid, shmid);

    /* open input file */
    int inp_fd = open (input, O_RDONLY);
    if (inp_fd == -1) ERROR();
 
    /* get file size */
    struct stat inp_buf;
    stat (input, &inp_buf);
    long file_size = inp_buf.st_size;
 
    /* read from input file */
    if (read (inp_fd, shm_buf, file_size) <= 0) ERROR();
    close (inp_fd);
    
    PRINT("Shm_buf in writer: %s\n", shm_buf);
    
    /* change semaphore */
    fill_sb (sb, 0, 0, -1, SEM_UNDO);
    semop (semid, sb, 1);
    return 0;
}

int reader (int semid, int shmid, 
        char* shm_buf, struct sembuf* sb)
{
    /* write to stdout */
    if (write (STDOUT_FILENO, shm_buf, shmem_size) <= 0) ERROR();
    
    /* change semaphore */
    fill_sb (sb, 0, 0, 1, SEM_UNDO);
    semop (semid, sb, 1);
    return 0;
}

int main (int argc, char** argv)
{
    /* get common sources */
    key_t key = ftok ("sem.c", 0);
    int shmid = shmget (key, shmem_size, 0600 | IPC_CREAT);
    char* shm_buf = (char*) shmat (shmid, NULL, 0);
    int semid = semget (key, 1, 0600 | IPC_CREAT);
    struct sembuf sb [1] = {};

    if (argc == 1) 
        reader (semid, shmid, shm_buf, sb);
    else if (argc == 2) 
        writer (argv [1], semid, shmid, shm_buf, sb);
    else {
        perror ("Incorrect arguments!\n");
        exit   (EXIT_FAILURE);
    }

    shmdt  (shm_buf);
    shmctl (shmid, IPC_RMID, NULL);
    semctl (semid, 0, IPC_RMID);
    return 0;
}
