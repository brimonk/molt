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
#include "config.h"

#define DEFAULTFILE "data.dat"
#define DBL_MACRO_SIZE(x) sizeof(x) / sizeof(double)

int main(int argc, char **argv)
{
	int fd, i, j, n;
	char *ptr;

	double nu[MOLT_TOTALWIDTH];
	double x[MOLT_TOTALWIDTH + 1];
	double *wl, *wr;

	for (i = 0; i < MOLT_TOTALWIDTH + 1; i++) {
		x[i] = i * MOLT_STEPINX;
		// printf("%d\t%lf\n", i, x[i]);
	}

	for (i = 0; i < MOLT_TOTALWIDTH; i++) {
		nu[i] = MOLT_ALPHA * (x[i + 1] - x[i]);
		// printf("%d\t%.8E\n", i, nu[i]);
	}

	get_exp_weights(nu, &wl, &wr, MOLT_TOTALWIDTH, MOLT_SPACEACC);

	printf("WL Results\n");
	for (i = 0; i < MOLT_TOTALWIDTH + 1; i++) {
		for (j = 0; j < MOLT_SPACEACC + 1; j++)
			printf("%.3e\t", wl[i * (MOLT_SPACEACC + 1) + j]);
		printf("\n");
	}

#if 0
	printf("WR Results\n");
	for (i = 0; i < MOLT_TOTALWIDTH; i++) {
		for (j = 0; j < MOLT_SPACEACC; j++)
			printf("%.3e\t", wr[i]);
		printf("\n");
	}
#endif

#if 0
	fd = io_open(DEFAULTFILE);

	io_resize(fd, 1024 * 16);

	ptr = io_mmap(fd, 1024 * 16);
	memset(ptr, 0, 1024 * 16);

	io_lumpcheck(ptr);
	io_lumpsetup(ptr);

	/* try it again */
	struct lump_pos_t *pos;

	pos = io_lumpgetid(ptr, MOLTLUMP_POSITIONS);

	io_munmap(ptr);
	io_close(fd);
#endif

	free(wr);
	free(wl);

	return 0;
}


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
