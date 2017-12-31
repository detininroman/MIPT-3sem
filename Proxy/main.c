#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>

#define $(CMD)                                      \
{                                                   \
    errno = 0;                                      \
    CMD;                                            \
    if (errno) {                                    \
        fprintf (stderr, "Line %d: %s\n" #CMD "\n", \
                __LINE__, strerror(errno));         \
        exit (EXIT_FAILURE);                        \
    }                                               \
}

struct data 
{
    int up[2]; 
    int down[2]; 
    char* buffer;
    char* start;
    size_t size;
    size_t cursize;
};

enum buf_const
{
    CHILD_BUF_SIZE    = 1 << 17, // 128 kB
    PARENT_BUF_FACTOR = 1 << 9   // 512 B
};

enum pipefds
{
    RD = 0,
    WR = 1
};

struct data* ch_node = NULL;
int N = 2; // input & stdout

long get_number (char* arg);
void init       (int input);
void parent     ();
void child      (int cur);
void do_read    (int cur);
void do_write   (int cur);
int power       (int x, int y);
int max         (int x, int y);

int main (int argc, char** argv)
{
    if (argc != 3)
    {
       fprintf (stderr, "Error: %s\n", strerror (EINVAL));
       exit (EXIT_FAILURE);
    }
    
    N += (int) get_number (argv[2]);
   
    int input = -1;
    $(input = open (argv[1], O_RDONLY));

    init (input);

    for (int i = 1; i < N - 1; i++)
    {
        if (!fork())
            child (i);
    }

    parent();
    return 0;
}

void init (int input)
{
    $(ch_node = calloc (N, sizeof(*ch_node)));

    for (int i = 1; i < N - 1; i++)
    {
        $(pipe (ch_node[i].down));
        $(pipe (ch_node[i].up));
        $(fcntl(ch_node[i].down[WR], F_SETFL, O_WRONLY|O_NONBLOCK));
        $(fcntl(ch_node[i].up[RD], F_SETFL, O_WRONLY|O_NONBLOCK));
    }

    ch_node[0].up[RD] = input;
    ch_node[0].up[WR] = -1;    
    ch_node[0].down[RD] = -1;
    ch_node[0].down[WR] = -1;
   
    ch_node[N-1].up[RD] = -1;
    ch_node[N-1].up[WR] = -1;
    ch_node[N-1].down[RD] = -1;
    ch_node[N-1].down[WR] = STDOUT_FILENO;
}

void parent ()
{
    for (int i = 1; i < N - 1; i++)
    {
        $(close (ch_node[i].down[RD]));
        $(close (ch_node[i].up[WR]));
    }
    
    for (int i = 1; i < N; i++)
    {
        ch_node[i].size = power (3, i) * PARENT_BUF_FACTOR; 
        ch_node[i].cursize = 0;
        $(ch_node[i].start = 
            ch_node[i].buffer = 
                calloc (ch_node[i].size, sizeof(char)));
    }
    
    int nfds = 1;
    while (1)
    {
        fd_set readfds, writefds;
        $(FD_ZERO (&readfds));
        $(FD_ZERO (&writefds));
        int fd_cnt = 0;
        for (int i = 0; i < N; i++)
        {
            nfds = max (nfds, ch_node[i].up[RD]);
            nfds = max (nfds, ch_node[i].down[WR]);
           
            if ((ch_node[i].up[RD] != -1) &&
                    (ch_node[i + 1].cursize == 0))
            {
                FD_SET (ch_node[i].up[RD], &readfds);
                fd_cnt++;
            }
           
            if ((ch_node[i].down[WR] != -1) &&
                    (ch_node[i].cursize > 0))
            {
                FD_SET (ch_node[i].down[WR], &writefds);
                fd_cnt++;
            }
        } 
 
        if (!fd_cnt) break;

        $(select (nfds + 1, &readfds, &writefds, NULL, NULL));
        
        for (int i = 0; i < N; i++)
        {
            if (FD_ISSET (ch_node[i].up[RD], &readfds)) 
                do_read (i);
            if (FD_ISSET (ch_node[i].down[WR], &writefds)) 
                do_write (i);
        }
    }

    for (int i = 1; i < N; i++)
        free (ch_node[i].buffer);
    free (ch_node);
    exit (EXIT_SUCCESS);
}

void do_read (int cur)
{
    ssize_t rbytes;
    $(rbytes = read (ch_node[cur].up[RD], 
            ch_node[cur + 1].buffer, ch_node[cur + 1].size));
    ch_node[cur + 1].cursize += rbytes;
    if (!rbytes)
    {
        //printf ("1\n");
        $(close (ch_node[cur].up[RD]));
        $(close (ch_node[cur + 1].down[WR]));
        ch_node[cur].up[RD] = -1; 
        ch_node[cur + 1].down[WR] = -1;
    }
}

void do_write (int cur)
{
    ssize_t wbytes;
    $(wbytes = write (ch_node[cur].down[WR], 
                ch_node[cur].start, ch_node[cur].cursize));
    if (wbytes < ch_node[cur].cursize) 
        ch_node[cur].start += wbytes;
    else 
        ch_node[cur].start = ch_node[cur].buffer;
    ch_node[cur].cursize -= wbytes;
}

void child (int cur)
{
    for (int j = 1; j < N - 1; j++)
    {
        $(close (ch_node[j].down[WR]));
        $(close (ch_node[j].up[RD]));

        if (j != cur)
        {
            $(close (ch_node[j].down[RD]));
            $(close (ch_node[j].up[WR]));
        } 
    }
    
    int in  = ch_node[cur].down[RD];
    int out = ch_node[cur].up[WR];

    char buffer[CHILD_BUF_SIZE] = {};
    ssize_t rbytes = 1;
    while (rbytes)
    {
        $(rbytes = read (in, buffer, CHILD_BUF_SIZE));
        $(write (out, buffer, rbytes));
    }

    $(close (out));
    $(close (in));
    free (ch_node);
    exit (EXIT_SUCCESS);
}

int power (int x, int y)
{
    int res = 1;
    for (int i = 0; i < y; i++)
        res *= x;
    return res;
}

int max (int x, int y)
{
    return (x > y) ? x : y;
}
