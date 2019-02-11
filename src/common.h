#ifndef MOLT_COMMON
#define MOLT_COMMON

void print_err_and_die(char *msg, char *file, int line);
void swapd(double *a, double *b);

#define PRINT_AND_DIE(x) (print_err_and_die((x), __FILE__, __LINE__))

#endif
