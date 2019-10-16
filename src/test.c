/*
 * Brian Chrzanowski
 * Wed Oct 16, 2019 09:31
 *
 * Implementation Testing Functions
 *
 * This isn't _really_ a unit testing sort of a thing. It spits out data
 * (stdout) for a person verifying the program.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "molt.h"
#include "common.h"

/* test_reorg : tests molt_reorg */
void test_reorg(ivec3_t dim)
{
	f64 *a, *b, *c;
	s64 len;
	s32 i, j;
	cvec3_t src_ord, dst_ord;
	char srcmsg[32];
	char dstmsg[32];

	cvec3_t original = { 'x', 'y', 'z' };

	cvec3_t organizations[6] = {
		{'x', 'y', 'z'},
		{'x', 'z', 'y'},
		{'y', 'x', 'z'},
		{'y', 'z', 'x'},
		{'z', 'x', 'y'},
		{'z', 'y', 'x'},
	};

	len = (s64)dim[0] * dim[1] * dim[2];

	a = calloc(len, sizeof(f64));
	b = calloc(len, sizeof(f64));
	c = calloc(len, sizeof(f64));

	printf("testing :: molt_reorg\n");

	for (i = 0; i < len; i++) {
		for (j = 0; j < len; j++) {
			a[j] = j;
		}

		Vec3Copy(src_ord, original);
		Vec3Copy(dst_ord, organizations[i]);

		molt_reorg(c, a, b, dim, src_ord, dst_ord);

		snprintf(srcmsg, sizeof srcmsg, "src iter %d, %.*s", i, 3, (char *)src_ord);
		snprintf(dstmsg, sizeof dstmsg, "dst iter %d, %.*s", i, 3, (char *)dst_ord);

		LOG3D(a, dim, srcmsg);
		LOG3D(c, dim, dstmsg);
	}

	free(a);
	free(b);
	free(c);
}

void testfunc()
{
	// NOTE (brian) tests our molt functions
	ivec3_t dim;

	Vec3Set(dim, 3, 3, 3);

	test_reorg(dim);
}

