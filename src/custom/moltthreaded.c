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
#include <assert.h>
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

static threadpool g_pool;
static struct sweep_args_t *g_sweepargs;

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

	printf("len is : %ld\n", len);

	g_sweepargs = calloc(len, sizeof(*g_sweepargs));

	printf("%s\n", __FUNCTION__);

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
	int rc;

	/*
	 * NOTE (brian)
	 * The dimensions of the 1st, 2nd, and 3rd dimensions are in dim, and
	 * their ordering in memory is provided in 'ord'.
	 *
	 * 'params' is the sweep parameters as setup in the C and D operators.
	 */

	// first, get the row's length
	rowlen = dim[0];

	// then, get the number of rows
	rownum = dim[1] * dim[2];

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

	// then figure out the correct dnu to use
	switch(ord[0]) {
	case 'x':
		usednu = dnu[0];
		break;
	case 'y':
		usednu = dnu[1];
		break;
	case 'z':
		usednu = dnu[2];
		break;
	default:
		assert(0);
		break;
	}

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
		rc = thpool_add_work(g_pool, molt_custom_sweep_work, g_sweepargs + i);

		assert(rc == 0);
	}

	thpool_wait(g_pool);
}

/* molt_custom_reorg : reorganizes a 3d mesh from src to dst */
void molt_custom_reorg(f64 *dst, f64 *src, f64 *work, ivec3_t dim, cvec3_t src_ord, cvec3_t dst_ord)
{
	/*
	 * arguments
	 * dst - destination of reorg
	 * src - source of reorg
	 * work - working storage for swap around (avoids same pointer problem)
	 * dim - dimensionality (dim[0] -> X, dim[1] -> Y, dim[2] -> Z)
	 * src_ord - the ordering of src
	 * dst_ord - the ordering of dsr
	 */

	// we're using the default reorg for now
	// I don't know if the overhead of threading will actually make this faster
	molt_reorg(dst, src, work, dim, src_ord, dst_ord);
}

// COPIES OF THE SINGLE-THREADED IMPLEMENTATION

