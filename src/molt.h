#ifndef MOLT_H
#define MOLT_H

#include "common.h"

/*
 * Brian Chrzanowski
 * Sat Apr 06, 2019 22:21
 *
 * TODO Create a Single-Header file Lib (Brian)
 * 1. Dimensionality should be u64 vec3_t, lvec3_t
 * 2. Possibly reorder the parameters in molt_cfg_parampull_gen
 *    Also might want to change the name, as it requires an order...
 * 3. Define DBL_MAX and friends without the C standard library (?)
 * 4. Change the vectored datatypes to be "mo_*" or "molt_*"
 *    The latter has a lot of typing.
 * 5. Define enums or macros for param indicies
 *
 *    Maybe we could just have #defines setup to turn on or off parts of
 *    the library? Dunno.
 *
 * 5. Include file-configurable function qualifier (static, static inline, etc)
 *
 * 6. Clean up the other routine calling conventions. (Oops)
 *
 * TODO Future
 * ?. Consider putting in the alpha calculation
 * ?. Debug/Test
 * ?. Remove use CSTDLIB math.h? stdio.h? stdlib.h?
 *
 * TODO Even Higher Amounts of Performance (Brian)
 * 1. In Place Mesh Reorganize (HARD)
 */

enum {
	MOLT_PARAM_START,
	MOLT_PARAM_STOP,
	MOLT_PARAM_STEP,
	MOLT_PARAM_POINTS, // zero based
	MOLT_PARAM_PINC // actual total
};

enum {
	MOLT_VOL_NEXT,
	MOLT_VOL_CURR,
	MOLT_VOL_PREV
};

#define MOLT_FLAG_FIRSTSTEP 0x01
#define MOLT_WORKSTORE_AMT  8

static char molt_staticbuf[BUFSMALL];

struct molt_cfg_t {
	// simulation values are kept as integers, and are scaled by the
	// following value
	f64 int_scale;

	// the dimensionality of the simulation is stored in s64 arrays for the
	// dimensionality we care about, time, then pos_x, pos_y, pos_z
	lvec5_t t_params;
	lvec5_t x_params;
	lvec5_t y_params;
	lvec5_t z_params;

	/* MOLT parameters */
	s64 spaceacc;
	s64 timeacc;
	f64 beta;
	f64 beta_sq; // b^2 term in sweeping function (for precomputation)
	f64 beta_fo; // b^4 term in sweeping function
	f64 beta_si; // b^6 term in sweeping function
	f64 alpha;

	// NOTE (brian) we can keep nu as a set of scalars as there's unsolved
	// geometry issues to be solved, should we want to really change the
	// granularity of the mesh
	dvec3_t nu;
	dvec3_t dnu;

	// working storage for simulation
	f64 *workstore[MOLT_WORKSTORE_AMT];
	f64 *worksweep;

};

// molt_cfg dimension intializer functions
void molt_cfg_dims_t(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc);
void molt_cfg_dims_x(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc);
void molt_cfg_dims_y(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc);
void molt_cfg_dims_z(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc);
void molt_cfg_set_intscale(struct molt_cfg_t *cfg, f64 scale);
void molt_cfg_set_accparams(struct molt_cfg_t *cfg, f64 spaceacc, f64 timeacc);
void molt_cfg_set_nu(struct molt_cfg_t *cfg);
void molt_cfg_parampull_xyz(struct molt_cfg_t *cfg, s32 *dst, s32 param);
s64 molt_cfg_parampull_gen(struct molt_cfg_t *cfg, s32 oidx, s32 cfgidx, cvec3_t order);

void molt_cfg_set_workstore(struct molt_cfg_t *cfg);
void molt_cfg_free_workstore(struct molt_cfg_t *cfg);

/* molt_step%v : a concise way to setup some parameters for whatever dim is being used */
void molt_step3v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec6_t vw, pdvec6_t ww, u32 flags);
void molt_step2v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec4_t vw, pdvec4_t ww, u32 flags);
void molt_step1v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec2_t vw, pdvec2_t ww, u32 flags);

/* molt_step%f : a less concise, yet more explicit approach to the same thing */
void molt_step1f(struct molt_cfg_t *cfg,
		f64 *next, f64 *curr, f64 *prev,
		f64 *vl_x, f64 *vr_x,
		f64 *wl_x, f64 *wr_x, u32 flags);

void molt_step2f(struct molt_cfg_t *cfg,
		f64 *next, f64 *curr, f64 *prev,
		f64 *vl_x, f64 *vr_x, f64 *vl_y, f64 *vr_y,
		f64 *wl_x, f64 *wr_x, f64 *wl_y, f64 *wr_y, u32 flags);

void molt_step3f(struct molt_cfg_t *cfg,
		f64 *next, f64 *curr, f64 *prev,
		f64 *vl_x, f64 *vr_x, f64 *vl_y, f64 *vr_y, f64 *vl_z, f64 *vr_z,
		f64 *wl_x, f64 *wr_x, f64 *wl_y, f64 *wr_y, f64 *wl_z, f64 *wr_z, u32 flags);


void molt_d_op(struct molt_cfg_t *cfg, pdvec2_t vol, pdvec6_t vw, pdvec6_t ww);
void molt_c_op(struct molt_cfg_t *cfg, pdvec2_t vol, pdvec6_t vw, pdvec6_t ww);

// molt_sweep : performs a sweep across the mesh in the dimension specified
void molt_sweep(struct molt_cfg_t *cfg, f64 *dst, f64 *src, pdvec4_t params, cvec3_t order);

/* molt_reorg : reorganize a 3d volume */
void molt_reorg(struct molt_cfg_t *cfg, f64 *dst, f64 *src, f64 *work, cvec3_t src_ord, cvec3_t dst_ord);

/* molt_gfquad_m : green's function quadriture on the input vector */
void molt_gfquad_m(struct molt_cfg_t *cfg, f64 *out, f64 *in, pdvec4_t params, cvec3_t order);

/* molt_makel : applies dirichlet boundary conditions to the line in */
void molt_makel(struct molt_cfg_t *cfg, f64 *arr, pdvec4_t params, f64 minval, s64 len);

/* molt_vect_mul : perform element-wise vector multiplication */
f64 molt_vect_mul(f64 *veca, f64 *vecb, s32 veclen);

#ifdef MOLT_IMPLEMENTATION

// TODO remove if possible
#include <float.h>

static cvec3_t molt_ord_xyz = {'x', 'y', 'z'};
static cvec3_t molt_ord_xzy = {'x', 'z', 'y'};
static cvec3_t molt_ord_yxz = {'y', 'x', 'z'};
static cvec3_t molt_ord_yzx = {'y', 'z', 'x'};
static cvec3_t molt_ord_zxy = {'z', 'x', 'y'};
static cvec3_t molt_ord_zyx = {'z', 'y', 'x'};

static s32 molt_gfquad_bound(s32 idx, s32 m2, s32 len);

/* molt_cfg_dims_t : initializes, very explicitly, cfg's time parameters */
void molt_cfg_dims_t(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc)
{
	cfg->t_params[MOLT_PARAM_START]  = start;
	cfg->t_params[MOLT_PARAM_STOP]   = stop;
	cfg->t_params[MOLT_PARAM_STEP]   = step;
	cfg->t_params[MOLT_PARAM_POINTS] = points;
	cfg->t_params[MOLT_PARAM_PINC]   = pointsinc;
}

/* molt_cfg_dims_x : initializes, very explicitly, cfg's x parameters */
void molt_cfg_dims_x(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc)
{
	cfg->x_params[MOLT_PARAM_START]  = start;
	cfg->x_params[MOLT_PARAM_STOP]   = stop;
	cfg->x_params[MOLT_PARAM_STEP]   = step;
	cfg->x_params[MOLT_PARAM_POINTS] = points;
	cfg->x_params[MOLT_PARAM_PINC]   = pointsinc;
}

/* molt_cfg_dims_y : initializes, very explicitly, cfg's y parameters */
void molt_cfg_dims_y(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc)
{
	cfg->y_params[MOLT_PARAM_START]  = start;
	cfg->y_params[MOLT_PARAM_STOP]   = stop;
	cfg->y_params[MOLT_PARAM_STEP]   = step;
	cfg->y_params[MOLT_PARAM_POINTS] = points;
	cfg->y_params[MOLT_PARAM_PINC]   = pointsinc;
}

/* molt_cfg_dims_z : initializes, very explicitly, cfg's z parameters */
void molt_cfg_dims_z(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc)
{
	cfg->z_params[MOLT_PARAM_START]  = start;
	cfg->z_params[MOLT_PARAM_STOP]   = stop;
	cfg->z_params[MOLT_PARAM_STEP]   = step;
	cfg->z_params[MOLT_PARAM_POINTS] = points;
	cfg->z_params[MOLT_PARAM_PINC]   = pointsinc;
}

/* molt_cfg_parampull_gen : generic param puller for an order, config idx and the order's idx */
s64 molt_cfg_parampull_gen(struct molt_cfg_t *cfg, s32 oidx, s32 cfgidx, cvec3_t order)
{
	switch (order[oidx]) {
	case 'x':
		return cfg->x_params[cfgidx];
	case 'y':
		return cfg->y_params[cfgidx];
	case 'z':
		return cfg->z_params[cfgidx];
	}

	return -1;
}

/* molt_cfg_set_intscale : sets the integer scale value (trivial) */
void molt_cfg_set_intscale(struct molt_cfg_t *cfg, f64 scale)
{
	cfg->int_scale = scale;
}

/* molt_cfg_set_accparams : set MOLT accuracy parameters */
void molt_cfg_set_accparams(struct molt_cfg_t *cfg, f64 spaceacc, f64 timeacc)
{
	cfg->spaceacc = spaceacc;
	cfg->timeacc  = timeacc ;

	// TODO possibly compute these values here???
	switch (cfg->timeacc) {
	case 3:
		cfg->beta = 1.23429074525305;
		break;
	case 2:
		cfg->beta = 1.48392756860545;
		break;
	case 1:
		cfg->beta = 2;
		break;
	default:
		fprintf(stderr, "ERR : unsupported time accuracy value '%lf'\n", cfg->beta);
		assert(0);
		break;
	}

	cfg->beta_sq = pow(cfg->beta, 2);
	cfg->beta_fo = pow(cfg->beta, 4) / 12;
	cfg->beta_si = pow(cfg->beta, 6) / 360;
}

/* molt_cfg_set_nu : computes nu and dnu values from existing cfg structure */
void molt_cfg_set_nu(struct molt_cfg_t *cfg)
{
	assert(cfg->alpha);

	cfg->nu[0] = cfg->int_scale * cfg->x_params[MOLT_PARAM_STEP] * cfg->alpha;
	cfg->nu[1] = cfg->int_scale * cfg->y_params[MOLT_PARAM_STEP] * cfg->alpha;
	cfg->nu[2] = cfg->int_scale * cfg->z_params[MOLT_PARAM_STEP] * cfg->alpha;
	cfg->dnu[0] = exp(-cfg->nu[0]);
	cfg->dnu[1] = exp(-cfg->nu[1]);
	cfg->dnu[2] = exp(-cfg->nu[2]);
}

/* molt_cfg_set_workstore : auto sets up the working store with the CRT */
void molt_cfg_set_workstore(struct molt_cfg_t *cfg)
{
	/*
	 * NOTE
	 * assumes the following:
	 *
	 * These have the dimensionality of the mesh:
	 *   workstore[0, 1, 2]       Used in molt_step3v
	 *   workstore[3, 4, 5, 6, 7] Used in the C and D operators
	 *
	 * This has the dimensionality of the LONGEST dimension
	 *   workstore[8]             Used in molt_sweep
	 */

	ivec3_t dim;
	s64 elems, len;
	s32 i;

	molt_cfg_parampull_xyz(cfg, dim, MOLT_PARAM_PINC);
	elems = ((s64)dim[0]) * dim[1] * dim[2];

	// find the largest dimension
	for (i = 0, len = dim[0]; i < 3; i++) {
		if (len < dim[i])
			len = dim[i];
	}

	for (i = 0; i < 8; i++) {
		cfg->workstore[i] = calloc(elems, sizeof(f64));
		// PERFLOG_APP_AND_PRINT(molt_staticbuf, "workstore[%d] : %p", i, cfg->workstore[i]);
	}

	cfg->worksweep = calloc(len, sizeof(f64));
}

/* molt_cfg_free_workstore : frees all of the working storage */
void molt_cfg_free_workstore(struct molt_cfg_t *cfg)
{
	s32 i;

	for (i = 0; i < 8; i++) {
		free(cfg->workstore[i]);
		cfg->workstore[i] = NULL;
	}

	free(cfg->worksweep);
	cfg->worksweep = NULL;
}

/* molt_cfg_parampull_xyz : helper func to pull out xyz params into dst */
void molt_cfg_parampull_xyz(struct molt_cfg_t *cfg, s32 *dst, s32 param)
{
	// NOTE (brian) the pulling is always in xyz order
	dst[0] = cfg->x_params[param];
	dst[1] = cfg->y_params[param];
	dst[2] = cfg->z_params[param];
}

/* molt_step1f : explicit parameter for molt stepping in 1d */
void molt_step1f(struct molt_cfg_t *cfg,
		f64 *next, f64 *curr, f64 *prev,
		f64 *vl_x, f64 *vr_x,
		f64 *wl_x, f64 *wr_x, u32 flags)
{
	// NOTE we really just setup params for molt_step3v, and execute that
	pdvec3_t vol;
	pdvec6_t vw, ww;

	memset(&vol, 0, sizeof vol);
	memset( &vw, 0, sizeof vw);
	memset( &ww, 0, sizeof ww);

	// MOV parameters
	vol[0] = next; vol[1] = curr; vol[2] = prev;
	vw[0] = vl_x; vw[1] = vr_x;
	ww[0] = wl_x; ww[1] = wr_x;

	molt_step3v(cfg, vol, vw, ww, flags);
}

/* molt_step2f : explicit parameter for molt stepping in 2d */
void molt_step2f(struct molt_cfg_t *cfg,
		f64 *next, f64 *curr, f64 *prev,
		f64 *vl_x, f64 *vr_x, f64 *vl_y, f64 *vr_y,
		f64 *wl_x, f64 *wr_x, f64 *wl_y, f64 *wr_y, u32 flags)
{
	// NOTE we really just setup params for molt_step3v, and execute that
	pdvec3_t vol;
	pdvec4_t vw, ww;

	memset(&vol, 0, sizeof vol);
	memset( &vw, 0, sizeof vw);
	memset( &ww, 0, sizeof ww);

	// MOV parameters
	vol[0] = next;
	vol[1] = curr;
	vol[2] = prev;

	vw[0] = vl_x;
	vw[1] = vr_x;
	vw[2] = vl_y;
	vw[3] = vr_y;

	ww[0] = wl_x;
	ww[1] = wr_x;
	ww[2] = wl_y;
	ww[3] = wr_y;

	molt_step3v(cfg, vol, vw, ww, flags);
}

/* molt_step3f : explicit parameter for molt stepping in 3d */
void molt_step3f(struct molt_cfg_t *cfg,
		f64 *next, f64 *curr, f64 *prev,
		f64 *vl_x, f64 *vr_x, f64 *vl_y, f64 *vr_y, f64 *vl_z, f64 *vr_z,
		f64 *wl_x, f64 *wr_x, f64 *wl_y, f64 *wr_y, f64 *wl_z, f64 *wr_z, u32 flags)
{
	// NOTE we really just setup params for molt_step3v, and execute that
	pdvec3_t vol;
	pdvec6_t vw, ww;

	memset(&vol, 0, sizeof vol);
	memset( &vw, 0, sizeof vw);
	memset( &ww, 0, sizeof ww);

	// MOV parameters
	vol[0] = next;
	vol[1] = curr;
	vol[2] = prev;

	vw[0] = vl_x;
	vw[1] = vr_x;
	vw[2] = vl_y;
	vw[3] = vr_y;
	vw[4] = vl_z;
	vw[5] = vr_z;

	ww[0] = wl_x;
	ww[1] = wr_x;
	ww[2] = wl_y;
	ww[3] = wr_y;
	ww[4] = wl_z;
	ww[5] = wr_z;

	molt_step3v(cfg, vol, vw, ww, flags);
}

/* molt_step1v : concise function for calling molt_step3v from 1d data */
void molt_step1v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec2_t vw, pdvec2_t ww, u32 flags)
{
	// NOTE we really just setup params for molt_step3v, and execute that
	pdvec6_t outvw, outww;

	memset( &outvw, 0, sizeof outvw);
	memset( &outww, 0, sizeof outww);

	outvw[0] = vw[0];
	outvw[1] = vw[1];

	outww[0] = ww[0];
	outww[1] = ww[1];

	molt_step3v(cfg, vol, outvw, outww, flags);
}

/* molt_step2v : concise function for calling molt_step3v from 2d data */
void molt_step2v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec4_t vw, pdvec4_t ww, u32 flags)
{
	// NOTE we really just setup params for molt_step3v, and execute that
	pdvec6_t outvw, outww;

	memset( &outvw, 0, sizeof outvw);
	memset( &outww, 0, sizeof outww);

	outvw[0] = vw[0];
	outvw[1] = vw[1];
	outvw[2] = vw[2];
	outvw[3] = vw[3];

	outww[0] = ww[0];
	outww[1] = ww[1];
	outww[2] = ww[2];
	outww[3] = ww[3];

	molt_step3v(cfg, vol, vw, ww, flags);
}

void molt_cfg_print(struct molt_cfg_t *cfg)
{
	s32 i;
	printf("int_scale : %lf", cfg->int_scale);
	printf("t_params : "); for (i = 0; i < 5; i++) { printf("%ld", cfg->t_params[i]); } printf("\n");
	printf("x_params : "); for (i = 0; i < 5; i++) { printf("%ld", cfg->x_params[i]); } printf("\n");
	printf("y_params : "); for (i = 0; i < 5; i++) { printf("%ld", cfg->y_params[i]); } printf("\n");
	printf("z_params : "); for (i = 0; i < 5; i++) { printf("%ld", cfg->z_params[i]); } printf("\n");

	printf("workstore :\n");
	for (i = 0; i < MOLT_WORKSTORE_AMT; i++) {
		printf("\t0x%p\n", cfg->workstore[i]);
	}
}

/* molt_step3v : the actual MOLT function, everything else is sugar */
void molt_step3v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec6_t vw, pdvec6_t ww, u32 flags)
{
	u64 totalelem, i;
	ivec3_t mesh_dim;
	f64 *work_d1, *work_d2, *work_d3;
	f64 *next, *curr, *prev;
	f64 tmp;
	pdvec2_t opstor;

	// first, we have to acquire some working storage, external to our meshes
	molt_cfg_parampull_xyz(cfg, mesh_dim, MOLT_PARAM_PINC);

	totalelem = ((u64)mesh_dim[0]) * mesh_dim[1] * mesh_dim[2];

	work_d1 = cfg->workstore[0];
	work_d2 = cfg->workstore[1];
	work_d3 = cfg->workstore[2];

	next = vol[MOLT_VOL_NEXT];
	curr = vol[MOLT_VOL_CURR];
	prev = vol[MOLT_VOL_PREV];

	if (flags & MOLT_FLAG_FIRSTSTEP) {
		// u1 = 2 * (u0 + d1 * v0)
		tmp = cfg->int_scale * cfg->t_params[MOLT_PARAM_STEP];
		for (i = 0; i < totalelem; i++) {
			next[i] = 2 * (curr[i] + tmp * prev[i]);
		}
	}

	// now we can begin doing work
	opstor[0] = work_d1, opstor[1] = next;
	molt_c_op(cfg, opstor, vw, ww);

	// u = u + beta ^ 2 * D1
	for (i = 0; i < totalelem; i++) {
		next[i] = next[i] * cfg->beta_sq * work_d1[i];
	}

	if (cfg->timeacc >= 2) {
		opstor[0] = work_d2, opstor[1] = work_d1;
		molt_c_op(cfg, opstor, vw, ww);
		opstor[0] = work_d1, opstor[1] = work_d1;
		molt_c_op(cfg, opstor, vw, ww);

		// u = u - beta ^ 2 * D2 + beta ^ 4 / 12 * D1
		for (i = 0; i < totalelem; i++) {
			next[i] -= cfg->beta_sq * work_d2[i] + cfg->beta_fo * work_d1[i];
		}
	}

	if (cfg->timeacc >= 3) {
		opstor[0] = work_d3, opstor[1] = work_d2;
		molt_c_op(cfg, opstor, vw, ww);
		opstor[0] = work_d2, opstor[1] = work_d2;
		molt_c_op(cfg, opstor, vw, ww);
		opstor[0] = work_d1, opstor[1] = work_d1;
		molt_c_op(cfg, opstor, vw, ww);

		// u = u + (beta ^ 2 * D3 - 2 * beta ^ 4 / 12 * D2 + beta ^ 6 / 360 * D1)
		for (i = 0; i < totalelem; i++) {
			tmp = cfg->beta_sq * work_d3[i] - cfg->beta_fo * work_d2[i] + cfg->beta_si * work_d1[i];
			next[i] += tmp;
		}
	}

	if (flags & MOLT_FLAG_FIRSTSTEP) {
		// next = next / 2;
		for (i = 0; i < totalelem; i++) { next[i] /= 2; }
	} else {
		// next = next + 2 * curr - prev
		for (i = 0; i < totalelem; i++) { next[i] += 2 * curr[i] - prev[i]; }
	}
}

/* molt_d_op : MOLT's D Convolution Operator*/
void molt_d_op(struct molt_cfg_t *cfg, pdvec2_t vol, pdvec6_t vw, pdvec6_t ww)
{
	/*
	 * NOTE (brian)
	 *
	 * The decisions that led to the layout of this function are for the
	 * following reasons.
	 *
	 * 1. Functions can be read by humans when they're more concise
	 * 2. Modern CPU architectures have caches. Data has to be in a cache for
	 *    the CPU to be able to use it. If it isn't in the cache, the CPU
	 *    generates a cache-miss and retrieves the data from a higher level of
	 *    cache (L1->L2->L3) or from memory (L3->Memory). For performance,
	 *    it is best to align items that are used right next to each other to
	 *    prevent these cache misses. Generally, cache lines (one cache read)
	 *    are 64bytes long. Because all of the data _has_ to be double
	 *    precision, this means we can only read 8 items from our array "at
	 *    once".
	 *
	 *    So, to optimize for this read/write set of operations that happens
	 *    in 64bytes at a time, if we spin the mesh's dimensionality
	 *    around and optimize that algorithm, apply hardware to it, etc,
	 *    the _hard_ part is no longer the sweeping or the convolution,
	 *    it's copying memory.
	 * 3. Performing the mesh dimension swap that happens in mesh_swap without
	 *    working storage is difficult. For the moment, it will rely upon
	 *    working memory at work_tmp. In the FUTURE, this could probably be
	 *    optimized by doing some tiling stuff.
	 */

	u64 totalelem, i;
	ivec3_t mesh_dim;
	f64 *work_ix, *work_iy, *work_iz, *work_tmp;
	f64 *src, *dst;

	pdvec6_t x_sweep_params, y_sweep_params, z_sweep_params;

	dst = vol[0];
	src = vol[1];

	// first, we have to acquire some working storage, external to our meshes
	molt_cfg_parampull_xyz(cfg, mesh_dim, MOLT_PARAM_PINC);

	totalelem = ((u64)mesh_dim[0]) * mesh_dim[1] * mesh_dim[2];

	x_sweep_params[0] = vw[0];
	x_sweep_params[1] = vw[1];
	x_sweep_params[2] = ww[0];
	x_sweep_params[3] = ww[1];

	y_sweep_params[0] = vw[2];
	y_sweep_params[1] = vw[3];
	y_sweep_params[2] = ww[2];
	y_sweep_params[3] = ww[3];

	z_sweep_params[0] = vw[4];
	z_sweep_params[1] = vw[5];
	z_sweep_params[2] = ww[4];
	z_sweep_params[3] = ww[5];

	work_ix  = cfg->workstore[3];
	work_iy  = cfg->workstore[4];
	work_iz  = cfg->workstore[5];
	work_tmp = cfg->workstore[6];

	// sweep in x, y, z
	molt_sweep(cfg, work_ix,     src, x_sweep_params, molt_ord_xyz);
	molt_reorg(cfg, work_ix, work_ix, work_tmp, molt_ord_xyz, molt_ord_yxz);
	molt_sweep(cfg, work_ix, work_ix, y_sweep_params, molt_ord_yxz);
	molt_reorg(cfg, work_ix, work_ix, work_tmp, molt_ord_yxz, molt_ord_zxy);
	molt_sweep(cfg, work_ix, work_ix, z_sweep_params, molt_ord_zxy);
	molt_reorg(cfg, work_ix, work_ix, work_tmp, molt_ord_zxy, molt_ord_xyz);

	// sweep in y, z, x
	molt_reorg(cfg, work_iy,     src, work_tmp, molt_ord_xyz, molt_ord_yxz);
	molt_sweep(cfg, work_iy, work_iy, y_sweep_params, molt_ord_yxz);
	molt_reorg(cfg, work_iy, work_ix, work_tmp, molt_ord_yxz, molt_ord_zxy);
	molt_sweep(cfg, work_iy, work_ix, z_sweep_params, molt_ord_zxy);
	molt_reorg(cfg, work_iy, work_ix, work_tmp, molt_ord_zxy, molt_ord_xzy);
	molt_sweep(cfg, work_iy, work_ix, x_sweep_params, molt_ord_xzy);
	molt_reorg(cfg, work_iy, work_iy, work_tmp, molt_ord_xzy, molt_ord_xyz);

	// sweep in z, x, y
	molt_reorg(cfg, work_iz,     src, work_tmp, molt_ord_xyz, molt_ord_zxy);
	molt_sweep(cfg, work_iz, work_iy, z_sweep_params, molt_ord_zxy);
	molt_reorg(cfg, work_iz, work_ix, work_tmp, molt_ord_zxy, molt_ord_xzy);
	molt_sweep(cfg, work_iz, work_ix, x_sweep_params, molt_ord_xzy);
	molt_reorg(cfg, work_iz, work_ix, work_tmp, molt_ord_xzy, molt_ord_yzx);
	molt_sweep(cfg, work_iz, work_ix, y_sweep_params, molt_ord_yzx);
	molt_reorg(cfg, work_iy, work_iy, work_tmp, molt_ord_yzx, molt_ord_xyz);

	// dst = (work_ix + work_iy + work_iz) / 2
	for (i = 0; i < totalelem; i++) {
		dst[i] = (work_ix[i] + work_iy[i] + work_iz[i]) / 3 - src[i];
	}
}

/* molt_c_op : MOLT's C Convolution Operator*/
void molt_c_op(struct molt_cfg_t *cfg, pdvec2_t vol, pdvec6_t vw, pdvec6_t ww)
{
	/*
	 * NOTE (brian)
	 *
	 * This function looks the way it does because of the reasoning in the D
	 * operator. HOWEVER, this one's special because we have to subtract off
	 * src from the results of our first sweep. Not a big deal when it's
	 * organized in X, but when it's in Y and Z, this is a pretty big issue.
	 */

	u64 totalelem, i;
	ivec3_t mesh_dim;
	f64 *work_ix, *work_iy, *work_iz, *work_tmp, *work_tmp_;
	f64 *src, *dst;

	pdvec6_t x_sweep_params, y_sweep_params, z_sweep_params;

	dst = vol[0];
	src = vol[1];

	// first, we have to acquire some working storage, external to our meshes
	molt_cfg_parampull_xyz(cfg, mesh_dim, MOLT_PARAM_PINC);

	totalelem = ((u64)mesh_dim[0]) * mesh_dim[1] * mesh_dim[2];

	x_sweep_params[0] = vw[0];
	x_sweep_params[1] = vw[1];
	x_sweep_params[2] = ww[0];
	x_sweep_params[3] = ww[1];

	y_sweep_params[0] = vw[2];
	y_sweep_params[1] = vw[3];
	y_sweep_params[2] = ww[2];
	y_sweep_params[3] = ww[3];

	z_sweep_params[0] = vw[4];
	z_sweep_params[1] = vw[5];
	z_sweep_params[2] = ww[4];
	z_sweep_params[3] = ww[5];

	// TEMPORARY
	// work_ix = calloc(totalelem, sizeof (double));
	work_ix   = cfg->workstore[3];
	work_iy   = cfg->workstore[4];
	work_iz   = cfg->workstore[5];
	work_tmp  = cfg->workstore[6];
	work_tmp_ = cfg->workstore[7];

	// sweep in x, y, z
	molt_sweep(cfg, work_ix,     src, x_sweep_params, molt_ord_xyz);
	for (i = 0; i < totalelem; i++) {
		work_ix[i] -= src[i];
	}
	molt_reorg(cfg, work_ix, work_ix, work_tmp, molt_ord_xyz, molt_ord_yxz);
	molt_sweep(cfg, work_ix, work_ix, y_sweep_params, molt_ord_yxz);
	molt_reorg(cfg, work_ix, work_ix, work_tmp, molt_ord_yxz, molt_ord_zxy);
	molt_sweep(cfg, work_ix, work_ix, z_sweep_params, molt_ord_zxy);
	molt_reorg(cfg, work_ix, work_ix, work_tmp, molt_ord_zxy, molt_ord_xyz);

	// sweep in y, z, x
	molt_reorg(cfg, work_iy,     src, work_tmp, molt_ord_xyz, molt_ord_yxz);
	molt_reorg(cfg, work_tmp_,   src, work_tmp, molt_ord_xyz, molt_ord_yxz);
	molt_sweep(cfg, work_iy, work_iy, y_sweep_params, molt_ord_yxz);
	for (i = 0; i < totalelem; i++) { work_iy[i] -= work_tmp_[i]; }
	molt_reorg(cfg, work_iy, work_iy, work_tmp, molt_ord_yxz, molt_ord_zxy);
	molt_sweep(cfg, work_iy, work_iy, z_sweep_params, molt_ord_zxy);
	molt_reorg(cfg, work_iy, work_iy, work_tmp, molt_ord_zxy, molt_ord_xzy);
	molt_sweep(cfg, work_iy, work_iy, x_sweep_params, molt_ord_xzy);
	molt_reorg(cfg, work_iy, work_iy, work_tmp, molt_ord_xzy, molt_ord_xyz);

	// sweep in z, x, y
	molt_reorg(cfg, work_iz,     src, work_tmp, molt_ord_xyz, molt_ord_zxy);
	molt_reorg(cfg, work_tmp_,   src, work_tmp, molt_ord_xyz, molt_ord_zxy);
	molt_sweep(cfg, work_iz, work_iy, z_sweep_params, molt_ord_zxy);
	for (i = 0; i < totalelem; i++) { work_iz[i] -= work_tmp_[i]; }
	molt_reorg(cfg, work_iz, work_iz, work_tmp, molt_ord_zxy, molt_ord_xzy);
	molt_sweep(cfg, work_iz, work_iz, x_sweep_params, molt_ord_xzy);
	molt_reorg(cfg, work_iz, work_iz, work_tmp, molt_ord_xzy, molt_ord_yzx);
	molt_sweep(cfg, work_iz, work_iz, y_sweep_params, molt_ord_yzx);
	molt_reorg(cfg, work_iz, work_iz, work_tmp, molt_ord_yzx, molt_ord_xyz);

	// dst = (work_ix + work_iy + work_iz) / 2
	for (i = 0; i < totalelem; i++) {
		dst[i] = (work_ix[i] + work_iy[i] + work_iz[i]) / 3 - src[i];
	}
}

/* molt_sweep : performs a sweep across the mesh in the dimension specified */
void molt_sweep(struct molt_cfg_t *cfg, f64 *dst, f64 *src, pdvec4_t params, cvec3_t order)
{
	ivec3_t dim;
	f64 *ptr, minval;
	s64 rowlen, rownum, i;

	molt_cfg_parampull_xyz(cfg, dim, MOLT_PARAM_PINC);

	// fetch the row's length and the number of rows
	rowlen  = molt_cfg_parampull_gen(cfg, 0, MOLT_PARAM_PINC, order);
	rownum  = molt_cfg_parampull_gen(cfg, 1, MOLT_PARAM_PINC, order);
	rownum *= molt_cfg_parampull_gen(cfg, 2, MOLT_PARAM_PINC, order);

	// find the minval (dN in Matlab)
	for (i = 0, minval = DBL_MAX; i < rowlen; i++) {
		if (src[i] < minval)
			minval = src[i];
	}

	// walk through the volume, grabbing each row-organized value
	for (i = 0, ptr = src; i < rownum; i++, ptr += rowlen) {
		molt_gfquad_m(cfg, cfg->worksweep, ptr, params, order);
		molt_makel(cfg, cfg->worksweep, params, minval, rowlen);
		memcpy(ptr, cfg->worksweep, sizeof(f64) * rowlen);
	}
}

int quad_count = 0;

/* molt_gfquad_m : green's function quadriture on the input vector */
void molt_gfquad_m(struct molt_cfg_t *cfg, f64 *out, f64 *in, pdvec4_t params, cvec3_t order)
{
	/* out and in's length is defined by hunklen */
	f64 IL, IR;
	f64 d;
	s32 iL, iR, iC, M2, N;
	s32 i, bound, orderm;
	s64 rowlen;

	// pull out the correct dnu value
	switch (order[0]) {
	case 'x':
		d = cfg->dnu[0];
		break;
	case 'y':
		d = cfg->dnu[1];
		break;
	case 'z':
		d = cfg->dnu[2];
		break;
	default:
		assert(0); // TODO recompute it here ??????
	}

	f64 *wl = params[2];
	f64 *wr = params[3];

	// PERFLOG_APP_AND_PRINT(molt_staticbuf, "quadnum %d", quad_count);
	quad_count++;

	rowlen = molt_cfg_parampull_gen(cfg, 0, MOLT_PARAM_PINC, order);
	orderm = cfg->spaceacc;

	IL = 0;
	IR = 0;
	M2 = orderm / 2;
	N = rowlen - 1;

	orderm++;

	iL = 0;
	iC = -M2;
	iR = rowlen - orderm;

	/* left sweep */
	for (i = 0; i < M2; i++) {
		// PERFLOG_SPRINTF(molt_staticbuf, "M %d, i %d", orderm, i);
		IL = d * IL + molt_vect_mul(&wl[i * orderm] , &in[iL], orderm);
		out[i + 1] = out[i + 1] + IL;
	}

	for (; i < N - M2; i++) {
		// PERFLOG_SPRINTF(molt_staticbuf, "M %d, i %d", orderm, i);
		IL = d * IL + molt_vect_mul(&wl[i * orderm], &in[i + 1 + iC], orderm);
		out[i + 1] = out[i + 1] + IL;
	}

	for (; i < N; i++) {
		// PERFLOG_SPRINTF(molt_staticbuf, "M %d, i %d", orderm, i);
		IL = d * IL + molt_vect_mul(&wl[i * orderm], &in[iR], orderm);
		out[i + 1] = out[i + 1] + IL;
	}

	for (i = N - 1; i > N - 1 - M2; i--) {
		// PERFLOG_SPRINTF(molt_staticbuf, "M %d, i %d", orderm, i);
		IR = d * IR + molt_vect_mul(&wr[i * orderm], &in[iL], orderm);
		out[i] = out[i] + IR;
	}

	for (; i > M2; i--) {
		// PERFLOG_SPRINTF(molt_staticbuf, "M %d, i %d", orderm, i);
		IR = d * IR + molt_vect_mul(&wr[i * orderm], &in[i + 1 + iC], orderm);
		out[i] = out[i] + IR;
	}

	for (; i >= 0; i--) {
		// PERFLOG_SPRINTF(molt_staticbuf, "M %d, i %d", orderm, i);
		IR = d * IR + molt_vect_mul(&wr[i * orderm], &in[iR], orderm);
		out[i] = out[i] + IR;
	}

#if 0
	for (i = 0; i < N; i++) { /* left */
		bound = molt_gfquad_bound(i, M2, N);

		switch (bound) {
		case -1: // left bound
			IL = d * IL + molt_vect_mul(&wl[i * orderm], &in[iL], orderm);
			break;
		case 0: // center
			IL = d * IL + molt_vect_mul(&wl[i * orderm], &in[i + 1 + iC], orderm);
			break;
		case 1:
			IL = d * IL + molt_vect_mul(&wl[i * orderm], &in[iR], orderm);
			break;
		}

		out[i + 1] = out[i + 1] + IL;
	}

	for (i = N; i >= 0; i--) { /* right */
		bound = molt_gfquad_bound(i, M2, N);

		switch (bound) {
		case -1: // left bound
			IR = d * IR + molt_vect_mul(&wr[i * orderm], &in[iL], orderm);
			break;
		case 0: // center
			IR = d * IR + molt_vect_mul(&wr[i * orderm], &in[i + 1 + iC], orderm);
			break;
		case 1: // right
			IR = d * IR + molt_vect_mul(&wr[i * orderm], &in[iR], orderm);
			break;
		}

		out[i] = out[i] + IR;
	}
#endif

	for (i = 0; i < rowlen; i++)
		out[i] /= 2;
}

/* molt_gfquad_bound : return easy bound parameters for gfquad */
static s32 molt_gfquad_bound(s32 idx, s32 m2, s32 len)
{
	if (0 <= idx && idx < m2) {
		return -1;
	}

	if (m2 <= idx && idx <= len - m2) {
		return 0;
	}

	if (len - m2 <= idx && idx <= len) {
		return 1;
	}

	assert(0);
	return 0;
}

/* molt_makel : applies dirichlet boundary conditions to the line in */
void molt_makel(struct molt_cfg_t *cfg, f64 *arr, pdvec6_t params, f64 minval, s64 len)
{
	/*
	 * molt_makel applies dirichlet boundary conditions to the line in place
	 *
	 * Executes this 
	 * w = w + ((wa - w(1)) * (vL - dN * vR) + (wb - w(end)) * (vR - dN * vL))
	 *			/ (1 - dN ^ 2)
	 *
	 * NOTE(s)
	 * wa and wb are left here as const scalars for future expansion of boundary
	 * conditions.
	 */

	f64 *vl, *vr;
	f64 wa_use, wb_use, wc_use;
	f64 val;
	s64 i;

	const f64 wa = 0;
	const f64 wb = 0;

	vl = params[2];
	vr = params[3];

	// * wa_use - w(1)
	// * wb_use - w(end)
	// * wc_use - 1 - dN ^ 2
	wa_use = wa - arr[0];
	wb_use = wb - arr[len - 1];
	wc_use = 1 - pow(minval, 2);

	for (i = 0; i < len; i++) {
		val  = wa_use * vl[i] - minval * vr[i];
		val += wb_use * vr[i] - minval * vl[i];
		val /= wc_use;
		arr[i] += val;
	}
}

/* mk_genericidx : retrieves a generic index from input dimensionality */
static u64 molt_genericidx(ivec3_t ival, ivec3_t idim, cvec3_t order)
{
	/*
	 * inpts and dim are assumed to come in with the structure:
	 *   - 0 -> x
	 *   - 1 -> y
	 *   - 2 -> z
	 *
	 * ival - input x, y, z values
	 * idim - input x, y, z dimensions
	 * lval - local x, y, z values
	 * ldim - local dim of order applied on idim
	 *
	 * and the order vector is used to reorganize them to provide us with
	 * the output index
	 */

	ivec3_t lval, ldim;
	s32 i;

	for (i = 0; i < 3; i++) {
		switch (order[i]) {
		case 'x':
			lval[i] = ival[0];
			ldim[i] = idim[0];
			break;
		case 'y':
			lval[i] = ival[1];
			ldim[i] = idim[1];
			break;
		case 'z':
			lval[i] = ival[2];
			ldim[i] = idim[2];
			break;
		}
	}

	return IDX3D(lval[0], lval[1], lval[2], ldim[1], ldim[2]);
}


/* molt_reorg : reorganizes a 3d mesh from src to dst */
void molt_reorg(struct molt_cfg_t *cfg, f64 *dst, f64 *src, f64 *work, cvec3_t src_ord, cvec3_t dst_ord)
{
	/*
	 * cfg - config structure
	 * dst - destination of reorg
	 * src - source of reorg
	 * work - working storage for swap around (avoids same pointer problem)
	 * src_ord - the ordering of src
	 * dst_ord - the ordering of dsr
	 *
	 * WARNING : this WILL fail if the input and output pointers are the same
	 */

	u64 src_i, dst_i, total;
	ivec3_t idx, dim;

	molt_cfg_parampull_xyz(cfg, dim, MOLT_PARAM_PINC);

	total = ((u64)dim[0]) * dim[1] * dim[2];

	// PERFLOG_APP_AND_PRINT(molt_staticbuf, "REORG (%d,%d,%d) : DST %p, WORK %p, SRC %p",
			// dim[0], dim[1], dim[2], dst, work, src);

	for (idx[2] = 0; idx[2] < dim[2]; idx[2]++) {
		for (idx[1] = 0; idx[1] < dim[1]; idx[1]++) {
			for (idx[0] = 0; idx[0] < dim[0]; idx[0]++) {
				src_i = molt_genericidx(idx, dim, src_ord);
				dst_i = molt_genericidx(idx, dim, dst_ord);
				work[dst_i] = src[src_i];
			}
		}
	}

	memcpy(dst, src, sizeof(*dst) * total);
}

/* molt_vect_mul : perform element-wise vector multiplication */
f64 molt_vect_mul(f64 *veca, f64 *vecb, s32 veclen)
{
	f64 val;
	s32 i;

	for (val = 0, i = 0; i < veclen; i++) {
		val += veca[i] * vecb[i];
	}

	return val;
}

#endif // MOLT_IMPLEMENTATION

#endif // MOLT_H

