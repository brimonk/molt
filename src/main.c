/*
 * Brian Chrzanowski
 * Sun Jan 20, 2019 14:43
 *
 * Method Of Lines Transpose - Implicit Wave Solver with Pluggable Modules
 *
 * TODO (Brian)
 * 1. Simulation Setup With Hunks
 * 2. Function Handles (f, g)
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
#include "calcs.h"
#include "config.h"
#include "gfquad.h"

#define DEFAULTFILE "data.dat"
#define DBL_MACRO_SIZE(x) sizeof(x) / sizeof(double)

void setup_simulation(void *base);
void applied_funcf(void *base);
void applied_funcg(void *base);

int main(int argc, char **argv)
{
	void *hunk;
	int fd;

	fd = io_open(DEFAULTFILE);

	io_resize(fd, 1024 * 16);

	hunk = io_mmap(fd, 1024 * 16);
	memset(hunk, 0, 1024 * 16);

	setup_simulation(hunk);

	io_masync(hunk, hunk, 1024 * 16);

	io_munmap(hunk);
	io_close(fd);

	return 0;
}

/* setup_simulation : sets up simulation based on config.h */
void setup_simulation(void *base)
{
	// ensure we have a valid memory mapped file
	if (io_lumpcheck(base)) {
		io_fprintf(stdout, "Setting Up New File %s\n", io_getfilename());
	}
}

/* applied_func : applies function "f" from the original source to data */
void applied_funcf(void *base)
{
}

/* applied_func : applies function "g" from the original source to data */
void applied_funcg(void *base)
{
}

