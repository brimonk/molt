/*
 * Brian Chrzanowski
 * Fri Feb 01, 2019 07:38
 *
 * Common Helpers
 */

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

void print_err_and_die(char *msg, char *file, int line)
{
	fprintf(stderr, "%s:%d %s\n", file, line, msg);
	exit(1);
}

/* swapd : swaps two doubles */
void swapd(double *a, double *b)
{
	double tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}
