/*
 * Brian Chrzanowski
 * Tue Feb 19, 2019 10:27
 *
 * GFQuad, Sweeping, and Time Stepping Functions are all included in here
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

/* sweepx : perform a molt sweep in x */
void sweepx(f64 *uout, f64 *uin, f64 *nu, f64 *wl, f64 *wr, f64 *vl, f64 *vr,
		s64 udimx, s64 udimy, s64 udimz,
		s64 nulen, s64 wlen, s64 vlen,
		s64 orderm)
{
	s64 nx, ny, nz;
	f64 *slice;

	for (nz = 0; nz < udimz; nz++) {
		for (ny = 0; ny < udimy; ny++) {
			/* retrieve */
			slice = uin + IDX3D(0, ny, nz, udimy, udimz);
			gfquad_makel(slice, nu, vl, vr, vlen, wl, wr, wlen, 0, 0, orderm);
		}
	}
}

/* gfquad_makel : compute gfquad and operations for Dirchlet boundary conds */
void gfquad_makel(f64 *wout, f64 *nu,
		f64 *vl, f64 *vr, s32 vlen,
		f64 *wl, f64 *wr, s32 wlen,
		f64 wa, f64 wb, s32 orderm)
{
	/*
	 * gfquad_m call goes here
	 *
	 *
	 * w = w + ( (wa-w(1))*(vL-dN*vR) + (wb-w(end))*(vR-dN*vL) )/(1-dN^2);
	 */
}

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

