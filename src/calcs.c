/*
 * Brian Chrzanowski
 * Tue Jan 22, 2019 21:51
 *
 * a collection of fancy Matlab operations that aren't trivial for computers
 *
 * Includes:
 *   matinv   - Matrix Inversion (Gauss-Jordan Elim)
 *   matprint - Matrix Printing (Debugging Only)
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

/* matprint : function to print out an NxN matrix */
void matprint(double *mat, int n)
{
	int i, j;

	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			printf("%.3f%c", mat[i * n + j], j == n - 1 ? '\n' : '\t');
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
	 * This function gets cludgy for the personal requirement that it needs to
	 * appear to work in-place. That means, no extra memory can be sent in. No
	 * tricks of moving about the matrix from 0 -> 2 * n. Nothing like that.
	 * Therefore, any loop that requires to perform such iterations are broken
	 * up into two portions. One that works on the input 'mat', and one that
	 * works on the entirely internal 'stack'.
	 *
	 * The first section is the "left", pre-augmented side.
	 *
	 * The second section is the "right", post-augmented portion of the matrix.
	 *
	 * TL;DR:
	 *   Better interface for more writing.
	 *
	 * There be Dragons Here
	 */

	double stack[100], tmp; /* working space for at most a 10x10 matrix */
	int i, j, k;

	if (10 < n) {
		return 1;
	}

	memset(stack, 0, sizeof(double) * n * n);

	/* fill the tmp matrix with an identity */
	for (i = 0; i < n; i++) {
		stack[i * n + i] = 1;
	}

#if 0
	printf("MATRIX\n");
	matprint(mat, n);
#endif

	/* interchange step (TODO brian: check if this can actually be skipped) */
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

#if 0
	printf("Row Replace\n");
	matprint(mat, n);
	matprint(stack, n);
#endif


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

#if 0
	printf("INV\n");
	matprint(stack, n);
#endif

	memcpy(mat, stack, sizeof(double) * n * n);

	return 0;
}
 
