/*
 * Brian Chrzanowski
 * Fri Feb 01, 2019 07:38
 *
 * Common Helpers
 */

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

void print_err_and_die(char *msg, char *file, int line)
{
	fprintf(stderr, "%s:%d %s\n", file, line, msg);
	exit(1);
}
