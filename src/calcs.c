/*
 * Brian Chrzanowski
 * Tue Jan 22, 2019 21:51
 *
 * A collection of fancy Matlab operations that aren't trivial for computers.
 *
 * TODO
 * 1. Complete Get_Exp_Weights conversion
 * 2. Prefix functions required to do that
 * 3. Declare functions as static where not needed in public interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#include "calcs.h"
#include "common.h"

// #define WORKINGSTACKSIZE  144
#define MINVAL 0.0000000000005
#define WORKINGSTACKN 12

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
			workvect_r[k] = (x[j + k] - x[i]) / nu[i];
			workvect_l[k] = (x[i + 1] - x[j + k]) / nu[i];
		}

		invvan(workmat_l, workvect_l, rowlen);
		invvan(workmat_r, workvect_r, rowlen);

		// matflip(workmat_l, rowlen);
		// matflip(workmat_r, rowlen);

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

/* invvan : create an inverted vandermonde matrix */
void invvan(double *mat, double *vect, int len)
{
	double workvect[WORKINGSTACKN];
	int i, j;

	/* clear off some space */
	memset(workvect, 0, WORKINGSTACKN * sizeof(double));
	memset(mat, 0, len * len * sizeof(double));

	for (i = 0; i < len; i++) {
	}
}

/* polyget : find coeff of a polyynomial with roots in src. store in dst */
void polyget(double *dst, double *src, int dstlen, int srclen)
{
	double workvect[WORKINGSTACKN];
	int i, j;

	if (dstlen != srclen + 1) {
		return; /* should probably assert error */
	}

	memset(workvect, 0, sizeof(workvect));
	memset(dst,      0, sizeof(double) * dstlen);

	dst[0] = 1; /* leading coefficient is assumed as 1 */

	for (i = 0; i < srclen; i++) {
		memcpy(workvect, dst, dstlen * sizeof(double));

		for (j = 1; j <= i + 1; j++) {
			dst[j] = workvect[j] - src[i] * workvect[j - 1];
		}
	}
}

void polydiv()
{
}

void polyval()
{
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
 
