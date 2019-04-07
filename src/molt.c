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
 */

#include <stdio.h>

#include "config.h"
#include "common.h"
#include "calcs.h"
#include "molt.h"

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

static
void do_c_op(lcfg_t *cfg, lrun_t *run, lnu_t *nu, lvweight_t *vw,
			 lwweight_t *ww, lmesh_t *mesh);

static
void do_d_op(lcfg_t *cfg, lrun_t *run, lnu_t *nu, lvweight_t *vw,
			 lwweight_t *ww, lmesh_t *mesh);
static
void mesh3_swap(f64 *out, f64* in, ivec3_t dim, cvec3_t from, cvec3_t to);
static inline
u64 mk_genericidx(ivec3_t inpts, ivec3_t dim, cvec3_t order);

static
void do_sweep();


/* molt_firststep : specific routines required for the "first" timestep */
void molt_firststep(
		lcfg_t *cfg, lrun_t *run, lnu_t *nu, lvweight_t *vw,
		lwweight_t *ww, lmesh_t *mesh)
{
	s32 i;

	// u1 = 2 * (u0 + dt * v0)
	vec_mul_s(mesh->vmesh, mesh->vmesh, cfg->t_step, sizeof(mesh->vmesh));
	vec_add_v(mesh->umesh, mesh->umesh, mesh->vmesh, sizeof(mesh->vmesh));
	vec_mul_s(mesh->umesh, mesh->umesh, 2.0, sizeof(mesh->umesh));

	do_c_op(cfg, run, nu, vw, ww, mesh);
}

/* molt_step : regular timestepping routine */
void molt_step(
		lcfg_t *cfg, lrun_t *run, lnu_t *nu, lvweight_t *vw,
		lwweight_t *ww, lmesh_t *mesh)
{
}

/* do_c_op : The MOLT C Operator */
static
void do_c_op(lcfg_t *cfg, lrun_t *run, lnu_t *nu, lvweight_t *vw,
			 lwweight_t *ww, lmesh_t *mesh)
{
#if 0
	ivec3_t dim;

	dim[0] = cfg->x_points_inc;
	dim[1] = cfg->y_points_inc;
	dim[2] = cfg->z_points_inc;

	do_sweep();
	// subtract off the original u
	mesh3_swap(NULL, NULL, dim, swap_x_y_z, swap_y_x_z);
	do_sweep();
	mesh3_swap(NULL, NULL, dim, swap_y_x_z, swap_z_x_y);
	do_sweep();

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

	// add up all of the matricies

	// copy to the output
#endif
}

/* do_d_op : The MOLT D Operator */
static
void do_d_op(lcfg_t *cfg, lrun_t *run, lnu_t *nu, lvweight_t *vw,
			 lwweight_t *ww, lmesh_t *mesh)
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
static
void do_sweep()
{
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
	ivec3_t tmp;

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

