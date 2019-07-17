/*
 * Brian Chrzanowski
 * Fri Feb 01, 2019 07:38
 *
 * Common Helper Functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

void print_err_and_die(char *msg, char *file, int line)
{
	fprintf(stderr, "%s:%d %s\n", file, line, msg);
	exit(1);
}

// TODO (brian) rework the hunklogging to be more succinct

/* 
 * the hunklog_{1,2,3} functions generically exist to log out 1d, 2d, or 3d
 * data, given the double pointer to the "base" of the data, and the
 * dimensionality of the item being pointed to
 */

/* hunklog_1 : creates a readable log of the 1d data at p with dimensions dim */
s32 hunklog_1(char *file, int line, char *msg, ivec_t dim, f64 *p)
{
	/*
	 * returns the number of lines printed
	 * NOTE (brian) dim must be the number of ELEMENTS 
	 */

	s32 lines;
	s32 x, tmp;
	char fmt[BUFLARGE];

	// load up our printf format
	// (%xd) : %lf\n
	snprintf(fmt, sizeof fmt, "%d", dim);
	tmp = strlen(fmt);
	snprintf(fmt, sizeof fmt, "%s(%%%dd) : %s\n", msg, tmp, LOG_FLOATFMT);

	// print a preamble to stdout
	printf("%s:%d %s\n", file, line, msg);

	lines = 0;
	for (x = 0; x < dim; x++, lines++) {
		printf(fmt, x, p[x]);
	}

#ifdef LOG_NEWLINESEP
	printf("\n");
#endif

	return lines;
}

/* hunklog_2 : creates a readable log of the 2d data at p with dimensions dim */
s32 hunklog_2(char *file, int line, char *msg, ivec2_t dim, f64 *p)
{
	/*
	 * returns the number of lines printed
	 * NOTE (brian) dim must be the number of ELEMENTS 
	 */

	s32 lines;
	s32 x, y, i, tmp;
	char fmt[BUFLARGE];

	// find which dimension is bigger
	for (i = 0, tmp = 0; i < 2; i++) {
		if (tmp < dim[i])
			tmp = dim[i];
	}

	// load up our printf format
	// (%xd, %yd) : %lf\n
	snprintf(fmt, sizeof fmt, "%d", tmp);
	tmp = strlen(fmt);
	snprintf(fmt, sizeof fmt, "%s(%%%dd,%%%dd) : %s\n", msg, tmp, tmp, LOG_FLOATFMT);

	// print a preamble to stdout
	printf("%s:%d %s\n", file, line, msg);

	lines = 0;
	for (y = 0; y < dim[1]; y++) {
		for (x = 0; x < dim[0]; x++, lines++) {
			i = IDX2D(x, y, dim[1]);
			printf(fmt, x, y, p[i]);
		}
	}

#ifdef LOG_NEWLINESEP
	printf("\n");
#endif

	return lines;
}

/* hunklog_3 : creates a readable log of the 3d data at p with dimensions dim */
s32 hunklog_3(char *file, int line, char *msg, ivec3_t dim, f64 *p)
{
	/*
	 * returns the number of lines printed
	 * NOTE (brian) dim must be the number of ELEMENTS 
	 */

	s32 lines;
	s32 x, y, z, i, tmp;
	char fmt[BUFLARGE];

	// find which dimension is bigger
	for (i = 0, tmp = 0; i < 3; i++) {
		if (tmp < dim[i])
			tmp = dim[i];
	}

	// load up our printf format
	// (%xd, %yd) : %lf\n
	snprintf(fmt, sizeof fmt, "%d", tmp);
	tmp = strlen(fmt);
	snprintf(fmt, sizeof fmt, "%s(%%%dd,%%%dd,%%%dd) : %s\n",
			msg, tmp, tmp, tmp, LOG_FLOATFMT);

	// print a preamble to stdout
	printf("%s:%d %s\n", file, line, msg);

	lines = 0;
	for (z = 0; z < dim[2]; z++) {
		for (y = 0; y < dim[1]; y++) {
			for (x = 0; x < dim[0]; x++, lines++) {
				i = IDX3D(x, y, z, dim[1], dim[2]);
				printf(fmt, x, y, z, p[i]);
			}
		}
	}

#ifdef LOG_NEWLINESEP
	printf("\n");
#endif

	return lines;
}

