#ifndef MOLT_H
#define MOLT_H

#include "common.h"

/*
 * Brian Chrzanowski
 * Sat Apr 06, 2019 22:21
 *
 * TODO Create a Single-Header file Lib (Brian)
 * 1. Initializer Functions
 * 2. Use CSTDLIB math.h? stdio.h? stdlib.h?
 * 3. The problem gets considerably harder when we don't (ab)use working storage
 *
 * TODO Future
 * ?. Consider putting in the alpha calculation
 * ?. Debug/Test
 *
 * TODO Even Higher Amounts of Performance (Brian)
 * 1. In Place Mesh Reorganize (HARD)
 * 2. Make function declaration not look like trash
 * 3. Figure out if we actually need swap_sub
 *
 * QUESTIONS:
 * 1. Should we use something other than vL for our weights all the time?
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
};

// molt_cfg dimension intializer functions
void molt_cfg_dims_t(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc);
void molt_cfg_dims_x(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc);
void molt_cfg_dims_y(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc);
void molt_cfg_dims_z(struct molt_cfg_t *cfg, s64 start, s64 stop, s64 step, s64 points, s64 pointsinc);

// molt_cfg integer scaling set
void molt_cfg_set_intscale(struct molt_cfg_t *cfg, f64 scale);

// molt_cfg set accuracy based parameters
void molt_cfg_set_accparams(struct molt_cfg_t *cfg, f64 spaceacc, f64 timeacc);

// parampull_xyz pulls out a parameter of xyz into dst
void molt_cfg_parampull_xyz(struct molt_cfg_t *cfg, s32 *dst, s32 param);

/* molt_step%v : a concise way to setup some parameters for whatever dim is being used */
void molt_step3v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec6_t nu, pdvec6_t vw, pdvec6_t ww, u32 flags);
void molt_step2v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec4_t nu, pdvec4_t vw, pdvec4_t ww, u32 flags);
void molt_step1v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec2_t nu, pdvec2_t vw, pdvec2_t ww, u32 flags);

/* molt_step%f : a less concise, yet more explicit approach to the same thing */
void molt_step1f(struct molt_cfg_t *cfg,
		f64 *next, f64 *curr, f64 *prev,
		f64 *nu_x, f64 *dnu_x,
		f64 *vl_x, f64 *vr_x,
		f64 *wl_x, f64 *wr_x, u32 flags);

void molt_step2f(struct molt_cfg_t *cfg,
		f64 *next, f64 *curr, f64 *prev,
		f64 *nu_x, f64 *dnu_x, f64 *nu_y, f64 *dnu_y,
		f64 *vl_x, f64 *vr_x, f64 *vl_y, f64 *vr_y,
		f64 *wl_x, f64 *wr_x, f64 *wl_y, f64 *wr_y, u32 flags);

void molt_step3f(struct molt_cfg_t *cfg,
		f64 *next, f64 *curr, f64 *prev,
		f64 *nu_x, f64 *dnu_x, f64 *nu_y, f64 *dnu_y, f64 *nu_z, f64 *dnu_z,
		f64 *vl_x, f64 *vr_x, f64 *vl_y, f64 *vr_y, f64 *vl_z, f64 *vr_z,
		f64 *wl_x, f64 *wr_x, f64 *wl_y, f64 *wr_y, f64 *wl_z, f64 *wr_z, u32 flags);


void molt_d_op(struct molt_cfg_t *cfg, pdvec2_t vol, pdvec6_t nu, pdvec6_t vw, pdvec6_t ww);
void molt_c_op(struct molt_cfg_t *cfg, pdvec2_t vol, pdvec6_t nu, pdvec6_t vw, pdvec6_t ww);

/* molt_sweep : performs a sweep across the mesh in the dimension specified */
void molt_sweep(struct molt_cfg_t *cfg, f64 *dst, f64 *src, pdvec6_t params, cvec3_t order);

/* molt_reorg : reorganize a 3d volume */
void molt_reorg(struct molt_cfg_t *cfg, f64 *dst, f64 *src, f64 *work, cvec3_t src_ord, cvec3_t dst_ord);

#ifdef MOLT_IMPLEMENTATION

static cvec3_t molt_ord_xyz = {'x', 'y', 'z'};
static cvec3_t molt_ord_xzy = {'x', 'z', 'y'};
static cvec3_t molt_ord_yxz = {'y', 'x', 'z'};
static cvec3_t molt_ord_yzx = {'y', 'z', 'x'};
static cvec3_t molt_ord_zxy = {'z', 'x', 'y'};
static cvec3_t molt_ord_zyx = {'z', 'y', 'x'};

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
		f64 *nu_x, f64 *dnu_x,
		f64 *vl_x, f64 *vr_x,
		f64 *wl_x, f64 *wr_x, u32 flags)
{
	// NOTE we really just setup params for molt_step3v, and execute that
	pdvec3_t vol, nu, vw, ww;

	memset(&vol, 0, sizeof vol);
	memset( &nu, 0, sizeof nu);
	memset( &vw, 0, sizeof vw);
	memset( &ww, 0, sizeof ww);

	// MOV parameters
	vol[0] = next; vol[1] = curr; vol[2] = prev;
	nu[0] = nu_x; nu[1] = dnu_x;
	vw[0] = vl_x; vw[1] = vr_x;
	ww[0] = wl_x; ww[1] = wr_x;

	molt_step3v(cfg, vol, nu, vw, ww, flags);
}

/* molt_step2f : explicit parameter for molt stepping in 2d */
void molt_step2f(struct molt_cfg_t *cfg,
		f64 *next, f64 *curr, f64 *prev,
		f64 *nu_x, f64 *dnu_x, f64 *nu_y, f64 *dnu_y,
		f64 *vl_x, f64 *vr_x, f64 *vl_y, f64 *vr_y,
		f64 *wl_x, f64 *wr_x, f64 *wl_y, f64 *wr_y, u32 flags)
{
	// NOTE we really just setup params for molt_step3v, and execute that
	pdvec3_t vol, nu, vw, ww;

	memset(&vol, 0, sizeof vol);
	memset( &nu, 0, sizeof nu);
	memset( &vw, 0, sizeof vw);
	memset( &ww, 0, sizeof ww);

	// MOV parameters
	vol[0] = next;
	vol[1] = curr;
	vol[2] = prev;

	nu[0] = nu_x;
	nu[1] = dnu_x;
	nu[2] = nu_y;
	nu[3] = dnu_y;

	vw[0] = vl_x;
	vw[1] = vr_x;
	vw[2] = vl_y;
	vw[3] = vr_y;

	ww[0] = wl_x;
	ww[1] = wr_x;
	ww[2] = wl_y;
	ww[3] = wr_y;

	molt_step3v(cfg, vol, nu, vw, ww, flags);
}

/* molt_step3f : explicit parameter for molt stepping in 3d */
void molt_step3f(struct molt_cfg_t *cfg,
		f64 *next, f64 *curr, f64 *prev,
		f64 *nu_x, f64 *dnu_x, f64 *nu_y, f64 *dnu_y, f64 *nu_z, f64 *dnu_z,
		f64 *vl_x, f64 *vr_x, f64 *vl_y, f64 *vr_y, f64 *vl_z, f64 *vr_z,
		f64 *wl_x, f64 *wr_x, f64 *wl_y, f64 *wr_y, f64 *wl_z, f64 *wr_z, u32 flags)
{
	// NOTE we really just setup params for molt_step3v, and execute that
	pdvec3_t vol, nu, vw, ww;

	memset(&vol, 0, sizeof vol);
	memset( &nu, 0, sizeof nu);
	memset( &vw, 0, sizeof vw);
	memset( &ww, 0, sizeof ww);

	// MOV parameters
	vol[0] = next;
	vol[1] = curr;
	vol[2] = prev;

	nu[0] = nu_x;
	nu[1] = dnu_x;
	nu[2] = nu_y;
	nu[3] = dnu_y;
	nu[4] = nu_z;
	nu[5] = dnu_z;

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

	molt_step3v(cfg, vol, nu, vw, ww, flags);
}

/* molt_step1v : concise function for calling molt_step3v from 1d data */
void molt_step1v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec2_t nu, pdvec2_t vw, pdvec2_t ww, u32 flags)
{
	// NOTE we really just setup params for molt_step3v, and execute that
	pdvec3_t outvol, outnu, outvw, outww;

	memset(&outvol, 0, sizeof outvol);
	memset( &outnu, 0, sizeof outnu);
	memset( &outvw, 0, sizeof outvw);
	memset( &outww, 0, sizeof outww);

	outvol[0] = vol[0];
	outvol[1] = vol[1];
	outvol[2] = vol[2];

	outnu[0] = nu[0];
	outnu[1] = nu[1];

	outvw[0] = vw[0];
	outvw[1] = vw[1];

	outww[0] = ww[0];
	outww[1] = ww[1];

	molt_step3v(cfg, vol, nu, vw, ww, flags);
}

/* molt_step2v : concise function for calling molt_step3v from 2d data */
void molt_step2v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec4_t nu, pdvec4_t vw, pdvec4_t ww, u32 flags)
{
	// NOTE we really just setup params for molt_step3v, and execute that
	pdvec3_t outvol, outnu, outvw, outww;

	memset(&outvol, 0, sizeof outvol);
	memset( &outnu, 0, sizeof outnu);
	memset( &outvw, 0, sizeof outvw);
	memset( &outww, 0, sizeof outww);

	// copy the params one by one
	outvol[0] = vol[0];
	outvol[1] = vol[1];
	outvol[2] = vol[2];

	outnu[0] = nu[0];
	outnu[1] = nu[1];
	outnu[2] = nu[2];
	outnu[3] = nu[3];

	outvw[0] = vw[0];
	outvw[1] = vw[1];
	outvw[2] = vw[2];
	outvw[3] = vw[3];

	outww[0] = ww[0];
	outww[1] = ww[1];
	outww[2] = ww[2];
	outww[3] = ww[3];

	molt_step3v(cfg, vol, nu, vw, ww, flags);
}

/* molt_step3v : the actual MOLT function, everything else is sugar */
void molt_step3v(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec6_t nu, pdvec6_t vw, pdvec6_t ww, u32 flags)
{
	u64 totalelem, i;
	ivec3_t mesh_dim;
	f64 *work_d1, *work_d2, *work_d3;
	f64 *next, *curr, *prev;
	f64 tmp;
	pdvec2_t opstor;

	// first, we have to acquire some working storage, external to our meshes
	molt_cfg_parampull_xyz(cfg, mesh_dim, MOLT_PARAM_POINTS);

	totalelem = mesh_dim[0] * mesh_dim[1] * mesh_dim[2];

	work_d1 = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);
	work_d2 = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);
	work_d3 = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);

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
	opstor[0] = work_d1, opstor[1] = curr;
	molt_c_op(cfg, opstor, nu, vw, ww);

	// u = u + beta ^ 2 * D1
	for (i = 0; i < totalelem; i++) {
		next[i] = next[i] * cfg->beta_sq * work_d1[i];
	}

	if (cfg->timeacc >= 2) {
		opstor[0] = work_d2, opstor[1] = work_d1;
		molt_c_op(cfg, opstor, nu, vw, ww);
		opstor[0] = work_d1, opstor[1] = work_d1;
		molt_c_op(cfg, opstor, nu, vw, ww);

		// u = u - beta ^ 2 * D2 + beta ^ 4 / 12 * D1
		for (i = 0; i < totalelem; i++) {
			next[i] -= cfg->beta_sq * work_d2[i] + cfg->beta_fo * work_d1[i];
		}
	}

	if (cfg->timeacc >= 3) {
		opstor[0] = work_d3, opstor[1] = work_d2;
		molt_c_op(cfg, opstor, nu, vw, ww);
		opstor[0] = work_d2, opstor[1] = work_d2;
		molt_c_op(cfg, opstor, nu, vw, ww);
		opstor[0] = work_d1, opstor[1] = work_d1;
		molt_c_op(cfg, opstor, nu, vw, ww);

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

	free(work_d1);
	free(work_d2);
	free(work_d3);
}

/* molt_d_op : MOLT's D Convolution Operator*/
void molt_d_op(struct molt_cfg_t *cfg, pdvec2_t vol, pdvec6_t nu, pdvec6_t vw, pdvec6_t ww)
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
	f64 tmp;

	pdvec6_t x_sweep_params, y_sweep_params, z_sweep_params;

	dst = vol[0];
	src = vol[1];

	// first, we have to acquire some working storage, external to our meshes
	molt_cfg_parampull_xyz(cfg, mesh_dim, MOLT_PARAM_POINTS);

	totalelem = mesh_dim[0] * mesh_dim[1] * mesh_dim[2];

	x_sweep_params[0] = nu[0];
	x_sweep_params[1] = nu[1];
	x_sweep_params[2] = vw[0];
	x_sweep_params[3] = vw[1];
	x_sweep_params[4] = ww[0];
	x_sweep_params[5] = ww[1];

	y_sweep_params[0] = nu[2];
	y_sweep_params[1] = nu[3];
	y_sweep_params[2] = vw[2];
	y_sweep_params[3] = vw[3];
	y_sweep_params[4] = ww[2];
	y_sweep_params[5] = ww[3];

	z_sweep_params[0] = nu[4];
	z_sweep_params[1] = nu[5];
	z_sweep_params[2] = vw[4];
	z_sweep_params[3] = vw[5];
	z_sweep_params[4] = ww[4];
	z_sweep_params[5] = ww[5];

	work_ix = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);
	work_iy = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);
	work_iz = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);
	work_tmp = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);

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

	free(work_ix);
	free(work_iy);
	free(work_iz);
	free(work_tmp);
}

/* molt_c_op : MOLT's C Convolution Operator*/
void molt_c_op(struct molt_cfg_t *cfg, pdvec3_t vol, pdvec6_t nu, pdvec6_t vw, pdvec6_t ww)
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
	f64 tmp;

	pdvec6_t x_sweep_params, y_sweep_params, z_sweep_params;

	dst = vol[0];
	src = vol[1];

	// first, we have to acquire some working storage, external to our meshes
	molt_cfg_parampull_xyz(cfg, mesh_dim, MOLT_PARAM_POINTS);

	totalelem = mesh_dim[0] * mesh_dim[1] * mesh_dim[2];

	x_sweep_params[0] = nu[0];
	x_sweep_params[1] = nu[1];
	x_sweep_params[2] = vw[0];
	x_sweep_params[3] = vw[1];
	x_sweep_params[4] = ww[0];
	x_sweep_params[5] = ww[1];

	y_sweep_params[0] = nu[2];
	y_sweep_params[1] = nu[3];
	y_sweep_params[2] = vw[2];
	y_sweep_params[3] = vw[3];
	y_sweep_params[4] = ww[2];
	y_sweep_params[5] = ww[3];

	z_sweep_params[0] = nu[4];
	z_sweep_params[1] = nu[5];
	z_sweep_params[2] = vw[4];
	z_sweep_params[3] = vw[5];
	z_sweep_params[4] = ww[4];
	z_sweep_params[5] = ww[5];

	work_ix = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);
	work_iy = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);
	work_iz = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);
	work_tmp = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);
	work_tmp_ = malloc((size_t)mesh_dim[0] * mesh_dim[1] * mesh_dim[2]);

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

	free(work_ix);
	free(work_iy);
	free(work_iz);
	free(work_tmp);
}

/* molt_sweep : performs a sweep across the mesh in the dimension specified */
void molt_sweep(struct molt_cfg_t *cfg, f64 *dst, f64 *src, pdvec6_t params, cvec3_t order)
{
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

	s32 i, *p;
	ivec3_t lval, ldim;

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

	return IDX3D(ival[0], ival[1], ival[2], ldim[1], ldim[2]);
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

	molt_cfg_parampull_xyz(cfg, dim, MOLT_PARAM_POINTS);

	total = (u64)dim[0] * dim[1] * dim[2];

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

#if 0

// TODO move this
static inline s32 gfquad_bound(s32 idx, s32 orderm, s32 len);

/* do_c_op : The MOLT C Operator */
static void do_c_op(f64 *dst, f64 *src, struct cfg_t *cfg, lnu_t *nu, lvweight_t *vw, lwweight_t *ww);

/* do_d_op : The MOLT D Operator */
static void do_d_op(f64 *dst, f64 *src, struct cfg_t *cfg, lnu_t *nu, lvweight_t *vw, lwweight_t *ww);

static
void mesh3_swap(f64 *out, f64* in, ivec3_t *dim, cvec3_t from, cvec3_t to);

static inline u64 mk_genericidx(ivec3_t inpts, ivec3_t dim, cvec3_t order);

/* meshdim_swap : swaps the dim values in dim from 'from' to 'to' */
static void meshdim_swap(ivec3_t *dim, cvec3_t from, cvec3_t to);

static void mk_genericdim(ivec3_t *out, ivec3_t dim, cvec3_t to);

static void matrix_subtract(f64 *out, f64 *ina, f64 *inb, ivec3_t dim);
static f64 minarr(f64 *arr, s32 arrlen);
static f64 vect_mul(f64 *veca, f64 *vecb, s32 veclen);

static void
do_sweep
(f64 *mesh, f64 *work, f64 *dvec, f64 *vl_r, f64 *vr_r,
 f64 *vl_w, f64 *vr_w, f64 *wl, f64 *wr, s32 rowlen, s32 iters,
 s32 vw_len, s32 wx_len, s32 wy_len, s32 orderm);


static void
gfquad(f64 *out, f64 *in, f64 *d, f64 *wl, f64 *wr,
		s32 veclen, s32 orderm, s32 wxlen, s32 wylen);

static void
make_l(f64 *vec, f64 *vl_r, f64 *vr_r, f64 *vl_w, f64 *vr_w,
		f64 wa, f64 wb, f64 dn, s32 vlen);

/* BEGIN PUBLIC MOLT ROUTINES */

/* do_sweep : sweeps iters times over rowlen strides of mesh data */
static void
do_sweep
(f64 *mesh, f64 *work, f64 *dvec, f64 *vl_r, f64 *vr_r,
 f64 *vl_w, f64 *vr_w, f64 *wl, f64 *wr, s32 rowlen, s32 iters,
 s32 vw_len, s32 wx_len, s32 wy_len, s32 orderm)
{
	f64 *ptr;
	f64 minval, minvalsq;
	f64 wa, wb;
	s32 i;

	// NOTE (brian) wa and wb both have to deal with boundary conditions and
	// as of writing (07/23/19), both are not used, and left at 0

	wa = 0;
	wb = 0;

	minval = minarr(vl_r, vw_len);

	// perform iters number of iterations
	for (i = 0, ptr = mesh; i < iters; i++, ptr += rowlen) {
		// ptr = mesh + (i * rowlen);
		memset(work, 0, sizeof(*work) * rowlen);
		gfquad(work, ptr, dvec, wl, wr, rowlen, orderm, wx_len, wy_len);
		make_l(work, vl_r, vr_r, vl_w, vr_w, wa, wb, minval, vw_len);
		memcpy(ptr, work, sizeof(*work) * rowlen);
	}
}

/* gfquad : performs green's function quadriture on the input vector */
static void
gfquad(f64 *out, f64 *in, f64 *d, f64 *wl, f64 *wr,
		s32 veclen, s32 orderm, s32 wxlen, s32 wylen)
{
	/* out and in's length is defined by hunklen */
	f64 IL, IR;
	s32 iL, iR, iC, M2, N;
	s32 i, bound;

	IL = 0;
	IR = 0;
	M2 = orderm / 2;
	N = veclen - 1;

	orderm++;
	iL = 0;
	iC = -M2;
	iR = veclen - orderm;

	for (i = 0; i < veclen; i++) { /* left */
		bound = gfquad_bound(i, M2, N);

		switch (bound) {
		case -1: // left bound
			IL = d[i] * IL + vect_mul(&wl[i * orderm], &in[iL], orderm);
			break;
		case 0: // center
			IL = d[i] * IL + vect_mul(&wl[i * orderm], &in[i + 1 + iC], orderm);
			break;
		case 1:
			IL = d[i] * IL + vect_mul(&wl[i * orderm], &in[iR], orderm);
			break;
		}

		out[i + 1] = out[i + 1] + IL;
	}

	for (i = veclen - 1; i >= 0; i--) { /* right */
		bound = gfquad_bound(i, M2, N);

		switch (bound) {
		case -1: // left bound
			IR = d[i] * IR + vect_mul(&wr[i * orderm], &in[iL], orderm);
			break;
		case 0: // center
			IR = d[i] * IR + vect_mul(&wr[i * orderm], &in[i + iC], orderm);
			break;
		case 1: // right
			IR = d[i] * IR + vect_mul(&wr[i * orderm], &in[iR], orderm);
			break;
		}

		out[i] = out[i] + IR;
	}
}

/* gfquad_bound : return easy bound parameters for gfquad */
static inline s32 gfquad_bound(s32 idx, s32 m2, s32 len)
{
	if (0 <= idx && idx <= 0 + m2) {
		return -1;
	}

	if (m2 + 1 <= idx && idx <= len - m2) {
		return 0;
	}

	if (len - m2 + 1 <= idx && idx <= len) {
		return 1;
	}

	assert(0);
	return 0;
}

/* make_l : applies dirichlet boundary conditions to the line in */
static void
make_l(f64 *vec, f64 *vl_r, f64 *vr_r, f64 *vl_w, f64 *vr_w,
		f64 wa, f64 wb, f64 dn, s32 vlen)
{
	/*
	 * make_l applies dirichlet boundary conditions to the line in place
	 *
	 * w = w + ((wa - w(1)) * (vL - dN * vR) + (wb - w(end)) * (vR - dN * vL))
	 *			/ (1 - dN ^ 2)
	 *
	 * because we need to compute the results of the inner vector expressions,
	 * and we still need the weights to be correct (vl and vr) from outside the
	 * function, pointers are suffixed with an _r and a _w to show which is the
	 * read and write pointers
	 *
	 * vec - place to write to
	 * vl_r, vr_r - vw weights to be read from
	 * vl_w, vr_w - working space the same size as vl_r and vr_r
	 * wa, wb     - scalars...(TODO Look Into This)
	 * dn         - minarr value from the weights (precompute)
	 * vlen       - length of all of the vectors
	 */

	f64 wa_use, wb_use, wc_use;

	// * wa - w(1)
	// * wb - w(end)
	// * 1 - dN ^ 2
	wa_use = wa - vec[0];
	wb_use = wb - vec[vlen - 1];
	wc_use = 1 - pow(dn, 2);

	// (vL - dN * vR)
	vec_mul_s(vr_w, vr_r, -1.0 * dn, vlen);
	vec_add_v(vl_w, vl_r,      vr_w, vlen);

	// (vR - dN * vL)
	vec_mul_s(vl_w, vl_r, -1.0 * dn, vlen);
	vec_add_v(vr_w, vr_r,      vl_w, vlen);

	// (wa - w(1)) * (vL - dN * vR)
	vec_mul_s(vl_w, vl_w, wa_use, vlen);

	// (wb - w(end)) * (vR - dN * vL)
	vec_mul_s(vr_w, vr_w, wb_use, vlen);

	// add the two computed vectors together
	vec_add_v(vr_w, vr_w, vl_w, vlen);

	// (...) / (1 - dN ^ 2) (current working answer is in vr_w)
	vec_mul_s(vr_w, vr_w, 1 / wc_use, vlen);

	// w = w + (...) / (...) (answer is still in vr_w)
	vec_add_v(vec, vr_w, vec, vlen);
}

/* mesh3_swap : reorganizes a 3d mesh */
static
void mesh3_swap(f64 *out, f64* in, ivec3_t *dim, cvec3_t from, cvec3_t to)
{
	/*
	 * mesh - pointer to the base mesh hunk
	 * dim  - 0 - x len, 1 - y len, 2 - z len
	 * curr - current ordering of the dimensions
	 * order- desired ordering of the dimensions
	 *
	 * this function will swap TWO of the THREE elements in our 3d mesh.
	 * careful bookkeeping for the functions that call this routine HAVE to be
	 * ensured.
	 * Keep in mind, this will be replaced by a GPU version that will (probably)
	 * kick this thing's butt, so it's almost lazy on purpose.
	 *
	 * While it might be slow, the function is generalized to work over any of
	 * the 3 possible swaps we might want, x <-> y, x <-> z, z <-> y. Over the
	 * loop of the three axis, a switch table is utilized to retrieve a pointer
	 * between our two elements, after which our swap is performed.
	 *
	 * TL;DR, I really hated writing this function. I think it should be better.
	 *
	 * WARNING : this WILL fail if the input and output pointers are the same
	 */

	u64 from_idx, to_idx;
	ivec3_t idx, ldim;

	// copy from our external dimensionality to our local one
	ldim[0] = (*dim)[0];
	ldim[1] = (*dim)[1];
	ldim[2] = (*dim)[2];

	// first, perform the matrix swaps that are required
	for (idx[2] = 0; idx[2] < ldim[2]; idx[2]++) {
		for (idx[1] = 0; idx[1] < ldim[1]; idx[1]++) {
			for (idx[0] = 0; idx[0] < ldim[0]; idx[0]++) { // for all of our things
				from_idx = mk_genericidx(idx, ldim, from);
				to_idx   = mk_genericidx(idx, ldim, to);
				out[to_idx] = in[from_idx];
			}
		}
	}

	// now ensure that our external dimensionality matches with
	// what we just swapped around
	meshdim_swap(dim, from, to);
}

/* meshdim_swap : swaps the dim values in dim from 'from' to 'to' */
static void meshdim_swap(ivec3_t *dim, cvec3_t from, cvec3_t to)
{
	s32 i, xlen, ylen, zlen;

	xlen = ylen = zlen = -1;

	// retrieve the dimensions into specific, known vars
	for (i = 0; i < 3; i++) {
		if (from[i] == 'x') { xlen = (*dim)[i]; }
		if (from[i] == 'y') { ylen = (*dim)[i]; }
		if (from[i] == 'z') { zlen = (*dim)[i]; }
	}

	// now put them back, with a similar technique to above
	for (i = 0; i < 3; i++) {
		if (to[i] == 'x') { (*dim)[i] = xlen; }
		if (to[i] == 'y') { (*dim)[i] = ylen; }
		if (to[i] == 'z') { (*dim)[i] = zlen; }
	}
}

/* mk_genericidx : retrieves a generic index from input dimensionality */
static inline
u64 mk_genericidx(ivec3_t inpts, ivec3_t dim, cvec3_t order)
{
	/*
	 * inpts and dim are assumed to come in with the structure:
	 *   - 0 -> x
	 *   - 1 -> y
	 *   - 2 -> z
	 *
	 * and the order vector is used to reorganize them to provide us with
	 * the output index
	 */

	s32 i;
	ivec3_t outpts, tmp;

	VectorCopy(dim, tmp);

	for (i = 0; i < 3; i++) {
		switch (order[i]) {
		case 'x':
			outpts[i] = inpts[0];
			tmp[i] = dim[0];
			break;
		case 'y':
			outpts[i] = inpts[1];
			tmp[i] = dim[1];
			break;
		case 'z':
			outpts[i] = inpts[2];
			tmp[i] = dim[2];
			break;
		}
	}

	return IDX3D(outpts[0], outpts[1], outpts[2], tmp[1], tmp[2]);
}

/* mk_genericdim : reorders dim in x,y,z to the 'to' specification */
static void mk_genericdim(ivec3_t *out, ivec3_t dim, cvec3_t to)
{
	s32 i;

	for (i = 0; i < 3; i++) {
		switch (to[i]) {
		case 'x':
			(*out)[i] = dim[0];
			break;
		case 'y':
			(*out)[i] = dim[1];
			break;
		case 'z':
			(*out)[i] = dim[2];
			break;
		}
	}
}

/* matrix_subtract : subtracts inb from ina storing in out */
static
void matrix_subtract(f64 *out, f64 *ina, f64 *inb, ivec3_t dim)
{
	ivec3_t idx;
	s32 i;

	for (idx[0] = 0; idx[0] < dim[0]; idx[0]++) {
		for (idx[1] = 0; idx[2] < dim[0]; idx[1]++) {
			for (idx[2] = 0; idx[2] < dim[0]; idx[2]++) {
				i = IDX3D(idx[0], idx[1], idx[2], dim[1], dim[2]);
				out[i] = ina[i] - inb[i];
			}
		}
	}
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

#endif

#endif // MOLT_IMPLEMENTATION

#endif // MOLT_H

