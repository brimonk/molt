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
 * 2. write flipud
 * 3. write vander generating func
 * 4. Vector * Matrix, left multiplication
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#include "calcs.h"
#include "common.h"

#define WORKINGSTACKSIZE  144

/* get_exp_weights : construct local weights for int up to order M */
void get_exp_weights(double *nu, double **wl, double **wr,
		int nulen, int orderm)
{
	int i, j, k;
	int rowlen, reallen;
	double *x, *phi, *workvect_r, *workvect_l;
	double *workmat_r, *workmat_l;

	rowlen = orderm + 1;
	reallen = nulen + 1;

	/* retrieve temporary working space from the operating system */
	x          = calloc(sizeof(double), reallen);
	phi        = calloc(sizeof(double), rowlen);
	workvect_r = calloc(sizeof(double), rowlen);
	workvect_l = calloc(sizeof(double), rowlen);
	workmat_r  = calloc(sizeof(double), rowlen * rowlen);
	workmat_l  = calloc(sizeof(double), rowlen * rowlen);

	/* WARNING : this memory is "returned" to the user */
	*wl = calloc(sizeof(double), nulen * rowlen);
	*wr = calloc(sizeof(double), nulen * rowlen);

	if (!x || !*wr || !*wl || !phi ||
			!workvect_r || !workvect_l || !workmat_r || !workmat_l) {
		PRINT_AND_DIE("Couldn't Get Enough Memory");
	}

	/* get the cumulative sum, store in X */
	memcpy(x + 1, nu, sizeof(double) * nulen);
	cumsum(x, reallen);

	for (i = 0; i < nulen; i++) {
		/* get the current coefficients */
		exp_coeff(phi, rowlen, nu[i]);

		/* determine what indicies we need to operate over */
		j = get_exp_ind(i, nulen, orderm);

		/* fill our our Z vectors */
		for (k = 0; k < rowlen; k++) { /* TODO (Brian): feels wonky */
			workvect_l[k] = (x[j + k] - x[i]) / nu[i];
			workvect_r[k] = (x[i + 1] - x[j + k]) / nu[i];
		}

		/* determine and find our vandermonde matrix */
		matvander(workmat_l, workvect_l, rowlen);
		matvander(workmat_r, workvect_r, rowlen);

		/* invert both of them */
		matinv(workmat_l, rowlen);
		matinv(workmat_r, rowlen);

		/* multiply our phi vector with our working matrix, giving the answer */
		vm_mult((*wl) + (i * rowlen), phi, workmat_l, rowlen);
		vm_mult((*wr) + (i * rowlen), phi, workmat_r, rowlen);
	}

	free(x);
	free(phi);
	free(workvect_r);
	free(workvect_l);
	free(workmat_r);
	free(workmat_l);
}

/* get_exp_ind : get indexes of X for get_exp_weights */
int get_exp_ind(int i, int n, int m)
{
	if (i <= m / 2) {
		return 0;
	} else if (n - m / 2 <= i) {
		return n - m;
	} else {
		return i + -m / 2;
	}
}

/* matvander : create an NxN vandermonde matrix, from vector vect */
void matvander(double *mat, double *vect, int n)
{
	int i, j, k;

	k = 0;

	for (i = n - 1; 0 <= i; i--) {
		for (j = 0; j < n; j++) {
			mat[j * n + i] = pow(vect[j], k);
		}
		k++;
	}
}

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

	if (12 < n) {
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

/* cumsum : perform a cumulative sum over elem along dimension dim */
void cumsum(double *elem, int len)
{
	/* this function models octave behavior, not what it states it does IMO */
	double curr;
	int i;

	for (i = 0, curr = 0.0; i < len; i++) {
		curr += elem[i];
		elem[i] = curr;
	}
}

/* exp_coeff : find the exponential coefficients, given nu and M */
void exp_coeff(double *phi, int philen, double nu)
{
	double d;
	int i, sizem;

	/*
	 * WARNING: the passed in philen NEEDS to be one greater than the initial
	 * M value, as this *also* operates in place.
	 */

	memset(phi, 0, sizeof(double) * philen);
	sizem = philen - 1; /* use sizem as an index */

	/* seed the value of phi, and work backwards */
	phi[sizem] = exp_int(nu, sizem);

	d = exp(-nu);

	for (i = sizem - 1; i >= 0; i--) {
		phi[i] = nu / (i + 1) * (phi[i + 1] + d);
	}
}

/* exp_int : perform an exponential integral (???) */
double exp_int(double nu, int sizem)
{
	double t, s, k, d, phi;

	d = exp(-nu);

	if (nu < 1) { /* avoid precision loss by using a Taylor Series */
		t = 1;
		s = 0;
		k = 1;

		while (t > DBL_EPSILON) {
			t = t * nu / (sizem + k);
			s = s + t;
			k++;
		}

		phi = d * s;

	} else {
		t = 1;
		s = 1;

		for (k = 0; k < sizem; k++) {
			t = t * nu / k;
			s = s + t;
		}

		phi = (1 - d * s) / t;
	}

	return phi;
}

/* vm_mult : perform vector matrix multiplication */
void vm_mult(double *out, double *invect, double *inmat, int singledim)
{
	/* inmatn is the side length of our hopefully square matrix */
	int i, j;

	memset(out, 0, singledim * sizeof(double));

	for (i = 0; i < singledim; i++) {
		for (j = 0; j < singledim; j++) {
			out[i] = out[i] + invect[j] * inmat[j * singledim + i];
		}
	}
}
 
