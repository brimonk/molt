/*
 * Brian Chrzanowski
 * Sat Jul 28, 2018 03:03
 *
 * vert.c
 *   - A small vertex library, inspired by the Quake functions of similar use.
 */

#include <string.h>

void vect_init(void *ptr, int n)
{
	/* initializes a vector of size 'n' with zeroes */
	memset(ptr, 0, sizeof(double) * n);
}

