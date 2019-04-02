/*
 * Brian Chrzanowski
 * Tue Feb 19, 2019 10:27
 *
 * GFQuad, Sweeping, and Time Stepping Functions are all included in here
 *
 * Note about Memory Layout
 *
 * It is consistently assumed that the dimensionality of multi-dimensional
 * arrays when the data comes in is row ordered by X, then Y, then Z. This means
 * that if we want to index into 5, 2, 3, to resolve the 1D index, we must
 * perform: ((x) + ((y) * (ylen)) + ((z) * (ylen) * (zlen))) => 1D Index
 *
 * This isn't an issue when we operate over the X's of our data, meaning that we
 * only ever need to retrieve the data by the rows of Xs. However, we maximize
 * cache misses when we want to operate over a row in Y or a row in Z.
 *
 * The algorithm has to perform sweeps along X, Y, and Z to fully compute the
 * dot products in those dimensions, and apply weights to them. The only
 * difference between the sweeps is which slice of the 3d data you retrieve to
 * apply the weights. Effectively, the algorithm is the same, you just have to
 * tell do_sweep how long your row-based dimension is. This allows us to have
 * the luxury to compute the geometry in any X, Y, and Z direction, while not
 * writing specific code for the sweeping function.
 *
 * TODO (Brian)
 * 1. GFQUAD operating over rows
 * 2. write do_line function, with a do_lines function overhead
 * 3. write reorg function to reorganize the 3d data into a different row-major
 *    order
 */

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <assert.h>

#include "common.h"
#include "calcs.h"

static f64 minarr(f64 *arr, s32 arrlen);
void gfquad_makel(f64 *wout, f64 *nu,
		f64 *vl, f64 *vr, s32 vlen,
		f64 *wl, f64 *wr, s32 wlen,
		f64 wa, f64 wb, s32 orderm);
static f64 vect_mul(f64 *veca, f64 *vecb, s32 veclen);
static f64 minarr(f64 *arr, s32 arrlen);

/* time_step : performs a single timestep */
void time_step(void *base, int timeidx)
{
	/* first retrieves our required hunk pointers from the base */
}

#if 0
/* sweepx : perform a sweep to compute weights */
void do_sweep(f64 *uout, f64 *uin, f64 *nu, f64 *wl, f64 *wr, f64 *vl, f64 *vr,
		s64 udimx, s64 udimy, s64 udimz,
		s64 nulen, s64 wlen, s64 vlen,
		s64 orderm)
{
	s64 i, j;
	f64 *slice;

	for (i = 0; i < dima; i++) {
		for (j = 0; j < dimb; j++) {
		}
	}
}
#endif

/* gfquad_m : does the GF Quad magic */
void gfquad_m(
		f64 *out, s32 outlen,
		f64 *in, s32 inlen,
		s32 orderm,
		f64 *nu, s32 nulen,
		pvec2_t weights, s32 w_x, s32 w_y)
{
	f64 *d;
	f64 IL, IR, M2;
	s32 iL, iR, iC;
	s32 i, internal_len;

	assert(16 > orderm + 1); // catch incorrect use
	internal_len = orderm + 1;

	/* setup */
	d = malloc(sizeof(*d) * nulen);

	for (i = 0; i < nulen; i++) {
		d[i] = exp(-nu[i]);
	}

	if (orderm < 4) { orderm = 4; }
	IL = 0;
	IR = 0;
	M2 = orderm / 2;

	iL = 0;
	iC = -M2;
	iR = inlen - internal_len;

	/* left sweep */
	for (i = 0; i < M2; i++) {
		IL = d[i] * IL + vect_mul(&weights[0][i * w_x], &in[iL], internal_len);
		out[i + 1] = out[i + 1] + IL;
	}

	for (; i < inlen - M2; i++) {
		IL = d[i] * IL + vect_mul(&weights[0][i * w_x], &in[i + iC], internal_len);
		out[i + 1] = out[i + 1] + IL;
	}

	for (; i < inlen; i++) {
		IL = d[i] * IL + vect_mul(&weights[0][i * w_x], &in[iR], internal_len);
		out[i + 1] = out[i + 1] + IL;
	}

	/* right sweep */
	for (i = inlen - 1; i > inlen - 1 - M2; i--) {
		IR = d[i] * IR + vect_mul(&weights[1][i * w_x], &in[iR], internal_len);
		out[i] = out[i] + IR;
	}

	for (; i > M2; i--) {
		IR = d[i] * IR + vect_mul(&weights[1][i * w_x], &in[i + iC], internal_len);
		out[i] = out[i] + IR;
	}

	for (; i >= 0; i--) {
		IR = d[i] * IR + vect_mul(&weights[1][i * w_x], &in[iL], internal_len);
		out[i] = out[i] + IR;
	}

	/* cleanup */
	free(d);
}

/* vect_mul : perform element-wise vector multiplication */
static f64 vect_mul(f64 *veca, f64 *vecb, s32 veclen)
{
	f64 val;
	s32 i;

	for (val = 0, i = 0; i < veclen; i++) {
		val += veca[i] * vecb[i];
	}

	return val;
}

/* minarr : finds the minimum f64 value in an array in linear time */
static f64 minarr(f64 *arr, s32 arrlen)
{
	f64 retval;
	s32 i;

	retval = DBL_MAX;

	for (i = 0; i < arrlen; i++) {
		if (arr[i] < retval)
			retval = arr[i];
	}

	return retval;
}

