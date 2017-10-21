#ifndef SEM_H
#define SEM_H

#define NDEBUG

#ifdef DEBUG
#define PRINT(...) printf(__VA_ARGS__)
#else
#define PRINT(...)
#endif

#define PRINT_LINE() printf("LINE: %d\n", __LINE__);

#define ERROR() {                                   \
    printf ("ERROR!\n");                            \
    printf ("FUNCTION: %s\n", __PRETTY_FUNCTION__); \
    printf ("LINE: %d\n", __LINE__);                \
    exit   (EXIT_FAILURE);                          \
}

enum {
    MUT,     /* 0 (mutex)             */
    WFWR,    /* 1 (wait for writer)   */
    SEMNUM   /* 2 (semaphores amount) */
};

const size_t shmem_size = 16384;

void fill_sb (struct sembuf* sb, int sembuf_num, unsigned short num,
        short op, short flg);
void writer (char* input, int semid, int shmid, char* shm_buf, 
        struct sembuf* sb); 
void reader (int semid, int shmid, char* shm_buf, struct sembuf* sb);

#endif
