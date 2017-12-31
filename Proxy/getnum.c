#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <stdlib.h>

long get_number (char* arg)
{
    char* endptr;
    long num = strtol (arg, &endptr, 10);

    if (endptr == arg) {
        perror ("You did not entered a number!\n");
        exit (EXIT_FAILURE);
    }

    if (num < 1) {
        perror ("Wrong number!\n");
        exit (EXIT_FAILURE);
    }

    if ((errno == ERANGE && (num == LONG_MAX || num == LONG_MIN))
            || (errno != 0 && num == 0)) {
        perror ("Too big number!\n");
        exit (EXIT_FAILURE);
    }

    if (*endptr != '\0') {
        printf ("Wrong number!\n");
        exit (EXIT_FAILURE);
    }

    return num;
}
