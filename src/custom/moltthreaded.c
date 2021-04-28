/*
 * Brian Chrzanowski
 * Mon Nov 04, 2019 22:31
 *
 * NOTE (brian)
 *
 * - We cheat a little in inialization. We already have working memory allocated
 *   by the main program with standard library malloc. In our molt_custom_t
 *   structure, we have a pointer to that config structure, which contains all
 *   of the working memory we'll need assuming we operate on the same local
 *   machine.
 *
 * TODO (brian)
 * 1. Cleanup function declarations (order, _t typing convention)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <math.h>

#include <pthread.h>
#include <dlfcn.h>

#include "../common.h"
#define MOLT_IMPLEMENTATION
#include "../molt.h"

#include "thpool.h"

// now we can actually define the functions that are going to be called
// from the molt module

#define THREADS 4

struct sweep_args_t {
	f64 *src;
	f64 *dst;
	f64 *work;

	f64 *vl;
	f64 *vr;
	f64 *wl;
	f64 *wr;

	f64 dnu;
	f64 minval;

	s64 rowlen;
	s32 orderm;
};

struct reorg_args_t {
	f64 *src;
	f64 *dst;
	f64 *work; // TODO (brian) do we need this work pointer?
	cvec3_t src_ord;
	cvec3_t dst_ord;
	ivec3_t dim;
	ivec3_t row;
};

static threadpool g_pool;
static struct sweep_args_t *g_sweepargs;
static struct reorg_args_t *g_reorgargs;

/* molt_custom_init : intializes the custom module */
int molt_custom_open(struct molt_custom_t *custom)
{
	int i;
	int a, b, c;
	s64 len;
	ivec3_t dim;

	// testing the threading pool
	g_pool = thpool_init(THREADS);

	// find the biggest dimension, and use that to know how many
	// sweep args we're going to need
	molt_cfg_parampull_xyz(custom->cfg, dim, MOLT_PARAM_PINC);

	a = INT_MIN;
	b = INT_MIN;
	c = INT_MIN;

	for (i = 0; i < 3; i++) {
		if (a < dim[i]) {
			a = dim[i];
		} else if (b < dim[i]) {
			b = dim[i];
		} else if (c < dim[i]) {
			c = dim[i];
		}
	}

	// a is the largest, followed by b, trailed by c
	len = a * b;

	g_sweepargs = calloc(len, sizeof(*g_sweepargs));
	g_reorgargs = calloc(len, sizeof(*g_reorgargs));

	// 0 on success
	return g_pool == NULL;
}

/* molt_custom_init : cleans up the custom module */
int molt_custom_close(struct molt_custom_t *custom)
{
	thpool_destroy(g_pool);

	free(g_sweepargs);

	return 0;
}

/* molt_custom_sweep_work : the work queue sweeping function */
void molt_custom_sweep_work(void *arg)
{
	struct sweep_args_t *sargs;

	// NOTE (brian)
	// this is exactly the same as the single threaded module's except for the
	// fact that it's setup to be called from the threadpool

	sargs = arg;

	molt_gfquad_m(sargs->work, sargs->src, sargs->dnu, sargs->wl, sargs->wr, sargs->rowlen, sargs->orderm);
	molt_makel(sargs->work, sargs->vl, sargs->vr, sargs->minval, sargs->rowlen);
	memcpy(sargs->dst, sargs->work, sizeof(f64) * sargs->rowlen);
}

/* molt_custom_sweep : performs a threaded sweep across the mesh in the dimension specified */
void molt_custom_sweep(f64 *dst, f64 *src, f64 *work, ivec3_t dim, cvec3_t ord, pdvec6_t params, dvec3_t dnu, s32 M)
{
	f64 *ptr, minval;
	f64 *wl, *wr;
	f64 *vl, *vr;
	f64 usednu;
	s64 rowlen, rownum, i;

	/*
	 * NOTE (brian)
	 * The dimensions of the 1st, 2nd, and 3rd dimensions are in dim, and
	 * their ordering in memory is provided in 'ord'.
	 *
	 * 'params' is the sweep parameters as setup in the C and D operators.
	 */

	for (i = 0; i < 3; i++)
		assert(ord[i] - 'x' < 3);

	// first, get the row's length
	rowlen = dim[ord[0] - 'x'];

	// then, get the number of rows
	// rownum = dim[1] * dim[2];
	rownum = dim[ord[1] - 'x'] * dim[ord[2] - 'x'];

	// get our weighting parameters all setup
	vl = params[0];
	vr = params[1];
	wl = params[2];
	wr = params[3];

	// find the minval (dN in Matlab)
	// NOTE (brian) this should have the same dimensionality all the time
	for (i = 0, minval = DBL_MAX; i < rowlen; i++) {
		if (vl[i] < minval)
			minval = vl[i];
	}

	usednu = dnu[ord[0] - 'x'];

	memset(work, 0, sizeof(f64) * rowlen * rownum);

	// walk through the volume, grabbing each row-organized value
	for (i = 0, ptr = src; i < rownum; i++, ptr += rowlen, work += rowlen) {
		// we setup our arguments for 'molt_sweep_custom_work'
		g_sweepargs[i].src = ptr;
		g_sweepargs[i].dst = ptr;
		g_sweepargs[i].work = work;
		g_sweepargs[i].vl = vl;
		g_sweepargs[i].vr = vr;
		g_sweepargs[i].wl = wl;
		g_sweepargs[i].wr = wr;
		g_sweepargs[i].rowlen = rowlen;
		g_sweepargs[i].orderm = M;
		g_sweepargs[i].dnu = usednu;

		// then we add it to the thread queue
		thpool_add_work(g_pool, molt_custom_sweep_work, g_sweepargs + i);
	}

	thpool_wait(g_pool);
}

/* molt_custom_reorg_work : worker function for threaded transpose */
void molt_custom_reorg_work(void *arg)
{
	struct reorg_args_t *reorgargs;
	s64 i;
	u64 src_i, dst_i;
	ivec3_t tmpv;

	/*
	 * NOTE (brian)
	 *
	 */

	reorgargs = arg;

	for (i = 0; i < reorgargs->dim[0]; i++) {
		Vec3Set(tmpv, i, reorgargs->row[1], reorgargs->row[2]);
		src_i = molt_genericidx(tmpv, reorgargs->dim, reorgargs->src_ord);
		dst_i = molt_genericidx(tmpv, reorgargs->dim, reorgargs->dst_ord);
		reorgargs->work[dst_i] = reorgargs->src[src_i];
	}
}

/* molt_custom_reorg : reorganizes a 3d mesh from src to dst */
void molt_custom_reorg(f64 *dst, f64 *src, f64 *work, ivec3_t dim, cvec3_t src_ord, cvec3_t dst_ord)
{
	/*
	 * NOTE (brian)
	 *
	 * This function is called in exactly the same way as the default
	 * 'molt_reorg' function. It differs, obviously, by creating threads
	 * to perform transposition one row at a time, much like the sweeping
	 * function.
	 *
	 * So,
	 *
	 * In this threaded module, we transpose "Y" by "Z" rows of "X",
	 * one row of "X" per thread. Keeping in mind that the values in src_ord
	 * and dst_ord may not be 'actual' x, y, or z.
	 */

	s64 i, j;

	memset(work, 0, sizeof(*work) * dim[0] * dim[1] * dim[2]);

	for (i = 0; i < dim[1]; i++) {
		for (j = 0; j < dim[2]; j++) {
			// we setup our arguments for 'molt_reorg_custom_work'
			g_reorgargs[i].src  = src;
			g_reorgargs[i].dst  = dst;
			g_reorgargs[i].work = work;
			Vec3Copy(g_reorgargs[i].src_ord, src_ord);
			Vec3Copy(g_reorgargs[i].dst_ord, dst_ord);
			Vec3Copy(g_reorgargs[i].dim, dim);
			Vec3Set(g_reorgargs[i].row, 0, i, j);

			// then we add it to the thread queue
			thpool_add_work(g_pool, molt_custom_reorg_work, g_reorgargs + i);
		}
	}

	thpool_wait(g_pool);

	memcpy(dst, work, sizeof(f64) * dim[0] * dim[1] * dim[2]);
}

