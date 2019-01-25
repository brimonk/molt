/*
 * Brian Chrzanowski
 * Sun Jan 20, 2019 14:43
 *
 * Method Of Lines Transpose - Implicit Wave Solver with Pluggable Modules
 *
 * TODO (Brian)
 *
 * 1. Implement Memory Mapped Storage
 * 2. Define a Binary File Format for aforementioned mmapped storage
 * 3. Implement GF_Quad and Get_Exp_Weights
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "io.h"
#include "structures.h"
#include "vect.h"
#include "calcs.h"

#define DEFAULTFILE "data.dat"

int main(int argc, char **argv)
{
	int fd, i, n;
	char *ptr;

	double mat[] =
	{
		4, 1, 8,
		3,-4, 3,
		1, 8, 9
	};

	fd = io_open(DEFAULTFILE);

	io_resize(fd, 1024 * 16);

	ptr = io_mmap(fd, 1024 * 16);
	memset(ptr, 0, 1024 * 16);

	io_munmap(ptr);

	/* demonstrate inverse matrix */
	n = sqrt(sizeof(mat) / sizeof(mat[0]));

	dprintf(fd, "Original Matrix\n");
	matprint(fd, mat, n);

	matinv(mat, n);

	dprintf(fd, "\nInverted Matrix\n");
	matprint(fd, mat, n);

	io_close(fd);

	return 0;
}

