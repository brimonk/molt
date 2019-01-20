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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "io.h"
#include "structures.h"
#include "vect.h"

#define DEFAULTFILE "data.dat"
#define DEFAULTFILEFLAGS 0664

int main(int argc, char **argv)
{
	int fd, i;
	char *ptr;

	fd = io_open(DEFAULTFILE, DEFAULTFILEFLAGS);

	io_resize(fd, 1024 * 16);

	ptr = io_mmap(fd, 1024 * 16);
	memset(ptr, 0, 1024 * 16);

	for (i = 'A'; i < 'Z' + 1; i++, ptr++) {
		*ptr = i;
	}

	io_munmap(ptr);
	io_close(fd);

	return 0;
}

