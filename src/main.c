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
#include "calcs.h"
#include "shared.h"

#define DEFAULTFILE "data.dat"

int main(int argc, char **argv)
{
	int fd, i, n;
	char *ptr;

	double initset[] = {-2, -1, 0, 1, 2};
	double mat[512];

	memset(mat, 0, sizeof(double) * 512);

	fd = io_open(DEFAULTFILE);

	io_resize(fd, 1024 * 16);

	ptr = io_mmap(fd, 1024 * 16);
	memset(ptr, 0, 1024 * 16);

	io_lumpcheck(ptr);
	io_lumpsetup(ptr);

	/* try it again */
	struct lump_pos_t *pos;

	pos = io_lumpgetid(ptr, MOLTLUMP_POSITIONS);

	printf("{");
	for (i = 0; i < 3; i++) {
		printf("%lf%s", initset[i], i == 2 ? "}\n" : ", ");
	}

	matvander(mat, initset, 5);
	matprint(mat, 5);

#if 0
	for (i = 0; i < io_lumprecnum(ptr, MOLTLUMP_POSITIONS); i++) {
		printf("%d\t%d\t%d\n", pos[i].x, pos[i].y, pos[i].z);
	}
#endif

	io_munmap(ptr);
	io_close(fd);

	return 0;
}

