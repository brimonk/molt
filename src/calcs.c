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
#include <assert.h>
#include <math.h> 

#include "calcs.h"
#include "common.h"

#define MINVAL 0.0000000000005
#define WORKINGSTACKN 12

/* get_exp_weights : construct local weights for int up to order M */
void get_exp_weights(f64 nu, f64 *wl, f64 *wr, s32 nulen, s32 orderm)
{
	int i, j, k;
	int rowlen, reallen;
	double *x, *phi, *workvect_r, *workvect_l;
	double *workmat_r, *workmat_l;
	double tmpnu;

	rowlen = orderm + 1;
	reallen = nulen + 1;

	/* retrieve temporary working space from the operating system */
	x          = calloc(sizeof(double), reallen);
	phi        = calloc(sizeof(double), rowlen);
	workvect_r = calloc(sizeof(double), rowlen);
	workvect_l = calloc(sizeof(double), rowlen);
	workmat_r  = calloc(sizeof(double), rowlen * rowlen);
	workmat_l  = calloc(sizeof(double), rowlen * rowlen);

	if (!x || !phi || !workvect_r || !workvect_l || !workmat_r || !workmat_l) {
		PRINT_AND_DIE("Couldn't Get Enough Memory");
	}

	// fetch the cumulative sum of nu
	nu_cumsum(x, nu, reallen);

	for (i = 0; i < nulen; i++) {
		/* get the current coefficients */
		exp_coeff(phi, rowlen, nu);

		/* determine what indicies we need to operate over */
		j = get_exp_ind(i, nulen, orderm);

		/* fill our our Z vectors */
		for (k = 0; k < rowlen; k++) {
			workvect_r[k] = (x[j + k] - x[i    ]) / nu;
			workvect_l[k] = (x[i + 1] - x[j + k]) / nu;
		}

		invvan(workmat_l, workvect_l, rowlen);
		invvan(workmat_r, workvect_r, rowlen);

		matflip(workmat_l, rowlen);
		matflip(workmat_r, rowlen);

		/* multiply our phi vector with our working matrix, giving the answer */
		mat_mv_mult(wl + (i * rowlen), workmat_l, phi, rowlen);
		mat_mv_mult(wr + (i * rowlen), workmat_r, phi, rowlen);
	}

	free(x);
	free(phi);
	free(workvect_r);
	free(workvect_l);
	free(workmat_r);
	free(workmat_l);
}

/* matflip : our implementation of Matlab's flipud */
void matflip(double *mat, int n)
{
	/* flips mat about the horizontal axis (upside-down) */
	double *ptra, *ptrb;
	int i, j;

	for (i = 0; i < n / 2; i++) {
		ptra = &mat[i * n];
		ptrb = &mat[(n - 1 - i) * n];

		for (j = 0; j < n; j++, ptra++, ptrb++) {
			SWAP(*ptra, *ptrb, double);
		}
	}
}

/* get_exp_ind : get indexes of X for get_exp_weights */
int get_exp_ind(int i, int n, int m)
{
	/*
	 * there are "four" bounding points of this 1d situation.
	 *
	 *  0     - start
	 *  m / 2 - left stencil edge
	 *  n-m/2 - right stencil edge
	 *  n     - end
	 *
	 *  Please PLEASE be careful editing this.
	 */

	int farleft, farright, left, right;
	int rc;

	i++;

	farleft = 0;
	left = m / 2;
	right = n - m / 2;
	farright = n;

	if (farleft <= i && i < left)
		rc = 0;

	if (left <= i && i < right)
		rc = i - m / 2;

	if (right <= i && i <= farright)
		rc = n - m;

	return rc;
}

/* invvan : create an inverted Vandermonde matrix */
void invvan(double *mat, double *vect, int len)
{
	double workvect_a[WORKINGSTACKN], workvect_b[WORKINGSTACKN];
	double currpolyval;
	int i, j;

	assert(len < WORKINGSTACKN);

	/* clear off some space */
	memset(workvect_a, 0, sizeof(workvect_b));
	memset(workvect_b, 0, sizeof(workvect_b));
	memset(mat, 0, len * len * sizeof(*mat));

	/* get coefficients of p(x) = (x - Z1) ... (x - Zlen) */
	polyget(workvect_a, vect, len + 1, len);

	for (i = 0; i < len; i++) {
		polydiv(workvect_b, workvect_a, vect[i], len, len + 1);
		currpolyval = polyval(workvect_b, vect[i], len);

		for (j = 0; j < len; j++) { /* MATLAB : mat(:,i) = b / p */
			mat[j * len + i] = workvect_b[j] / currpolyval;
		}
	}
}

/* polyget : find coeff of a polyynomial with roots in src. store in dst */
void polyget(double *dst, double *src, int dstlen, int srclen)
{
	double workvect[WORKINGSTACKN];
	int i, j;

	assert(dstlen == srclen + 1);

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

/* polydiv : use synthetic division to find the coefficients the poly */
void polydiv(double *dst, double *src, double scale, int dstlen, int srclen)
{
	/* 
	 * Performs "b(x) = a(x) / (x - z)" in place
	 */

	int i;

	assert(dstlen == srclen - 1);

	memset(dst, 0, sizeof(double) * dstlen);
	dst[0] = src[0];

	for (i = 1; i < dstlen; i++) {
		dst[i] = src[i] + scale * dst[i - 1];
	}
}

/* polyval : evaluate a polynomial at x = z */
double polyval(double *vect, double scale, int vectlen)
{
	double retval;
	int i;

	for (retval = vect[0], i = 1; i < vectlen; i++) {
		retval = retval * scale + vect[i];
	}

	return retval;
}

/* nu_cumsum : perform a cumulative sum over elem along dimension dim */
void nu_cumsum(double *elem, double nu, int len)
{
	/* this function models octave behavior, not what it states it does IMO */
	int i;

	memset(elem, 0, sizeof(*elem) * len);

	for (i = 0; i < len; i++, nu += nu) {
		elem[i] += nu;
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

/* vec_add_s : add a scalar to a vector */
void vec_add_s(f64 *outvec, f64 *invec, f64 scalar, s32 len)
{
	s32 i;

	for (i = 0; i < len; i++) {
		outvec[i] = invec[i] + scalar;
	}
}

/* vec_sub_s : subtracts a scalar from an input vector */
void vec_sub_s(f64 *outvec, f64 *invec, f64 scalar, s32 len)
{
	s32 i;

	for (i = 0; i < len; i++) {
		outvec[i] = invec[i] - scalar;
	}
}

/* vec_add_v : adds two vector's elements together */
void vec_add_v(f64 *outvec, f64 *veca, f64 *vecb, s32 len)
{
	s32 i;

	for (i = 0; i < len; i++) {
		outvec[i] = veca[i] + vecb[i];
	}
}

/* vec_sub_v : subtracts two vector's elements from each other */
void vec_sub_v(f64 *outvec, f64 *veca, f64 *vecb, s32 len)
{
	s32 i;

	for (i = 0; i < len; i++) {
		outvec[i] = veca[i] - vecb[i];
	}
}

/* vec_mul_s : multiply a vector by a scalar */
void vec_mul_s(f64 *outvec, f64 *invec, f64 scalar, s32 len)
{
	s32 i;

	for (i = 0; i < len; i++) {
		outvec[i] = invec[i] * scalar;
	}
}

/* vec_negate : negates an entire vector */
void vec_negate(f64 *outvec, f64 *invec, s32 len)
{
	s32 i;

	for (i = 0; i < len; i++) {
		outvec[i] = -invec[i];
	}
}

/* mat_mv_mult : perform vector matrix multiply */
void mat_mv_mult(f64 *out, f64 *inmat, f64 *invec, s32 singledim)
{
	/* inmatn is the side length of our hopefully square matrix */
	int i, j;

	memset(out, 0, singledim * sizeof(double));

	for (i = 0; i < singledim; i++) {
		for (j = 0; j < singledim; j++) {
			out[i] = out[i] + invec[j] * inmat[j * singledim + i];
		}
	}
}

/* matprint : function to print out an NxN matrix */
void matprint(double *mat, int n)
{
	int i, j;

	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			printf("%.3e%c", mat[i * n + j], j == n-1 ?'\n':'\t');
		}
	}
}

