/*
 * Brian Chrzanowski
 * Tue Nov 19, 2019 20:12
 *
 * A test experiment in C. Mainly for use on Windows, where other programming
 * languages can be somewhat cludgy to use cleanly.
 *
 * TO COMPILE
 *   gcc -o test.exe test.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define C0 299792458
#define EI 4
#define C  (C0 / sqrt(EI))

typedef float f32;
typedef double f64;

f64 funcf(f64 dim[3], f64 curr[3]);
f64 funcg(f64 dim[3], f64 curr[3]);

int main(int argc, char **argv)
{
	f64 dim[3];
	f64 curr[3];
	f64 ans;
	long line;
	int rc, velocitymode;
	char buf[256];

	if (argc < 2) {
		fprintf(stderr, "USAGE : %s [--vel] [--amp]\n", argv[0]);
		return 1;
	}

	if (strcmp("--vel", argv[1]) == 0) {
		velocitymode = 1;
	} else if (strcmp("--amp", argv[1]) == 0) {
		velocitymode = 0;
	} else {
		fprintf(stderr, "Unrecognized argument [%s]\n", argv[1]);
		fprintf(stderr, "USAGE : %s [--vel] [--amp]\n", argv[0]);
		return 1;
	}

	for (line = 0; buf == fgets(buf, sizeof(buf), stdin); line++) {
		rc = sscanf(buf, "%lf\t%lf\t%lf\n", &curr[0], &curr[1], &curr[2]);

		if (rc != 3) {
			fprintf(stderr, "WE DIDN'T GET 3 ITEMS FROM THE INPUT!!!!\n");
			break;
		}

		if (line == 0) {
			memcpy(dim, curr, sizeof(dim));
		} else {
			if (velocitymode) {
				ans = funcg(dim, curr);
			} else {
				ans = funcf(dim, curr);
			}

			printf("%lf\n", ans);
		}
	}

	return 0;
}

f64 funcf(f64 dim[3], f64 curr[3])
{
	f64 xinit, yinit, zinit, interim;

	xinit = pow((2 * curr[0] / dim[0] - 1), 2);
	yinit = pow((2 * curr[1] / dim[1] - 1), 2);
	zinit = pow((2 * curr[2] / dim[2] - 1), 2);

	interim = exp(-13.0 * (xinit + yinit + zinit));

	return interim;
}

f64 funcg(f64 dim[3], f64 curr[3])
{
	f64 interim;

	interim = funcf(dim, curr);

	return C * 2 * 13 * 2 / dim[0] * (2 * curr[0] / dim[0] - 1) * interim;
}

