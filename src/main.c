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
#define DBL_MACRO_SIZE(x) sizeof(x) / sizeof(double)

int main(int argc, char **argv)
{
	int fd, i, n;
	char *ptr;

	double initset[] = { -2, -1, 0, 1, 2 };
	double vanderwork[512];

	double mat[] =
	{
		1, 2, 3, 4,
		5, 6, 7, 8,
		9,10,11,12,
	   13,14,15,16
	};
	double vector[] =
	{
		1, 2, 3, 4
	};

	double space[DBL_MACRO_SIZE(vector)];

	memset(vanderwork, 0, 512 * sizeof(double));
	memcpy(vanderwork, mat, sizeof(mat));

	fd = io_open(DEFAULTFILE);

	io_resize(fd, 1024 * 16);

	ptr = io_mmap(fd, 1024 * 16);
	memset(ptr, 0, 1024 * 16);

	io_lumpcheck(ptr);
	io_lumpsetup(ptr);

	/* try it again */
	struct lump_pos_t *pos;

	pos = io_lumpgetid(ptr, MOLTLUMP_POSITIONS);

#if 0 /* test lumrecord IO */
	for (i = 0; i < io_lumprecnum(ptr, MOLTLUMP_POSITIONS); i++) {
		printf("%d\t%d\t%d\n", pos[i].x, pos[i].y, pos[i].z);
	}
#endif

#if 0 /* test matvander */
	printf("{");
	for (i = 0; i < 5; i++) {
		printf("%lf%s", initset[i], i == 4 ? "}\n" : ", ");
	}

	matvander(mat, initset, 5);
	matprint(mat, 5);
#endif

#if 0 /* test exp_coeff */
	exp_coeff(mat, 7, 1.6469E-08);
	for (i = 0; i < 7; i++) {
		printf("%.10e\n", mat[i]);
	}
#endif

#if 0 /* test vm_mult */
	memset(space, 0, sizeof(space));

	vm_mult(space, vector, mat,
			DBL_MACRO_SIZE(vector),
			DBL_MACRO_SIZE(vector),
			sqrt(DBL_MACRO_SIZE(mat)));

	for (i = 0; i < DBL_MACRO_SIZE(space); i++) {
		printf("%.10e\n", space[i]);
	}

#endif

	io_munmap(ptr);
	io_close(fd);

	return 0;
}

