/*
 * Brian Chrzanowski
 * Tue Jan 22, 2019 21:51
 *
 * a collection of fancy Matlab operations that aren't trivial for computers
 *
 * Includes:
 *   matinv   - Matrix Inversion (Gauss-Jordan Elim)
 *   matprint - Matrix Printing (Debugging Only)
 *
 * TODO
 * 1. test matinv
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#define WORKINGSTACKSIZE 144

/* matprint : function to print out an NxN matrix */
void matprint(double *mat, int n)
{
	int i, j;

	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			printf("%.3f%c", mat[i * n + j], j == n-1 ?'\n':'\t');
		}
	}
}

/* matinv : given an n x n matrix, perform Gauss-Jordan elimination, in place */
int matinv(double *mat, int n)
{
	/*
	 * Attempt to perform Gauss-Jordan Elimination to find a matrix's inversion,
	 * "in-place"
	 *
	 * This function gets cludgy for the requirement that it needs to appear to
	 * work in-place. To avoid passing in working space memory, some working
	 * space is created on the stack and used. This avoids operating system
	 * calls with the downside being it's only suited for a
	 * sqrt(WORKINGSTACKSIZE)^2 matrix.
	 *
	 * In every place where the algorithm would wipe from the "left" column
	 * to the "right" column, the algorithm is busted up into two segments, one
	 * iteration for the input "left" matrix, and one for the working space,
	 * "right".
	 *
	 * TL;DR: Better, faster interface for typing.
	 *
	 * There be Dragons Here
	 */

	double stack[WORKINGSTACKSIZE], tmp; /* working space at most a 12x12 mat */
	int i, j, k;

	if (10 < n) {
		return 1;
	}

	memset(stack, 0, sizeof(double) * n * n);

	/* fill the tmp matrix with an identity */
	for (i = 0; i < n; i++) {
		stack[i * n + i] = 1;
	}

	/* row interchange step */
	for (i = n - 1; i > 0; i--) {
		if (mat[i - 1] < mat[i]) {
			for (j = 0; j < n; j++) { /* left */
				/* swap 'em */
				tmp = mat[i * n + j];
				mat[i * n + j] = mat[(i - 1) * n + j];
				mat[(i - 1) * n + j] = tmp;
			}

			for (j = 0; j < n; j++) { /* right */
				tmp = stack[i * n + j];
				stack[i * n + j] = stack[(i - 1) * n + j];
				stack[(i - 1) * n + j] = tmp;
			}
		}
	}

	/* replace a row by sum of itself and const mul of another matrix row */
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) { /* left */
			if (j != i) {
				tmp = mat[j * n + i] / mat[i * n + i];
				for (k = 0; k < n; k++) { /* left */
					mat[j * n + k] -= mat[i * n + k] * tmp;
					// mat[j * n + k] -= mat[i * k] * tmp;
				}

				for (k = 0; k < n; k++) { /* right */
					stack[j * n + k] -= stack[i * n + k] * tmp;
				}
			}
		}

		for (j = 0; j < n; j++) { /* right */
			if (j != i) {
				tmp = mat[j * n + i] / mat[i * n + i];
				for (k = 0; k < n; k++) { /* left */
					stack[j * n + k] -= mat[i * n + k] * tmp;
					// mat[j * n + k] -= mat[i * k] * tmp;
				}

				for (k = 0; k < n; k++) { /* right */
					stack[j * n + k] -= mat[i * n + k] * tmp;
				}
			}
		}
	}

	/* 
	 * multiply each row by a nonzero integer, then divide row elements by the
	 * diagonal element
	 */
	for (i = 0; i < n; i++) {
		tmp = mat[i * n + i];

		for (j = 0; j < n; j++) { /* left */
			mat[i * n + j] = mat[i * n + j] / tmp;
		}

		for (j = 0; j < n; j++) { /* right */
			stack[i * n + j] = stack[i * n + j] / tmp;
		}
	}

	memcpy(mat, stack, sizeof(double) * n * n);

	return 0;
}
 
