/*
 * Brian Chrzanowski
 * 2020-07-20 20:21:18
 *
 * MOLT Function Tests
 */

#define COMMON_IMPLEMENTATION
#include "common.h"

#include <math.h>

#define MOLT_IMPLEMENTATION
#include "molt.h"

#define REORG_TESTS (100)

/* test_molt_reorg : tests molt reorg */
int test_molt_reorg(void);

int main(int argc, char **argv)
{
	if (!test_molt_reorg()) {
		printf("test_molt_reorg() failed!\n");
	}

	return 0;
}

/* test_molt_reorg : tests molt reorg */
int test_molt_reorg(void)
{
	f64 *a, *b, *c, *w;
	u64 v;
	ivec3_t dim;
	ivec3_t idx;
	s64 elem;
	int i, j, rc;

	rc = 0;

	for (i = 1; i < REORG_TESTS; i++) {

		printf("%s - %d\n", __FUNCTION__, i);

		dim[0] = i;
		dim[1] = i;
		dim[2] = i;

		elem = dim[0] * dim[1] * dim[2];

		a = calloc(sizeof(*a), elem);
		b = calloc(sizeof(*b), elem);
		c = calloc(sizeof(*c), elem);
		w = calloc(sizeof(*w), elem);

		assert(a && b && c && w);

		// initialize the 3d array
		for (idx[2] = 0; idx[2] < i; idx[2]++)
		for (idx[1] = 0; idx[1] < i; idx[1]++)
		for (idx[0] = 0; idx[0] < i; idx[0]++) {
			v = molt_genericidx(idx, dim, molt_ord_xyz);
			a[v] = (idx[0] * 0.01) + (idx[1] * 0.1) + idx[2];
		}

		// perform the swap
		molt_reorg(b, a, w, dim, molt_ord_xyz, molt_ord_yxz);
		molt_reorg(c, b, w, dim, molt_ord_yxz, molt_ord_xyz);

		// check for equality
		if (memcmp(a, c, sizeof(*c) * elem)) {
			printf("%s failed on i = %d\n", __FUNCTION__, i);

			for (idx[2] = 0; idx[2] < i; idx[2]++)
			for (idx[1] = 0; idx[1] < i; idx[1]++)
			for (idx[0] = 0; idx[0] < i; idx[0]++) {
				v = molt_genericidx(idx, dim, molt_ord_xyz);
				printf("[%lld] A %lf C %lf\n", v, a[v], c[v]);
			}
		} else {
			rc++;
		}

		free(a);
		free(b);
		free(c);
		free(w);
	}

	return rc == i - 1;
}

