/*
 * Brian Chrzanowski
 * Fri Nov 01, 2019 11:50
 *
 * The MOLT Custom CUDA Implementation
 *
 * Documentation
 *
 * Ideally, the way this module would work is a user would be able to just
 * call a few functions, and bob's their uncle, they're off. That doesn't really
 * work that well because of what the single threaded implementation looks like.
 *
 * The implementation for the business logic, `sweep` and `reorg`, assume
 * "dst, src, and work" pointers to work over. They take those pointers as a
 * hunk of memory to sweep over, or to transpose, and are finished after that.
 * That's fine for a serial implementation on the CPU. Keeping in mind that
 * ideally, these pointers come from a larger hunk of contiguous memory, this
 * becomes a hard problem to deal with.
 *
 * So, I think the only reasonable thing to do is to add more hooks into the
 * init of a sweep
 *
 *
 * TODO
 * 1. create a mapping from source, host pointers, to device pointers
 *    as to know which parameters to use on the device's kernel launch
 */

#include <stdlib.h>
#include <string.h>

#include "../common.h"
#include "../molt.h"

struct molt_custom_t {
	// begin by defining the pointers we KNOW won't change
	f64 *vlx,   *vrx;
	f64 *vly,   *vry;
	f64 *vlz,   *vrz;
	f64 *d_vlx, *d_vrx;
	f64 *d_vly, *d_vry;
	f64 *d_vlz, *d_vrz;

	f64 *wlx,   *wrx;
	f64 *wly,   *wry;
	f64 *wlz,   *wrz;
	f64 *d_wlx, *d_wrx;
	f64 *d_wly, *d_wry;
	f64 *d_wlz, *d_wrz;

	// then the hard part...
	f64 *workstore[MOLT_WORKSTORE_AMT];
	f64 *worksweep;
};

static struct molt_custom_t g_custom;

/* molt_custom_init : intializes global custom info (g_custom) from cfg's values */
extern "C"
int molt_custom_init(struct molt_cfg_t *cfg)
{
	return 0;
}

/* molt_custom_free : frees all custom resources */
extern "C"
int molt_custom_free(struct molt_cfg_t *cfg)
{
	return 0;
}

extern "C"
int molt_custom_workstore_init(struct molt_cfg_t *cfg)
{
}

/* molt_custom_sweep : the custom cuda sweeping function */
extern "C"
void molt_custom_sweep(f64 *dst, f64 *src, f64 *work, ivec3_t dim, cvec3_t src_ord, pdvec6_t params, dvec3_t dnu, s32 M)
{
}

/* molt_custom_reorg : the custom cuda reorg function */
extern "C"
void molt_custom_reorg(f64 *dst, f64 *src, f64 *work, ivec3_t dim, cvec3_t src_ord, cvec3_t dst_ord)
{
}

