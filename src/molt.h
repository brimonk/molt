#ifndef MOLT_H
#define MOLT_H

#include "common.h"

/*
 * Brian Chrzanowski
 * Sat Apr 06, 2019 22:21
 *
 * TODO Create a Single-Header file Lib (Brian)
 * 1. Initializer Functions for 
 * 2. Use CSTDLIB math.h? stdio.h? stdlib.h?
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

void molt_step(struct molt_cfg_t *cfg, f64 *next, f64 *curr, f64 *prev, pdvec3_t *nu, pdvec3_t *dnu, pdvec3_t *vw, pdvec3_t *ww);
void molt_step_vec(struct molt_cfg_t *cfg, pdvec3_t mesh, pdvec3_t nu, pdvec3_t dnu, pdvec3_t vw, pdvec3_t ww);

#ifdef MOLT_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include "config.h"
#include "common.h"
#include "calcs.h"
#include "molt.h"

static cvec3_t ord_x_y_z = {'x', 'y', 'z'};
static cvec3_t ord_x_z_y = {'x', 'z', 'y'};
static cvec3_t ord_y_x_z = {'y', 'x', 'z'};
static cvec3_t ord_y_z_x = {'y', 'z', 'x'};
static cvec3_t ord_z_x_y = {'z', 'x', 'y'};
static cvec3_t ord_z_y_x = {'z', 'y', 'x'};

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

/* molt_step : performs one molt timestep */
void molt_step(struct molt_cfg_t *cfg, f64 *next, f64 *curr, f64 *prev, pdvec3_t *nu, pdvec3_t *dnu, pdvec3_t *vw, pdvec3_t *ww)
{
}

void molt_step_vec(struct molt_cfg_t *cfg, pdvec3_t mesh, pdvec3_t nu, pdvec3_t dnu, pdvec3_t vw, pdvec3_t ww);

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

/* molt_firststep : specific routines required for the "first" timestep */
void molt_firststep(lmesh_t *dst, lmesh_t *src, struct cfg_t *cfg, lnu_t *nu,
		lvweight_t *vw, lwweight_t *ww)
{
	u64 totalelem;

	totalelem = (u64)cfg->x_points_inc * cfg->y_points_inc * cfg->z_points_inc;

	// u1 = 2 * (u0 + dt * v0)
	vec_mul_s(workp->swap, src->vmesh, cfg->t_step * cfg->int_scale, totalelem);
	vec_add_v(workp->swap, workp->swap, src->umesh, totalelem);
	vec_mul_s(workp->swap, workp->swap,        2.0, totalelem);
	memcpy(dst->umesh, workp->swap, sizeof(workp->swap));

	do_c_op(workp->d1, dst->umesh, cfg, nu, vw, ww);

	// u1 = u1 + beta ^ 2 * d1
	vec_mul_s(workp->swap, workp->d1, cfg->beta_sq, totalelem);
	vec_add_v(dst->umesh, workp->swap, dst->umesh, totalelem);

	// macros for less compiled operations
#if MOLT_TIMEACC >= 2
	do_d_op(workp->d2, workp->d1, cfg, nu, vw, ww);
	do_d_op(workp->d1, workp->d1, cfg, nu, vw, ww);

	// u1 = u1 - beta ^ 2 * d2 + beta ^ 4 / 12 * d1
	vec_mul_s(workp->swap, workp->d2, cfg->beta_sq, totalelem);
	vec_mul_s(workp->temp, workp->d1, cfg->beta_fo, totalelem);
	vec_add_v(workp->swap, workp->swap, workp->temp, totalelem);
	vec_sub_v(dst->umesh, dst->umesh, workp->swap, totalelem);

#elif MOLT_TIMEACC >= 3
	do_d_op(workp->d3, workp->d2, cfg, nu, vw, ww);
	do_d_op(workp->d2, workp->d1, cfg, nu, vw, ww);
	do_d_op(workp->d1, workp->d1, cfg, nu, vw, ww);

	// u1 = u1 + (beta^2 * d3 - 2 * beta^4/12 * d2 + beta^6/360 * d1)
	vec_mul_s(workp->swap, workp->d3, cfg->beta_sq, totalelem);
	vec_mul_s(workp->temp, workp->d2, cfg->beta_fo, totalelem);
	vec_sub_v(workp->swap, workp->swap, workp->temp, totalelem);
	vec_mul_s(workp->temp, workp->d1, cfg->beta_si, totalelem);
	vec_add_v(workp->swap, workp->swap, workp->temp, totalelem);
	vec_add_v(dst->umesh, dst->umesh, workp->swap, totalelem);
#endif

	// u1 = u1 / 2
	vec_mul_s(dst->umesh, dst->umesh, 1 / 2, totalelem);
}

/* molt_step : regular timestepping routine */
void molt_step(lmesh_t *dst, lmesh_t *src,
		struct cfg_t *cfg, lnu_t *nu, lvweight_t *vw, lwweight_t *ww)
{
	s64 totalelem;

	totalelem = cfg->x_points_inc * cfg->y_points_inc * cfg->z_points_inc;

	memset(dst->umesh, 0, sizeof(dst->umesh));

	do_c_op(workp->d1, src->umesh, cfg, nu, vw, ww);

	// u = u + beta^2 * d1
	vec_mul_s(workp->swap, workp->d1, cfg->beta_sq, totalelem);
	vec_add_v(dst->umesh, dst->umesh, workp->swap, totalelem);

#if MOLT_TIMEACC >= 2
	do_d_op(workp->d2, workp->d1, cfg, nu, vw, ww);
	do_d_op(workp->d1, workp->d1, cfg, nu, vw, ww);

	// u = u - beta^2 * d2 + beta^4/12 * d1
	vec_mul_s(workp->swap, workp->d2, cfg->beta_sq, totalelem);
	vec_mul_s(workp->temp, workp->d1, cfg->beta_fo, totalelem);
	vec_add_v(workp->swap, workp->swap, workp->temp, totalelem);
	vec_sub_v(dst->umesh, dst->umesh, workp->swap, totalelem);
#elif MOLT_TIMEACC >= 3
	do_d_op(workp->d3, workp->d2, cfg, nu, vw, ww);
	do_d_op(workp->d2, workp->d1, cfg, nu, vw, ww);
	do_d_op(workp->d1, workp->d1, cfg, nu, vw, ww);

	// u1 = u1 + (beta^2 * d3 - 2 * beta^4/12 * d2 + beta^6/360 * d1)
	vec_mul_s(workp->swap, workp->d3, cfg->beta_sq, totalelem);
	vec_mul_s(workp->temp, workp->d2, cfg->beta_fo, totalelem);
	vec_sub_v(workp->swap, workp->swap, workp->temp, totalelem);
	vec_mul_s(workp->temp, workp->d1, cfg->beta_si, totalelem);
	vec_add_v(workp->swap, workp->swap, workp->temp, totalelem);
	vec_add_v(dst->umesh, dst->umesh, workp->swap, totalelem);
#endif

	// u = u + 2 * u1 - u0 (u0 is our vmesh from src)
	vec_mul_s(workp->swap, src->umesh, 2, totalelem);
	vec_sub_v(workp->swap, workp->swap, src->vmesh, totalelem);
	vec_add_v(dst->umesh, dst->umesh, workp->swap, totalelem);
}

/* do_c_op : The MOLT C Operator */
static void do_c_op(
f64 *dst, f64 *src, struct cfg_t *cfg, lnu_t *nu, lvweight_t *vw, lwweight_t *ww)
{
	u64 totalelem;
	ivec3_t dim, iterinrow;
	s32 wy; // ww in the "y" dimension is always the same

	dim[0] = cfg->x_points_inc;
	dim[1] = cfg->y_points_inc;
	dim[2] = cfg->z_points_inc;

	totalelem = dim[0] * dim[1] * dim[2];

	iterinrow[0] = totalelem / dim[0];
	iterinrow[1] = totalelem / dim[1];
	iterinrow[2] = totalelem / dim[2];

	wy = cfg->space_acc + 1;

	// FIRST SWEEP IN X, Y, Z --> BEGIN
	memcpy(workp->ix, src, sizeof(workp->ix));
	do_sweep(workp->ix, workp->quad_x, nu->dnux, 
			vw->vlx, vw->vrx, workp->vlx_w, workp->vrx_w,
			ww->xl_weight, ww->xr_weight, dim[0], iterinrow[0],
			dim[0], cfg->x_points, wy, cfg->space_acc);

	// subtract u from the resultant matrix
	matrix_subtract(workp->ix, workp->ix, src, dim);
	mesh3_swap(workp->swap, workp->ix, &dim, swap_x_y_z, swap_y_x_z);
	memcpy(workp->ix, workp->swap, sizeof(workp->ix));

	do_sweep(workp->ix, workp->quad_y, nu->dnuy, 
			vw->vly, vw->vry, workp->vly_w, workp->vry_w,
			ww->yl_weight, ww->yr_weight, dim[1], iterinrow[1],
			dim[1], cfg->y_points, wy, cfg->space_acc);

	mesh3_swap(workp->swap, workp->ix, &dim, swap_y_x_z, swap_z_x_y);
	memcpy(workp->ix, workp->swap, sizeof(workp->ix));

	do_sweep(workp->ix, workp->quad_z, nu->dnuz, 
			vw->vlz, vw->vrz, workp->vlz_w, workp->vrz_w,
			ww->zl_weight, ww->yr_weight, dim[2], iterinrow[2],
			dim[2], cfg->z_points, wy, cfg->space_acc);
	mesh3_swap(workp->swap, workp->ix, &dim, swap_z_x_y, swap_x_y_z);
	memcpy(workp->ix, workp->swap, sizeof(workp->ix));
	// SWEEP IN X, Y, Z --> END

	// SWEEP IN Y, Z, X --> BEGIN
	mesh3_swap(workp->iy, src, &dim, swap_x_y_z, swap_y_x_z);

	do_sweep(workp->iy, workp->quad_y, nu->dnuy, 
			vw->vly, vw->vry, workp->vly_w, workp->vry_w,
			ww->yl_weight, ww->yr_weight, dim[1], iterinrow[1],
			dim[1], cfg->y_points, wy, cfg->space_acc);

	mesh3_swap(workp->swap, workp->iy, &dim, swap_y_x_z, swap_x_y_z);
	matrix_subtract(workp->iy, workp->swap, src, dim);

	mesh3_swap(workp->swap, workp->iy, &dim, swap_x_y_z, swap_z_x_y);
	memcpy(workp->iy, workp->swap, sizeof(workp->iy));

	do_sweep(workp->iz, workp->quad_z, nu->dnuz, 
			vw->vlz, vw->vrz, workp->vlz_w, workp->vrz_w,
			ww->zl_weight, ww->yr_weight, dim[2], iterinrow[2],
			dim[2], cfg->z_points, wy, cfg->space_acc);

	mesh3_swap(workp->swap, workp->iy, &dim, swap_z_x_y, swap_x_y_z);
	memcpy(workp->iy, workp->swap, sizeof(workp->ix));

	do_sweep(workp->iz, workp->quad_x, nu->dnux, 
			vw->vlx, vw->vrx, workp->vlx_w, workp->vrx_w,
			ww->xl_weight, ww->xr_weight, dim[0], iterinrow[0],
			dim[0], cfg->z_points, wy, cfg->space_acc);
	// SWEEP IN Y, Z, X --> END

	// SWEEP IN Z, X, Y --> END
	mesh3_swap(workp->iz, src, &dim, swap_x_y_z, swap_z_x_y);
	do_sweep(workp->iz, workp->quad_z, nu->dnuz, 
			vw->vlz, vw->vrz, workp->vlz_w, workp->vrz_w,
			ww->zl_weight, ww->zr_weight, dim[2], iterinrow[2],
			dim[2], cfg->z_points, wy, cfg->space_acc);
	matrix_subtract(workp->iz, workp->iz, src, dim);

	mesh3_swap(workp->swap, workp->iz, &dim, swap_z_x_y, swap_x_z_y);
	memcpy(workp->iz, workp->swap, sizeof(workp->iz));

	do_sweep(workp->iz, workp->quad_x, nu->dnux, 
			vw->vlx, vw->vrx, workp->vlx_w, workp->vrx_w,
			ww->xl_weight, ww->xr_weight, dim[0], iterinrow[0],
			dim[0], cfg->x_points, wy, cfg->space_acc);

	mesh3_swap(workp->swap, workp->iy, &dim, swap_x_z_y, swap_y_z_x);
	memcpy(workp->iz, workp->swap, sizeof(workp->iz));

	do_sweep(workp->iz, workp->quad_x, nu->dnux, 
			vw->vlx, vw->vrx, workp->vlx_w, workp->vrx_w,
			ww->xl_weight, ww->xr_weight, dim[0], iterinrow[0],
			dim[0], cfg->z_points, wy, cfg->space_acc);
	mesh3_swap(workp->swap, workp->iy, &dim, swap_y_z_x, swap_x_y_z);
	memcpy(workp->iz, workp->swap, sizeof(workp->iz));

	memset(dst, 0, sizeof(workp->ix));
	vec_add_v(dst, dst, workp->ix, totalelem);
	vec_add_v(dst, dst, workp->iy, totalelem);
	vec_add_v(dst, dst, workp->iz, totalelem);
}

/* do_d_op : The MOLT D Operator */
static void do_d_op(
f64 *dst, f64 *src, struct cfg_t *cfg, lnu_t *nu, lvweight_t *vw, lwweight_t *ww)
{
	s64 totalelem;
	ivec3_t dim, iterinrow;
	s32 orderm, wy; // ww in the "y" dimension is always the same

	dim[0] = cfg->x_points_inc;
	dim[1] = cfg->y_points_inc;
	dim[2] = cfg->z_points_inc;

	totalelem = dim[0] * dim[1] * dim[2];

	iterinrow[0] = totalelem / dim[0];
	iterinrow[1] = totalelem / dim[1];
	iterinrow[2] = totalelem / dim[2];

	wy = cfg->space_acc + 1;

	// SWEEP IN X, Y, Z --> BEGIN
	memcpy(workp->ix, src, sizeof(workp->ix));
	do_sweep(workp->ix, workp->quad_x, nu->dnux, 
			vw->vlx, vw->vrx, workp->vlx_w, workp->vrx_w,
			ww->xl_weight, ww->xr_weight, dim[0], iterinrow[0],
			dim[0], cfg->x_points, wy, cfg->space_acc);

	mesh3_swap(workp->swap, workp->ix, &dim, swap_x_y_z, swap_y_x_z);
	memcpy(workp->ix, workp->swap, sizeof(workp->ix));

	do_sweep(workp->ix, workp->quad_y, nu->dnuy, 
			vw->vly, vw->vry, workp->vly_w, workp->vry_w,
			ww->yl_weight, ww->yr_weight, dim[1], iterinrow[1],
			dim[1], cfg->y_points, wy, cfg->space_acc);

	mesh3_swap(workp->swap, workp->ix, &dim, swap_y_x_z, swap_z_x_y);
	memcpy(workp->ix, workp->swap, sizeof(workp->ix));

	do_sweep(workp->ix, workp->quad_z, nu->dnuz, 
			vw->vlz, vw->vrz, workp->vlz_w, workp->vrz_w,
			ww->zl_weight, ww->zr_weight, dim[2], iterinrow[2],
			dim[2], cfg->z_points, wy, cfg->space_acc);
	mesh3_swap(workp->swap, workp->ix, &dim, swap_z_x_y, swap_x_y_z);
	memcpy(workp->ix, workp->swap, sizeof(workp->ix));
	// SWEEP IN X, Y, Z --> END

	// SWEEP IN Y, Z, X --> BEGIN
	mesh3_swap(workp->iy, src, &dim, swap_x_y_z, swap_y_x_z);
	do_sweep(workp->iy, workp->quad_y, nu->dnuy, 
			vw->vly, vw->vry, workp->vly_w, workp->vry_w,
			ww->yl_weight, ww->yr_weight, dim[1], iterinrow[1],
			dim[1], cfg->y_points, wy, cfg->space_acc);

	mesh3_swap(workp->swap, workp->iy, &dim, swap_x_y_z, swap_z_x_y);
	memcpy(workp->iy, workp->swap, sizeof(workp->iy));

	do_sweep(workp->iy, workp->quad_z, nu->dnuz, 
			vw->vlz, vw->vrz, workp->vlz_w, workp->vrz_w,
			ww->zl_weight, ww->zr_weight, dim[2], iterinrow[2],
			dim[2], cfg->z_points, wy, cfg->space_acc);

	mesh3_swap(workp->swap, workp->iy, &dim, swap_z_x_y, swap_x_y_z);
	memcpy(workp->iy, workp->swap, sizeof(workp->iy));

	do_sweep(workp->iy, workp->quad_x, nu->dnux, 
			vw->vlx, vw->vrx, workp->vlx_w, workp->vrx_w,
			ww->xl_weight, ww->xr_weight, dim[0], iterinrow[0],
			dim[0], cfg->z_points, wy, cfg->space_acc);
	// SWEEP IN Y, Z, X --> END

	// SWEEP IN Z, X, Y --> END
	mesh3_swap(workp->iz, src, &dim, swap_x_y_z, swap_z_x_y);

	do_sweep(workp->iz, workp->quad_z, nu->dnuz, 
			vw->vlz, vw->vrz, workp->vlz_w, workp->vrz_w,
			ww->zl_weight, ww->zr_weight, dim[2], iterinrow[2],
			dim[2], cfg->z_points, wy, cfg->space_acc);

	mesh3_swap(workp->swap, workp->iz, &dim, swap_z_x_y, swap_x_z_y);
	memcpy(workp->iz, workp->swap, sizeof(workp->iz));

	do_sweep(workp->iz, workp->quad_x, nu->dnux, 
			vw->vlx, vw->vrx, workp->vlx_w, workp->vrx_w,
			ww->xl_weight, ww->xr_weight, dim[0], iterinrow[0],
			dim[0], cfg->x_points, wy, cfg->space_acc);

	mesh3_swap(workp->swap, workp->iz, &dim, swap_x_z_y, swap_y_z_x);
	memcpy(workp->iz, workp->swap, sizeof(workp->iz));

	do_sweep(workp->iz, workp->quad_y, nu->dnuy, 
			vw->vly, vw->vry, workp->vly_w, workp->vry_w,
			ww->yl_weight, ww->yr_weight, dim[0], iterinrow[0],
			dim[0], cfg->y_points, wy, cfg->space_acc);

	mesh3_swap(workp->swap, workp->iy, &dim, swap_y_z_x, swap_x_y_z);
	memcpy(workp->iz, workp->swap, sizeof(workp->iz));

	memset(dst, 0, sizeof(workp->ix));
	vec_add_v(dst, dst, workp->ix, totalelem);
	vec_add_v(dst, dst, workp->iy, totalelem);
	vec_add_v(dst, dst, workp->iz, totalelem);

	vec_mul_s(dst, dst, 1 / 3, totalelem);
	vec_sub_v(dst, dst, src, totalelem);
}

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

	// TODO (brian) do these values need to be anything special
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

