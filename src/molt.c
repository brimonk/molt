/*
 * Brian Chrzanowski
 * Sat Apr 06, 2019 22:21
 *
 * CPU Implementation of MOLT's Timestepping Features
 *
 * NOTE:
 *
 * It is consistently assumed that the dimensionality of multi-dimensional
 * arrays when the data comes in is row ordered by X, then Y, then Z. This means
 * that if we want to index into 5, 2, 3, to resolve the 1D index, we must
 * perform the IDX3D macro (in common.h).
 *
 * This isn't an issue when we operate over the X's of our data, meaning that we
 * only ever need to retrieve the data by the rows of Xs. However, we maximize
 * cache misses when we want to operate over a row in Y or a row in Z, as these
 * elements are each placed their width away from each other. The Ys are however
 * many y elements away from each other.
 *
 * However, the C and D operators, as defined in the paper proposed by Causley
 * and Christlieb require sweeps over x, y, and z. I therefore, propose that to
 * optimize the sweeping, we can reorganize the data within our mesh such that
 * the element we're currently operating over is within the mesg in row major
 * order.
 *
 * While hemming and hawing about how to do everything in place, it honestly
 * gets SO much easier using some extra space for these calculations. And
 * because it's 2019, memory is relatively cheap.
 *
 * TODO (Brian)
 * 1. Skeleton C and D Operators
 * 2. Skeleton Sweep Function
 * 3. Skeleton / Translate (Again) GF_QUAD
 * 4. Skeleton Weights Calculation
 * 5. Finish Writing the functions for time sweeping
 *
 * TODO CLEANUP:
 * 1. Remove stdlib headers and depencency
 * 2. In Place Mesh Reorganize (HARD)
 * 3. Make our generic index functions the same one
 * 4. Make function declaration not look like trash
 *
 * QUESTIONS:
 * 1. Should we use something other than vL for our weights all the time?
 */

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

struct work_t {
	f64 ix  [MOLT_X_POINTS_INC * MOLT_Y_POINTS_INC * MOLT_Z_POINTS_INC];
	f64 iy  [MOLT_X_POINTS_INC * MOLT_Y_POINTS_INC * MOLT_Z_POINTS_INC];
	f64 iz  [MOLT_X_POINTS_INC * MOLT_Y_POINTS_INC * MOLT_Z_POINTS_INC];
	f64 d1  [MOLT_X_POINTS_INC * MOLT_Y_POINTS_INC * MOLT_Z_POINTS_INC];
	f64 d2  [MOLT_X_POINTS_INC * MOLT_Y_POINTS_INC * MOLT_Z_POINTS_INC];
	f64 d3  [MOLT_X_POINTS_INC * MOLT_Y_POINTS_INC * MOLT_Z_POINTS_INC];
	f64 swap[MOLT_X_POINTS_INC * MOLT_Y_POINTS_INC * MOLT_Z_POINTS_INC];
};

typedef struct work_t work_t;

static struct work_t *workptr = NULL;

static cvec3_t swap_x_y_z = {'x', 'y', 'z'};
static cvec3_t swap_x_z_y = {'x', 'z', 'y'};
static cvec3_t swap_y_x_z = {'y', 'x', 'z'};
static cvec3_t swap_y_z_x = {'y', 'z', 'x'};
static cvec3_t swap_z_x_y = {'z', 'x', 'y'};
static cvec3_t swap_z_y_x = {'z', 'y', 'x'};

/*
 * both of the C and D operators as noted from above. they serve as the
 * bread and butter for molt's exponential recursive properties
 */

static void
do_c_op(lcfg_t *cfg, lnu_t *nu, lvweight_t *vw, lwweight_t *ww, lmesh_t *mesh);

/* do_d_op : The MOLT D Operator */
static void
do_d_op(lcfg_t *cfg, lnu_t *nu, lvweight_t *vw, lwweight_t *ww, lmesh_t *mesh);

static
void mesh3_swap(f64 *out, f64* in, ivec3_t dim, cvec3_t from, cvec3_t to);
static inline u64 mk_genericidx(ivec3_t inpts, ivec3_t dim, cvec3_t order);
static void mk_genericdim(ivec3_t *out, ivec3_t dim, cvec3_t to);
static s32 get_genericrowlen(ivec3_t dim, cvec3_t to);

static
void matrix_subtract(f64 *out, f64 *ina, f64 *inb, ivec3_t dim);
static f64 minarr(f64 *arr, s32 arrlen);
static f64 vect_mul(f64 *veca, f64 *vecb, s32 veclen);

/* molt_init and free : allocates and frees our working memory */
void molt_init() { workptr = malloc(sizeof(struct work_t)); assert(workptr); }
void molt_free() { if (workptr) free(workptr); }

/* molt_firststep : specific routines required for the "first" timestep */
void molt_firststep(
		lcfg_t *cfg, lnu_t *nu, lvweight_t *vw, lwweight_t *ww, lmesh_t *mesh)
{
	s32 i;

	// u1 = 2 * (u0 + dt * v0)
	vec_mul_s(mesh->vmesh, mesh->vmesh, cfg->t_step, sizeof(mesh->vmesh));
	vec_add_v(mesh->umesh, mesh->umesh, mesh->vmesh, sizeof(mesh->vmesh));
	vec_mul_s(mesh->umesh, mesh->umesh, 2.0, sizeof(mesh->umesh));

	do_c_op(cfg, nu, vw, ww, mesh);
}

/* molt_step : regular timestepping routine */
void
molt_step(lcfg_t *cfg, lnu_t *nu, lvweight_t *vw, lwweight_t *ww, lmesh_t *mesh)
{
}

/* do_c_op : The MOLT C Operator */
static void
do_c_op(lcfg_t *cfg, lnu_t *nu, lvweight_t *vw, lwweight_t *ww, lmesh_t *mesh)
{
	s64 totalelem;
	ivec3_t dim, iterinrow;

	dim[0] = cfg->x_points_inc;
	dim[1] = cfg->y_points_inc;
	dim[2] = cfg->z_points_inc;

	totalelem = dim[0] * dim[1] * dim[2];

	iterinrow[0] = totalelem / dim[0];
	iterinrow[1] = totalelem / dim[1];
	iterinrow[2] = totalelem / dim[2];

#if 0
	// sweeps in x, y, z
	memcpy(workptr->ix, mesh->umesh, sizeof(mesh->umesh));
	do_sweep(workptr->ix, cfg, nu, vw, ww, dim[0], iterinrow[0]);
	matrix_subtract(workptr->ix, workptr->ix, mesh->umesh, dim);
	mesh3_swap(workptr->swap, workptr->ix, dim, swap_x_y_z, swap_y_x_z);
	do_sweep(workptr->ix, cfg, nu, vw, ww, dim[1], iterinrow[0]);
	mesh3_swap(workptr->swap, workptr->ix, dim, swap_y_x_z, swap_z_x_y);
	do_sweep(workptr->swap, cfg, nu, vw, ww, dim[2], iterinrow[0]);
	memcpy(workptr->ix, workptr->swap, sizeof(workptr->swap));

	mesh3_swap(NULL, NULL, dim, swap_x_y_z, swap_y_x_z);
	do_sweep();
	// subtract off the original u
	mesh3_swap(NULL, NULL, dim, swap_y_x_z, swap_z_x_y);
	do_sweep();
	mesh3_swap(NULL, NULL, dim, swap_y_x_z, swap_z_x_y);
	do_sweep();

	mesh3_swap(NULL, NULL, dim, swap_x_y_z, swap_z_y_x);
	// subtract off the original u
	do_sweep();
	mesh3_swap(NULL, NULL, dim, swap_z_y_x, swap_x_y_z);
	do_sweep();
	mesh3_swap(NULL, NULL, dim, swap_x_y_z, swap_y_x_z);
	do_sweep();
#endif

	// add up all of the matricies

	// copy to the output
}

/* do_d_op : The MOLT D Operator */
static void
do_d_op(lcfg_t *cfg, lnu_t *nu, lvweight_t *vw, lwweight_t *ww, lmesh_t *mesh)
{
#if 0
	ivec3_t dim;

	dim[0] = cfg->x_points_inc;
	dim[1] = cfg->y_points_inc;
	dim[2] = cfg->z_points_inc;

	do_sweep();
	mesh3_swap(NULL, NULL, dim, swap_x_y_z, swap_y_x_z);
	do_sweep();
	mesh3_swap(NULL, NULL, dim, swap_y_x_z, swap_z_x_y);
	do_sweep();

	mesh3_swap(NULL, NULL, dim, swap_x_y_z, swap_y_x_z);
	do_sweep();
	mesh3_swap(NULL, NULL, dim, swap_y_x_z, swap_z_x_y);
	do_sweep();
	mesh3_swap(NULL, NULL, dim, swap_z_x_y, swap_x_z_y);
	do_sweep();

	mesh3_swap(NULL, NULL, dim, swap_x_y_z, swap_z_y_x);
	do_sweep();
	mesh3_swap(NULL, NULL, dim, swap_z_y_x, swap_x_y_z);
	do_sweep();
	mesh3_swap(NULL, NULL, dim, swap_x_y_z, swap_y_x_z);
	do_sweep();
#endif
}

/* do_sweep : does a sweep over the row-major data in the mesh */
static void
do_sweep(f64 *mesh, lcfg_t *cfg, lnu_t *nu, f64 *vw, f64 *ww,
		s32 rowlen, s32 iters, s32 nulen, s32 vwlen, s32 wwlen)
{
	f64 minval, minvalsq;
	s32 i;

	minval = minarr(vw, vwlen);

	// perform iters number of iterations
	for (i = 0; i < iters; i++) {
		// gfquad();
		// make_l();
	}
}

static void
gfquad(f64 *out, f64 *in, f64 *d, f64 *wl, f64 *wr,
		s32 hunklen, s32 orderm, s32 wxlen, s32 wylen)
{
	/* out and in's length is defined by hunklen */
	f64 IL, IR, M2;
	s32 iL, iR, iC;
	s32 i;

	IL = 0, IR = 0;
	M2 = orderm / 2;

	orderm++;
	iL = 0;
	iC = -M2;
	iR = hunklen - orderm;

	/* left sweep */
	for (i = 0; i < M2; i++) {
		IL = d[i] * IL + vect_mul(&wl[i * wxlen], &in[iL], orderm);
		out[i + 1] = out[i + 1] + iL;
	}

	for (; i < hunklen - M2; i++) {
		IL = d[i] * IL + vect_mul(&wl[i * wxlen], &in[i + iC], orderm);
		out[i + 1] = out[i + 1] + IL;
	}

	for (; i < hunklen; i++) {
		IL = d[i] * IL + vect_mul(&wl[i * wxlen], &in[iR], orderm);
		out[i + 1] = out[i + 1] + IL;
	}

	/* right sweep */
	for (i = hunklen - 1; i > hunklen - 1 - M2; i--) {
		IR = d[i] * IR + vect_mul(&wr[i * wxlen], &in[iR], orderm);
		out[i] = out[i] + IR;
	}

	for (; i > M2; i--) {
		IR = d[i] * IR + vect_mul(&wr[i * wxlen], &in[i + iC], orderm);
		out[i] = out[i] + IR;
	}

	for (; i >= 0; i--) {
		IR = d[i] * IR + vect_mul(&wr[i * wxlen], &in[iL], orderm);
		out[i] = out[i] + IR;
	}
}

static void
make_l(f64 *out, f64 *in, f64 *vl_r, f64 *vr_r, f64 *vl_w, f64 *vr_w,
		f64 wa, f64 wb, f64 dn, s32 vlen)
{
	/*
	 * make_l applies dirichlet boundary conditions to the line
	 *
	 * w = w + ((wa - w(1)) * (vL - dN * vR) + (wb - w(end)) * (vR - dN * vL))
	 *			/ (1 - dN ^ 2)
	 *
	 * because we need to compute the results of the inner vector expressions,
	 * and we still need the weights to be correct (vl and vr) from outside the
	 * function, pointers are suffixed with an _r and a _w to show which is the
	 * read and write pointers
	 *
	 * out - place to write to
	 * in  - place to write to
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
	wa_use = wa - in[0];
	wb_use = wb - in[vlen - 1];
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
	vec_add_v(out, vr_w, in, vlen);
}

/* mesh3_swap : reorganizes a 3d mesh */
static
void mesh3_swap(f64 *out, f64* in, ivec3_t dim, cvec3_t from, cvec3_t to)
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
	 * WARNING : this WILL fail if the input and output are the same
	 */

	u64 from_idx, to_idx;
	ivec3_t idx;

	for (idx[2] = 0; idx[2] < dim[2]; idx[2]++) {
		for (idx[1] = 0; idx[1] < dim[1]; idx[1]++) {
			for (idx[0] = 0; idx[0] < dim[0]; idx[0]++) { // for all of our things
				from_idx = mk_genericidx(idx, dim, from);
				to_idx   = mk_genericidx(idx, dim, to);
				out[to_idx] = in[from_idx];
			}
		}
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

#if 0
	printf("(%3d, %3d, %3d) -> (%3d, %3d, %3d)\n",
			inpts[0], inpts[1], inpts[2], outpts[0], outpts[1], outpts[2]);
#endif

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

static s32 get_genericrowlen(ivec3_t dim, cvec3_t to)
{
	switch (to[0]) {
	case 'x':
		return dim[0];
		break;
	case 'y':
		return dim[1];
		break;
	case 'z':
		return dim[2];
		break;
	default:
		assert(0); // incorrect usage
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
