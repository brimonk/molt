/*
 * Brian Chrzanowski
 * Tue Feb 19, 2019 10:27
 *
 * GFQuad, Sweeping, and Time Stepping Functions are all included in here
 */

#include <stdlib.h>
#include <float.h>

#include "common.h"
#include "shared.h"
#include "calcs.h"

static f64 minarr(f64 *arr, s32 arrlen);
void gfquad_makel(f64 *wout, f64 *nu,
		f64 *vl, f64 *vr, s32 vlen,
		f64 *wl, f64 *wr, s32 wlen,
		f64 wa, f64 wb, s32 orderm);

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
		f64 *arri, s32 ilen,
		f64 *arru, s32 ulen,
		f64 *nu, f64 *wl, f64 *wr, int orderm)
{
	f64 tmpi;
	int i;

	tmpi = 0;

	if (orderm < 4)
		orderm = 4;

	/* perform the left sweep */
	for (i = 0; i < ulen; i++) {
	}

	/* perform the right sweep */
	for (i = ilen - 1; i >= 0; i--) {
	}
}

/* minarr : finds the minimum f64 value in an array in linear time */
static
f64 minarr(f64 *arr, s32 arrlen)
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

