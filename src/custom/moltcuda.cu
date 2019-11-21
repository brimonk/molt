/*
 * Brian Chrzanowski
 * Tue Nov 19, 2019 21:02
 *
 * NOTE (brian)
 *
 * - The threaded implementation can rely on memory where the CUDA model
 *   can't _really_. IE, the working storage allocated by "molt.h" won't
 *   do our graphics card any good. Because the library (even the
 *   custom routine) expects that it will have the result of a given operation,
 *   reorg or step in the destination hunk of memory, we'll have to copy it
 *   to and from the graphics card.
 *
 *   There are some optimizations that we can do though.
 *
 *   1) We don't ever expect the values for the v and w weights to ever change.
 *      So, we can allocate storage for those at the beginning of the journey,
 *      and keep those allocated until the library calls "molt_custom_close".
 *
 *   2) Unfortunately, the library expects it to be able to give us source
 *      and destination memory hunks as it needs to. Because of this,
 *      the custom implementation can't just keep the problem state on-device
 *      the entire time, as that would require an equivalent amount of
 *      allocations to the host device.
 *
 *      To get around this, on every custom library call (reorg and sweep), we
 *      can compare the work, dst, and src pointers to what we got "last" time.
 *      If they're different, we need to copy from host to device, to update
 *      what the device is using to perform the operation.
 *
 * TODO (brian)
 * 1. Test with copying from host to device and host to device every time
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <math.h>

#include "../common.h"
#define MOLT_IMPLEMENTATION
#include "../molt.h"

struct molt_custommod_t {
	f64 *d_src, *d_work, *d_dst;
	f64 *l_src, *l_dst; // the last pointers we've seen

	struct molt_cfg_t config;

	f64 *d_v[6];
	f64 *d_w[6];
	f64 *h_v[6];
	f64 *h_w[6];
};

static struct molt_custommod_t g_mod;

/* alloc_and_copy : allocs space on device, copies 'size' bytes from host */
int alloc_and_copy(f64 **d, f64 **newh, f64 *oldh, size_t size);
/* copy_if_needed : copies from host to device if needed, returns device ptr */
f64 *copy_if_needed(f64 *device, f64 *last_host, f64 *curr_host, size_t size);

/* cuda_gfquad_m : green's function quadriture on the input vector (CUDA) */
__device__ void cuda_gfquad_m(f64 *dst, f64 *src, f64 dnu, f64 *wl, f64 *wr, s64 len, s32 M);

/* molt_makel : applies dirichlet boundary conditions to the line in (CUDA) */
__device__ void cuda_makel(f64 *src, f64 *vl, f64 *vr, f64 minval, s64 len);

/* cuda_sweep : the cuda parallel'd sweeping function */
__global__ void cuda_sweep(f64 *dst, f64 *src, f64 *vl, f64 *vr, f64 *wl, f64 *wr, f64 minval, f64 dnu, s32 M, ivec3_t dim);

/* mk_genericidx : retrieves a generic index from input dimensionality */
__device__ u64 cuda_genericidx(ivec3_t ival, ivec3_t idim, cvec3_t order);

/* cuda_reorg : the cuda parallel'd transposition function */
__global__ void cuda_reorg(f64 *dst, f64 *src, f64 *work, ivec3_t dim, cvec3_t src_ord, cvec3_t dst_ord);

/* alloc_and_copy : allocs space on device, copies 'size' bytes from host */
int alloc_and_copy(f64 **d, f64 **newh, f64 *oldh, size_t size)
{
	cudaError_t err;

	err = cudaMalloc((void **)d, size);
	if (err != cudaSuccess) {
		return -1;
	}

	err = cudaMemcpy((void *)*d, (void *)oldh, size, cudaMemcpyHostToDevice);
	if (err != cudaSuccess) {
		return -1;
	}

	// save the host pointer for later use
	*newh = oldh;

	return 0;
}

/* copy_if_needed : if needed, updates host pointer and data */
f64 *copy_if_needed(f64 *device, f64 **last_host, f64 *curr_host, size_t size)
{
	if ((*last_host) != curr_host) {
		*last_host = curr_host;
		cudaMemcpy(device, curr_host, size, cudaMemcpyHostToDevice);
	}

	return device;
}

/* molt_custom_init : intializes the custom module */
extern "C" __declspec(dllexport)
int molt_custom_open(struct molt_custom_t *custom)
{
	u64 elements;
	struct molt_cfg_t *cfg;
	cudaError_t err;
	ivec3_t pinc;
	ivec3_t points;
	int rc;

	memset(&g_mod, 0, sizeof(struct molt_custommod_t));

	// snag a copy of the config structure we'll use
	// thoughout the module's lifetime
	memcpy(&g_mod.config, custom->cfg, sizeof(struct molt_cfg_t));

	cfg = &g_mod.config;

	molt_cfg_parampull_xyz(cfg, pinc, MOLT_PARAM_PINC);
	molt_cfg_parampull_xyz(cfg, points, MOLT_PARAM_POINTS);
	elements = pinc[0] * (u64)pinc[1] * pinc[2];

	rc = alloc_and_copy(&g_mod.d_v[0], &g_mod.h_v[0], custom->vlx, points[0] * sizeof(f64));
	if (rc < 0) { return -1; }
	rc = alloc_and_copy(&g_mod.d_v[1], &g_mod.h_v[1], custom->vrx, points[0] * sizeof(f64));
	if (rc < 0) { return -1; }
	rc = alloc_and_copy(&g_mod.d_v[2], &g_mod.h_v[2], custom->vly, points[1] * sizeof(f64));
	if (rc < 0) { return -1; }
	rc = alloc_and_copy(&g_mod.d_v[3], &g_mod.h_v[3], custom->vry, points[1] * sizeof(f64));
	if (rc < 0) { return -1; }
	rc = alloc_and_copy(&g_mod.d_v[4], &g_mod.h_v[4], custom->vlz, points[2] * sizeof(f64));
	if (rc < 0) { return -1; }
	rc = alloc_and_copy(&g_mod.d_v[5], &g_mod.h_v[5], custom->vrz, points[2] * sizeof(f64));
	if (rc < 0) { return -1; }

	rc = alloc_and_copy(&g_mod.d_w[0], &g_mod.h_w[0], custom->wlx, cfg->x_params[MOLT_PARAM_POINTS] * (cfg->spaceacc + 1) * sizeof(f64));
	if (rc < 0) { return -1; }
	rc = alloc_and_copy(&g_mod.d_w[1], &g_mod.h_w[1], custom->wrx, cfg->x_params[MOLT_PARAM_POINTS] * (cfg->spaceacc + 1) * sizeof(f64));
	if (rc < 0) { return -1; }
	rc = alloc_and_copy(&g_mod.d_w[2], &g_mod.h_w[2], custom->wly, cfg->y_params[MOLT_PARAM_POINTS] * (cfg->spaceacc + 1) * sizeof(f64));
	if (rc < 0) { return -1; }
	rc = alloc_and_copy(&g_mod.d_w[3], &g_mod.h_w[3], custom->wry, cfg->y_params[MOLT_PARAM_POINTS] * (cfg->spaceacc + 1) * sizeof(f64));
	if (rc < 0) { return -1; }
	rc = alloc_and_copy(&g_mod.d_w[4], &g_mod.h_w[4], custom->wlz, cfg->z_params[MOLT_PARAM_POINTS] * (cfg->spaceacc + 1) * sizeof(f64));
	if (rc < 0) { return -1; }
	rc = alloc_and_copy(&g_mod.d_w[5], &g_mod.h_w[5], custom->wrz, cfg->z_params[MOLT_PARAM_POINTS] * (cfg->spaceacc + 1) * sizeof(f64));
	if (rc < 0) { return -1; }

	// because we don't need to copy from the host, we'll just use cuda funcs
	err = cudaMalloc(&g_mod.d_src, elements * sizeof(f64));
	if (err != cudaSuccess) { return -1; }
	err = cudaMalloc(&g_mod.d_dst, elements * sizeof(f64));
	if (err != cudaSuccess) { return -1; }
	err = cudaMalloc(&g_mod.d_work, elements * sizeof(f64));
	if (err != cudaSuccess) { return -1; }

	return 0;
}

/* molt_custom_close : cleans up the custom module */
extern "C" __declspec(dllexport)
int molt_custom_close(struct molt_custom_t *custom)
{
	int i;

	for (i = 0; i < 6; i++) {
		cudaFree(g_mod.d_v[i]);
		cudaFree(g_mod.d_w[i]);
	}

	cudaFree(g_mod.d_src);
	cudaFree(g_mod.d_dst);
	cudaFree(g_mod.d_work);

	memset(&g_mod, 0, sizeof(g_mod));

	return 0;
}

/* cuda_vect_mul : perform element-wise vector multiplication */
__device__
f64 cuda_vect_mul(f64 *veca, f64 *vecb, s32 veclen)
{
	f64 val;
	s32 i;

	for (val = 0, i = 0; i < veclen; i++) {
		val += veca[i] * vecb[i];
	}

	return val;
}


/* cuda_gfquad_m : green's function quadriture on the input vector (CUDA) */
__device__ void cuda_gfquad_m(f64 *dst, f64 *src, f64 dnu, f64 *wl, f64 *wr, s64 len, s32 M)
{
	/* out and in's length is defined by hunklen */
	f64 IL, IR;
	s32 iL, iR, iC, M2, N;
	s32 i;

	IL = 0;
	IR = 0;
	M2 = M / 2;
	N = len - 1;

	M++;

	iL = 0;
	iC = -M2;
	iR = len - M;

	/* left sweep */
	for (i = 0; i < M2; i++) {
		IL = dnu * IL + cuda_vect_mul(&wl[i * M] , &src[iL], M);
		dst[i + 1] = dst[i + 1] + IL;
	}

	for (; i < N - M2; i++) {
		IL = dnu * IL + cuda_vect_mul(&wl[i * M], &src[i + 1 + iC], M);
		dst[i + 1] = dst[i + 1] + IL;
	}

	for (; i < N; i++) {
		IL = dnu * IL + cuda_vect_mul(&wl[i * M], &src[iR], M);
		dst[i + 1] = dst[i + 1] + IL;
	}

	/* right sweep */
	for (i = N - 1; i > N - 1 - M2; i--) {
		IR = dnu * IR + cuda_vect_mul(&wr[i * M], &src[iR], M);
		dst[i] = dst[i] + IR;
	}

	for (; i >= M2; i--) {
		IR = dnu * IR + cuda_vect_mul(&wr[i * M], &src[i + 1 + iC], M);
		dst[i] = dst[i] + IR;
	}

	for (; i >= 0; i--) {
		IR = dnu * IR + cuda_vect_mul(&wr[i * M], &src[iL], M);
		dst[i] = dst[i] + IR;
	}

	// I = I / 2
	for (i = 0; i < len; i++)
		dst[i] /= 2;
}

/* molt_makel : applies dirichlet boundary conditions to the line in (CUDA) */
__device__ void cuda_makel(f64 *src, f64 *vl, f64 *vr, f64 minval, s64 len)
{
	/*
	 * molt_makel applies dirichlet boundary conditions to the line in place
	 *
	 * Executes this:
	 * w = w + ((wa - w(1)) * (vL - dN * vR) + (wb - w(end)) * (vR - dN * vL))
	 *			/ (1 - dN ^ 2)
	 *
	 * NOTE(s)
	 * wa and wb are left here as const scalars for future expansion of boundary
	 * conditions.
	 */

	f64 wa_use, wb_use, wc_use;
	f64 val;
	s64 i;

	const f64 wa = 0;
	const f64 wb = 0;

	// * wa_use - w(1)
	// * wb_use - w(end)
	// * wc_use - 1 - dN ^ 2
	wa_use = wa - src[0];
	wb_use = wb - src[len - 1];
	wc_use = 1 - pow(minval, 2);

	for (i = 0; i < len; i++) {
		val  = wa_use * vl[i] - minval * vr[i];
		val += wb_use * vr[i] - minval * vl[i];
		val /= wc_use;
		src[i] += val;
	}
}

/* cuda_sweep : the cuda parallel'd sweeping function */
__global__ void cuda_sweep(f64 *dst, f64 *src, f64 *vl, f64 *vr, f64 *wl, f64 *wr, f64 minval, f64 dnu, s32 M, ivec3_t dim)
{
	/*
	 * NOTE (brian)
	 * This function, while idiomatic CUDA, might seem a bit weird. Because
	 * launching the kernel in higher dimensions is, honestly, difficult to
	 * think about, I chose to solve the slightly easier problem, that is,
	 * launch the kernel in "a single dimension", with "Y by Z (dim[1] * dim[2])
	 * threads, then use our single dimension launch parameters to determine
	 * how far we're into the volume, using the IDX3D macro.
	 */

	u64 y, z, i;

	// first, get our thread number (thread 0, thread 1, thread 500, etc)
	i = threadIdx.x + blockDim.x * blockIdx.x;

	// use our volume dimensionality to determine the REAL y and z values from that
	// this assumes that we think about the problem in a "2D" sense
	y = i % dim[1];
	z = i / dim[1];

	i = IDX3D(0, y, z, dim[1], dim[2]);

	// don't perform the computation if we're out of range
	if (((u64)dim[0] * dim[1] * dim[2]) < i) {
		return;
	}

	// now that we have this thread's starting point, perform the algorithm
	// on this thread, for this row in x
	cuda_gfquad_m(dst + i, src + i, dnu, wl, wr, dim[0], M);
	cuda_makel(dst + i, vl, vr, minval, dim[0]);
}

/* molt_custom_sweep : performs a threaded sweep across the mesh in the dimension specified */
extern "C" __declspec(dllexport)
void molt_custom_sweep(f64 *dst, f64 *src, f64 *work, ivec3_t dim, cvec3_t ord, pdvec6_t params, dvec3_t dnu, s32 M)
{
	/*
	 * NOTE (brian)
	 * Because of the interface being defined how it is, we first have to
	 * create our mapping from host pointers, to dst pointers.
	 *
	 * We don't care about what the work pointer is because the device has to
	 * have its own working memory anyways, AND we're just going to memset it
	 * to 0 to begin with anyways.
	 *
	 * TODO (brian)
	 * - it might be worth making these function return ints and checking for
	 *   errors. Ideally, these won't error, but I suppose you wouldn't
	 *   know until you checked..
	 */

	f64 *d_src, *d_work, *d_dst;
	f64 *d_vl, *d_vr, *d_wl, *d_wr;
	f64 *h_vl, *h_vr, *h_wl, *h_wr;
	f64 usednu, minval;
	u64 elements, i;
	size_t bytes;
	dim3 block, grid;

	elements = (u64)dim[0] * dim[1] * dim[2];
	bytes = elements * sizeof(f64);

	// copy the bytes from host to device if the pointers have changed
	// since last time
	d_src  = copy_if_needed(g_mod.d_src, &g_mod.l_src, src, bytes);
	d_dst  = copy_if_needed(g_mod.d_dst, &g_mod.l_dst, dst, bytes);
	d_work = g_mod.d_work;

	// find our v and w weights on the device
	h_vl = params[0];
	h_vr = params[1];
	h_wl = params[2];
	h_wr = params[3];
	d_vl = NULL;
	d_vr = NULL;
	d_wl = NULL;
	d_wr = NULL;

	for (i = 0; i < 6; i++) {
		if (!d_vl && g_mod.h_v[i] == h_vl) {
			d_vl = g_mod.d_v[i];
		}
		if (!d_vr && g_mod.h_v[i] == h_vr) {
			d_vr = g_mod.d_v[i];
		}
		if (!d_wl && g_mod.h_w[i] == h_wl) {
			d_wl = g_mod.d_w[i];
		}
		if (!d_wr && g_mod.h_w[i] == h_wr) {
			d_wr = g_mod.d_w[i];
		}
	}

	// find the minval (dN in Matlab)
	// NOTE (brian) this should have the same dimensionality all the time
	for (i = 0, minval = DBL_MAX; i < dim[0]; i++) {
		if (h_vl[i] < minval)
			minval = h_vl[i];
	}

	// determine the correct dnu to use
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
	default: // assert here? it's an illegal parameter
		break;
	}

	cudaMemset(d_work, 0, bytes);

	u64 threads, blocks, iterations;

	iterations = dim[1] * dim[2];
	threads = 256;
	blocks = (iterations + threads - 1) / threads;

	// Launch kernel with dimensionality Y by Z, to sweep through the volume in a plane.
	// init dimensionality dim3s, launch our kernel, then wait for the sync
	cuda_sweep<<<blocks, threads>>>(d_dst, d_src, d_vl, d_vr, d_wl, d_wr, minval, usednu, M, dim);
	cudaDeviceSynchronize();

	// copy from device to host, so the library's expectations are met
	cudaMemcpy((void *)dst, (void *)d_dst, bytes, cudaMemcpyDeviceToHost);
}

/* cuda_genericidx : retrieves a generic index from input dimensionality */
__device__ u64 cuda_genericidx(ivec3_t ival, ivec3_t idim, cvec3_t order)
{
	/*
	 * NOTE (brian)
	 * This is just a copy, for the CUDA device, of the library function
	 * of a similar name.
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

/* cuda_reorg : the cuda parallel'd transposition function */
__global__ void cuda_reorg(f64 *dst, f64 *src, f64 *work, ivec3_t dim, cvec3_t src_ord, cvec3_t dst_ord)
{
	u64 src_i, dst_i;
	ivec3_t curr;

	curr[0] = threadIdx.x + blockDim.x * blockIdx.x;
	curr[1] = threadIdx.y + blockDim.y * blockIdx.y;
	curr[2] = threadIdx.z + blockDim.z * blockIdx.z;

	src_i = cuda_genericidx(curr, dim, src_ord);
	dst_i = cuda_genericidx(curr, dim, src_ord);

	dst[dst_i] = src[src_i];
}

/* molt_custom_reorg : reorganizes a 3d mesh from src to dst */
extern "C" __declspec(dllexport)
void molt_custom_reorg(f64 *dst, f64 *src, f64 *work, ivec3_t dim, cvec3_t src_ord, cvec3_t dst_ord)
{
	f64 *d_src, *d_work, *d_dst;
	u64 elements;
	dim3 grid, block;
	size_t bytes;

	elements = (u64)dim[0] * dim[1] * dim[2];
	bytes = elements * sizeof(f64);

	d_src = copy_if_needed(g_mod.d_src, &g_mod.l_src, src, bytes);
	// d_dst = copy_if_needed(g_mod.d_dst, &g_mod.l_src, src, bytes);
	d_dst  = g_mod.d_src;
	d_work = g_mod.d_work;

	cudaMemset(d_work, 0, bytes);

	// Unlike the cuda sweep, where we launch the kernel in a
	// "single dimension", we launch this one 3 dimensions.
	block = dim3(64, 64, 64);
	grid = dim3(ceil(dim[0] / block.x), ceil(dim[1] / block.y), ceil(dim[2] / block.z));

	cuda_reorg<<<grid, block>>>(d_dst, d_src, d_work, dim, src_ord, dst_ord);

	cudaDeviceSynchronize();

	cudaMemcpy((void *)dst, (void *)d_dst, bytes, cudaMemcpyDeviceToHost);
}

