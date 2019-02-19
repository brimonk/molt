#ifndef MOLT_COMMON
#define MOLT_COMMON

/* macros to help index into our 2d and 3d arrays */
#define IDX2D(x, y, ylen)    ((x) + ((ylen) * (y)))
#define IDX3D(x, y, z, ylen, zlen) \
	((x) + ((y) * (ylen)) + ((z) * (ylen) * (zlen)))

void print_err_and_die(char *msg, char *file, int line);
void swapd(double *a, double *b);

#define PRINT_AND_DIE(x) (print_err_and_die((x), __FILE__, __LINE__))

#endif
