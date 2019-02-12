#ifndef MOLT_COMMON
#define MOLT_COMMON

/* macros to help index into our 2d and 3d arrays */
#define IDX_2D(x, y, len)    ((x) * (len) + (y))
#define IDX_3D(x, y, z, len) (((x) * (len) * (len)) + ((y) * (len)) + (z))

void print_err_and_die(char *msg, char *file, int line);
void swapd(double *a, double *b);

#define PRINT_AND_DIE(x) (print_err_and_die((x), __FILE__, __LINE__))

#endif
