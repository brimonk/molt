#ifndef GFQUAD
#define GFQUAD

#include "common.h"

/* gfquad_m : does the GF Quad magic */
void gfquad_m(
		f64 *out, s32 outlen,
		f64 *in, s32 inlen,
		s32 orderm,
		f64 *nu, s32 nulen,
		pvec2_t weights, s32 w_x, s32 w_y);

#endif
