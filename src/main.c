/*
 * Brian Chrzanowski
 * Sun Jan 20, 2019 14:43
 *
 * Causley, Christlieb, et al.
 *
 * Method Of Lines Transpose - Implicit Wave Solver Implementation
 *
 * DOCUMENTATION
 * This File:
 *   1. Initializes Storage through the io_* routines
 *   2. Initializes problem state through the setuplump_* routines
 *   3. Kicks off MOLT Timesteps
 *   4. Synchronizes after those timesteps
 *
 * WARNINGS
 * 1. We blast over whatever file you give it with the new simulation data.
 *
 * TODO (Brian)
 * 1. Run a single simulation all the way through
 * 2. Clean up how we load and begin simulations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MOLT_IMPLEMENTATION
#include "molt.h"

#include "io.h"
#include "calcs.h"
#include "config.h"
#include "common.h"
#include "viewer.h"

/* lump setup functions */
void setup_simulation(void **base, u64 *size, int fd);
u64 setup_lumps(void *base);
void setuplump_cfg(struct lump_header_t *hdr, struct molt_cfg_t *cfg);
void setuplump_run(struct lump_runinfo_t *run);
void setuplump_efield(struct lump_header_t *hdr, struct lump_efield_t *efield);
void setuplump_pfield(struct lump_header_t *hdr, struct lump_pfield_t *pfield);
void setuplump_vweight(struct lump_header_t *hdr, struct lump_vweight_t *vw);
void setuplump_wweight(struct lump_header_t *hdr, struct lump_wweight_t *ww);
void setuplump_umesh(struct lump_header_t *hdr, struct lump_mesh_t *state);

void do_simulation(void *hunk, u64 hunksize);

void setupstate_print(void *hunk);
s32 lump_magiccheck(void *hunk);

void print_help(char *prog);

#define USAGE "USAGE : %s [--viewer] [-v] [-h] outfile\n"

#define FLG_VERBOSE 0x01
#define FLG_VIEWER  0x02
#define FLG_SIM     0x04

int main(int argc, char **argv)
{
	void *hunk;
	char *fname, *s, **targv, targc;
	u64 hunksize;
	s32 fd, longopt;
	u32 flags;
	struct molt_cfg_t *cfg;
	struct run_t *run;

	flags = 0, targc = argc, targv = argv;

	flags |= FLG_SIM;

	// parse arguments
	while (--targc > 0 && (*++targv)[0] == '-') {
		for (s = targv[0] + 1, longopt = 0; *s && !longopt; s++) {

			// parse long options first to avoid getting all illegal options
			if (strcmp(s, "-viewer") == 0) {
				flags |= FLG_VIEWER; longopt = 1; continue;
			}

			if (strcmp(s, "-nosim") == 0) {
				flags &= ~FLG_SIM; longopt = 1; continue;
			}

			switch (*s) {
				case 'v':
					flags |= FLG_VERBOSE;
					break;
				case 'h':
					print_help(argv[0]);
					return 1;
				default:
					fprintf(stderr, "Illegal Option %c\n", *s);
					targc = 0;
					break;
			}
		}
	}

	if (targc != 1) {
		fprintf(stderr, USAGE, argv[0]);
		return 1;
	}

	fname = targv[0];

	// create our disk-backed storage
	fd = io_open(fname);

	// figure out if our disk-backed store is already good enough
	// WARN (brian) not robust
	hunksize = sizeof(struct lump_header_t);

	if (hunksize < io_getsize()) {
		hunksize = io_getsize();
		hunk = io_mmap(fd, hunksize);
	} else {
		io_resize(fd, hunksize);
		// mind the reader, setup_simulation remmaps the hunk into vmemory
		// after figuring out how big it is
		hunk = io_mmap(fd, hunksize);
		setup_simulation(&hunk, &hunksize, fd);
		io_mssync(hunk, hunk, hunksize);
	}

	if (flags & FLG_SIM) {
		// do_simulation(hunk, hunksize);
		io_mssync(hunk, hunk, hunksize);
	}

	if (flags & FLG_VERBOSE) {
		lump_magiccheck(hunk);
		setupstate_print(hunk);
	}

	if (flags & FLG_VIEWER) {
		// cfg = io_lumpgetbase(hunk, MOLTLUMP_CONFIG);
		cfg = NULL;
		viewer_run(hunk, hunksize, fd, cfg);
	}

	io_munmap(hunk);
	io_close(fd);

	return 0;
}

/* do_simulation : actually does the simulating */
void do_simulation(void *hunk, u64 hunksize)
{
	struct molt_cfg_t *cfg;
	struct lump_runinfo_t *run;
	struct lump_vweight_t *vw_lump;
	struct lump_wweight_t *ww_lump;
	struct lump_vmesh_t *vmesh;
	struct lump_umesh_t *umesh, *curr;

	u32 firststep_flg, normal_flg;

	pdvec3_t vol;
	pdvec6_t nu, vw, ww;

	cfg     = io_lumpgetbase(hunk, MOLTLUMP_CONFIG);
	run     = io_lumpgetbase(hunk, MOLTLUMP_RUNINFO);
	vw_lump = io_lumpgetbase(hunk, MOLTLUMP_VWEIGHT);
	ww_lump = io_lumpgetbase(hunk, MOLTLUMP_WWEIGHT);
	vmesh   = io_lumpgetbase(hunk, MOLTLUMP_VMESH);
	umesh   = io_lumpgetbase(hunk, MOLTLUMP_UMESH);

	vw[0] = vw_lump->vlx;
	vw[1] = vw_lump->vrx;
	vw[2] = vw_lump->vly;
	vw[3] = vw_lump->vry;
	vw[4] = vw_lump->vlz;
	vw[5] = vw_lump->vrz;

	ww[0] = ww_lump->xl_weight;
	ww[1] = ww_lump->xr_weight;
	ww[2] = ww_lump->yl_weight;
	ww[3] = ww_lump->yr_weight;
	ww[4] = ww_lump->zl_weight;
	ww[5] = ww_lump->zr_weight;

	// we have to set the first step flag, for caring about initial conditions
	firststep_flg = MOLT_FLAG_FIRSTSTEP;
	normal_flg = 0;

	// setup the volume info for the FIRST step
	// 0 - next, 1 - curr, 2 - prev
	vol[MOLT_VOL_NEXT] = (umesh + 1)->data;
	vol[MOLT_VOL_CURR] = umesh->data;
	vol[MOLT_VOL_PREV] = vmesh->data;

	molt_step3v(cfg, vol, vw, ww, firststep_flg);

	for (run->t_idx = 1; run->t_idx < run->t_total; run->t_idx++) {
		curr = umesh + run->t_idx;

		// ensure that vol is filled with the correct parameters
		vol[MOLT_VOL_NEXT] = (curr + 1)->data;
		vol[MOLT_VOL_CURR] = (curr    )->data;
		vol[MOLT_VOL_PREV] = (curr - 1)->data;

		molt_step3v(cfg, vol, vw, ww, normal_flg);

		// save off whatever fields are required
	}

	// TODO (brian) move somewhere else
	molt_cfg_free_workstore(cfg);
}

/* setup_simulation : sets up simulation based on config.h */
void setup_simulation(void **base, u64 *size, int fd)
{
	void *newblk;
	u64 oldsize;

	io_lumpcheck(*base);

	oldsize = *size;
	*size = setup_lumps(*base);

	/* resize the file (if needed) to get enough simulation space */
	if (io_getsize() < *size) {
		if (io_resize(fd, *size) < 0) {
			fprintf(stderr, "Couldn't get space. Quitting\n");
			exit(1);
		}
	}

	/* remmap it to the correct size */
	if ((newblk = io_mremap(*base, oldsize, *size)) == ((void *)-1)) {
		fprintf(stderr, "ERR: Couldn't remap the file to the correct size!\n");
		exit(1);
	} else {
		*base = newblk;
	}

	printf("file size %ld\n", *size);

	setuplump_cfg(*base, io_lumpgetbase(*base, MOLTLUMP_CONFIG));
	setuplump_run(io_lumpgetbase(*base, MOLTLUMP_RUNINFO));
	setuplump_efield(*base, io_lumpgetbase(*base, MOLTLUMP_EFIELD));
	setuplump_pfield(*base, io_lumpgetbase(*base, MOLTLUMP_PFIELD));
	setuplump_vweight(*base, io_lumpgetbase(*base, MOLTLUMP_VWEIGHT));
	setuplump_wweight(*base, io_lumpgetbase(*base, MOLTLUMP_WWEIGHT));
	setuplump_umesh(*base, io_lumpgetbase(*base, MOLTLUMP_UMESH));
}

/* setup_lumps : returns size of file after lumpsystem setup */
u64 setup_lumps(void *base)
{
	struct lump_header_t *hdr;
	s32 curr_lump;
	u64 curr_offset;

	/* begin the setup by setting up our lump header and our lump directory */

	hdr = base;
	curr_offset = sizeof(struct lump_header_t);

	/* setup the lump header with the little run-time data we have */
	curr_lump = MOLTLUMP_CONFIG;
	hdr->lump[curr_lump].offset = curr_offset;
	hdr->lump[curr_lump].elemsize = sizeof(struct molt_cfg_t);
	hdr->lump[curr_lump].lumpsize = 1 * hdr->lump[curr_lump].elemsize;
	curr_offset += hdr->lump[curr_lump].lumpsize;

	/* setup our runtime info lump */
	curr_lump = MOLTLUMP_RUNINFO;
	hdr->lump[curr_lump].offset = curr_offset;
	hdr->lump[curr_lump].elemsize = sizeof(struct lump_runinfo_t);
	hdr->lump[curr_lump].lumpsize = 1 * hdr->lump[curr_lump].elemsize;
	curr_offset += hdr->lump[curr_lump].lumpsize;

	/* setup our vweight information */
	curr_lump = MOLTLUMP_VWEIGHT;
	hdr->lump[curr_lump].offset = curr_offset;
	hdr->lump[curr_lump].elemsize = sizeof(struct lump_vweight_t);
	hdr->lump[curr_lump].lumpsize = hdr->lump[curr_lump].elemsize;
	curr_offset += hdr->lump[curr_lump].lumpsize;

	/* setup our wweight information */
	curr_lump = MOLTLUMP_WWEIGHT;
	hdr->lump[curr_lump].offset = curr_offset;
	hdr->lump[curr_lump].elemsize = sizeof(struct lump_wweight_t);
	hdr->lump[curr_lump].lumpsize = hdr->lump[4].elemsize;
	curr_offset += hdr->lump[curr_lump].lumpsize;

	/* setup our efield information */
	curr_lump = MOLTLUMP_EFIELD;
	hdr->lump[curr_lump].offset = curr_offset;
	hdr->lump[curr_lump].elemsize = sizeof(struct lump_efield_t);
	hdr->lump[curr_lump].lumpsize = ((u64)MOLT_T_POINTS) * hdr->lump[curr_lump].elemsize;
	curr_offset += hdr->lump[curr_lump].lumpsize;

	/* setup our pfield information */
	curr_lump = MOLTLUMP_PFIELD;
	hdr->lump[curr_lump].offset = curr_offset;
	hdr->lump[curr_lump].elemsize = sizeof(struct lump_pfield_t);
	hdr->lump[curr_lump].lumpsize = ((u64)MOLT_T_POINTS) * hdr->lump[curr_lump].elemsize;
	curr_offset += hdr->lump[curr_lump].lumpsize;

	/* the wave initial conditions */
	curr_lump = MOLTLUMP_VMESH;
	hdr->lump[curr_lump].offset = curr_offset;
	hdr->lump[curr_lump].elemsize = sizeof(struct lump_vmesh_t);
	hdr->lump[curr_lump].lumpsize = hdr->lump[curr_lump].elemsize;
	curr_offset += hdr->lump[curr_lump].lumpsize;

	/* the 3d volume lump */
	curr_lump = MOLTLUMP_UMESH;
	hdr->lump[curr_lump].offset = curr_offset;
	hdr->lump[curr_lump].elemsize = sizeof(struct lump_umesh_t);
	hdr->lump[curr_lump].lumpsize = ((u64)MOLT_T_PINC) * hdr->lump[curr_lump].elemsize;
	curr_offset += hdr->lump[curr_lump].lumpsize;

	return curr_offset;
}

/* simsetup_cfg : setup the config lump */
// void setuplump_cfg(struct molt_cfg_t *cfg, int argc, char **argv)
void setuplump_cfg(struct lump_header_t *hdr, struct molt_cfg_t *cfg)
{
	memset(cfg, 0, sizeof *cfg);

	molt_cfg_set_spacescale(cfg, MOLT_SPACESCALE);
	molt_cfg_set_timescale(cfg, MOLT_TIMESCALE);

	molt_cfg_dims_t(cfg, MOLT_T_START, MOLT_T_STOP, MOLT_T_STEP, MOLT_T_POINTS, MOLT_T_PINC);
	molt_cfg_dims_x(cfg, MOLT_X_START, MOLT_X_STOP, MOLT_X_STEP, MOLT_X_POINTS, MOLT_X_PINC);
	molt_cfg_dims_y(cfg, MOLT_Y_START, MOLT_Y_STOP, MOLT_Y_STEP, MOLT_Y_POINTS, MOLT_Y_PINC);
	molt_cfg_dims_z(cfg, MOLT_Z_START, MOLT_Z_STOP, MOLT_Z_STEP, MOLT_Z_POINTS, MOLT_Z_PINC);

	molt_cfg_set_accparams(cfg, MOLT_SPACEACC, MOLT_TIMEACC);

	// compute alpha by hand because it relies on simulation specific params
	// (beta is set by the previous function)
	// TODO (brian) can we get alpha setting into the library?
	cfg->alpha = cfg->beta /
		(MOLT_TISSUESPEED * cfg->t_params[MOLT_PARAM_STEP] * cfg->time_scale);

	molt_cfg_set_nu(cfg);

	// setup working storage by virtue of the library
	molt_cfg_set_workstore(cfg);
}

/* simsetup_run : setup run information */
void setuplump_run(struct lump_runinfo_t *run)
{
	memset(run, 0, sizeof *run);

	run->meta.magic = MOLTLUMP_MAGIC;
	run->t_start = 0;
	run->t_step  = .5;
	run->t_stop  = 10;
	run->t_idx   = 0;
	// We only take T_POINTS steps. That's because we need the extra step from
	// MOLT_T_PINC as the "firststep" mesh storage
	run->t_total = MOLT_T_POINTS;
}

/* setuplump_efield : setup the efield lump */
void setuplump_efield(struct lump_header_t *hdr, struct lump_efield_t *efield)
{
	struct lump_t *lump;
	s32 i;

	lump = &hdr->lump[MOLTLUMP_EFIELD];
	memset(efield, 0, lump->lumpsize);

	// set ALL of the efield meta tags
	for (i = 0; i < MOLT_T_POINTS; i++) {
		(efield + i)->meta.magic = MOLTLUMP_MAGIC;
	}
}

/* setuplump_pfield : setup the pfield lump */
void setuplump_pfield(struct lump_header_t *hdr, struct lump_pfield_t *pfield)
{
	struct lump_t *lump;
	s32 i;

	lump = &hdr->lump[MOLTLUMP_PFIELD];
	memset(pfield, 0, lump->lumpsize);

	// set ALL of the pfield meta tags
	for (i = 0; i < MOLT_T_POINTS; i++) {
		(pfield + i)->meta.magic = MOLTLUMP_MAGIC;
	}
}

/* setuplump_vweight : setup the vweight lump */
void setuplump_vweight(struct lump_header_t *hdr, struct lump_vweight_t *vw)
{
	struct molt_cfg_t *cfg;
	s64 i;
	ivec3_t dim;

	memset(vw, 0, sizeof *vw);

	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);

	vw->meta.magic = MOLTLUMP_MAGIC;

	molt_cfg_parampull_xyz(cfg, dim, MOLT_PARAM_PINC);

	for (i = 0; i < dim[0]; i++) // vlx
		vw->vlx[i] = exp((-cfg->alpha) * cfg->space_scale * i);

	for (i = 0; i < dim[0]; i++) // vrx
		vw->vrx[i] = exp((-cfg->alpha) * cfg->space_scale * i);

	for (i = 0; i < dim[1]; i++) // vly
		vw->vly[i] = exp((-cfg->alpha) * cfg->space_scale * i);

	for (i = 0; i < dim[1]; i++) // vry
		vw->vry[i] = exp((-cfg->alpha) * cfg->space_scale * i);

	for (i = 0; i < dim[2]; i++) // vlz
		vw->vrz[i] = exp((-cfg->alpha) * cfg->space_scale * i);

	for (i = 0; i < dim[2]; i++) // vrz
		vw->vlz[i] = exp((-cfg->alpha) * cfg->space_scale * i);
}

/* setuplump_wweight : setup the wweight lump */
void setuplump_wweight(struct lump_header_t *hdr, struct lump_wweight_t *ww)
{
	struct molt_cfg_t *cfg;

	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);

	ww->meta.magic = MOLTLUMP_MAGIC;

	// perform all of our get_exp_weights
	get_exp_weights(cfg->nu[0], ww->xl_weight, ww->xr_weight, cfg->x_params[MOLT_PARAM_POINTS], cfg->spaceacc);
	get_exp_weights(cfg->nu[1], ww->yl_weight, ww->yr_weight, cfg->y_params[MOLT_PARAM_POINTS], cfg->spaceacc);
	get_exp_weights(cfg->nu[2], ww->zl_weight, ww->zr_weight, cfg->z_params[MOLT_PARAM_POINTS], cfg->spaceacc);
}

void setuplump_vmesh(struct lump_header_t *hdr, struct lump_vmesh_t *vmesh)
{
	// NOTE (brian) not used, initial wave velocity setup
	vmesh->meta.magic = MOLTLUMP_MAGIC;
}

f64 initial_scale(struct molt_cfg_t *cfg, f64 v, f64 vlen)
{
	return 2 * (cfg->space_scale * v) / (cfg->space_scale * vlen) - 1;
}

/* setuplump_umesh : setup the initial conditions for the (3d) volume */
void setuplump_umesh(struct lump_header_t *hdr, struct lump_mesh_t *state)
{
	// write zeroes to the umesh
	s32 x, y, z, i, xpinc, ypinc, zpinc, xpoints, ypoints, zpoints;
	struct molt_cfg_t *cfg;
	struct lump_t *lump;
	struct lump_umesh_t *umesh;
	struct lump_vmesh_t *vmesh;
	f64 fx, fy, fz;

	lump = &hdr->lump[MOLTLUMP_UMESH];
	umesh = io_lumpgetbase(hdr, MOLTLUMP_UMESH);
	vmesh = io_lumpgetbase(hdr, MOLTLUMP_VMESH);
	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);

	memset(state, 0, lump->lumpsize);

	vmesh->meta.magic = MOLTLUMP_MAGIC;
	for (i = 0; i < lump->lumpsize / lump->elemsize; i++) {
		umesh[i].meta.magic = MOLTLUMP_MAGIC;
	}

	xpinc   = cfg->x_params[MOLT_PARAM_PINC];
	ypinc   = cfg->y_params[MOLT_PARAM_PINC];
	zpinc   = cfg->y_params[MOLT_PARAM_PINC];
	xpoints = cfg->x_params[MOLT_PARAM_POINTS];
	ypoints = cfg->y_params[MOLT_PARAM_POINTS];
	zpoints = cfg->z_params[MOLT_PARAM_POINTS];

	// we only want to setup the first (0th) umesh applies function f
	for (z = 0; z < zpinc; z++) {
		for (y = 0; y < ypinc; y++) {
			for (x = 0; x < xpinc; x++) {
				i = IDX3D(x, y, z, ypinc, zpinc);

				fx = pow(initial_scale(cfg, x, xpoints), 2);
				fy = pow(initial_scale(cfg, y, ypoints), 2);
				fz = pow(initial_scale(cfg, z, zpoints), 2);

				umesh->data[i] = exp(-13.0 * (fx + fy + fz));
				vmesh->data[i] = MOLT_TISSUESPEED * 2 * 13.0 * 2 / (xpoints * cfg->space_scale) * initial_scale(cfg, x, xpoints) * umesh->data[i];
			}
		}
	}
}

/* setupstate_print : perform lots of printf debugging */
void setupstate_print(void *hunk)
{
	struct lump_header_t *hdr;
	struct molt_cfg_t *cfg;
	struct lump_nu_t *nu;
	struct lump_vweight_t *vw;
	struct lump_wweight_t *ww;
	struct lump_vmesh_t *vmesh;
	struct lump_umesh_t *umesh;
	s32 i;
	ivec2_t xweight_dim, yweight_dim, zweight_dim;
	ivec3_t umesh_dim, vmesh_dim;
	char buf[BUFLARGE];

	hdr = hunk;

	cfg = io_lumpgetbase(hunk, MOLTLUMP_CONFIG);

	/* print out lump & storage information */
	printf("header : 0x%8x\tver : %d\t type : 0x%02x\n", hdr->meta.magic, hdr->meta.version, hdr->meta.type);
	for (i = 0; i < MOLTLUMP_TOTAL; i++) {
		printf("lump[%d] off : 0x%08lx\tlumpsz : 0x%08lx\telemsz : 0x%08lx\t"
				"cnt : %ld\n",
			i,
			hdr->lump[i].offset,
			hdr->lump[i].lumpsize,
			hdr->lump[i].elemsize,
			hdr->lump[i].lumpsize / hdr->lump[i].elemsize);
	}
	printf("\n");

	// log all of the vweights
	vw = io_lumpgetbase(hunk, MOLTLUMP_VWEIGHT);
	LOG1D(vw->vlx, cfg->x_params[MOLT_PARAM_PINC], "VWEIGHT-VLX");
	LOG1D(vw->vrx, cfg->x_params[MOLT_PARAM_PINC], "VWEIGHT-VRX");
	LOG1D(vw->vly, cfg->y_params[MOLT_PARAM_PINC], "VWEIGHT-VLY");
	LOG1D(vw->vry, cfg->y_params[MOLT_PARAM_PINC], "VWEIGHT-VRY");
	LOG1D(vw->vlz, cfg->z_params[MOLT_PARAM_PINC], "VWEIGHT-VLZ");
	LOG1D(vw->vrz, cfg->z_params[MOLT_PARAM_PINC], "VWEIGHT-VRZ");

	// configure the weights, and log wweights
	ww = io_lumpgetbase(hunk, MOLTLUMP_WWEIGHT);

	// setup the weights for passing
	Vec2Set(xweight_dim, cfg->x_params[MOLT_PARAM_POINTS], cfg->spaceacc + 1);
	Vec2Set(yweight_dim, cfg->y_params[MOLT_PARAM_POINTS], cfg->spaceacc + 1);
	Vec2Set(zweight_dim, cfg->z_params[MOLT_PARAM_POINTS], cfg->spaceacc + 1);

	LOG2D(ww->xl_weight, xweight_dim, "WWEIGHT-XL");
	LOG2D(ww->xr_weight, xweight_dim, "WWEIGHT-XR");
	LOG2D(ww->yl_weight, yweight_dim, "WWEIGHT-YL");
	LOG2D(ww->yr_weight, yweight_dim, "WWEIGHT-YR");
	LOG2D(ww->zl_weight, zweight_dim, "WWEIGHT-ZL");
	LOG2D(ww->zr_weight, zweight_dim, "WWEIGHT-ZR");

	// log out the initial state of the wave
	vmesh = io_lumpgetbase(hunk, MOLTLUMP_VMESH);
	molt_cfg_parampull_xyz(cfg, vmesh_dim, MOLT_PARAM_POINTS);
	LOG3D(vmesh->data, vmesh_dim, "VMESH");

	// log out the 3d volume
	umesh = io_lumpgetbase(hunk, MOLTLUMP_UMESH);
	molt_cfg_parampull_xyz(cfg, umesh_dim, MOLT_PARAM_POINTS);

	for (i = 0; i < cfg->t_params[MOLT_PARAM_PINC]; i++) {
		snprintf(buf, sizeof buf, "UMESH[%02d]", i);
		LOG3D((umesh + i)->data, umesh_dim, buf);
	}
}

/* lump_magiccheck : checks all lumps for the magic number, asserts if wrong */
s32 lump_magiccheck(void *hunk)
{
	// TODO (brian)
	// refactor/remove this function
	// it (probably) shouldn't be needed
	//
	//
	// the 'magic number' is MOLTLUMP_MAGIC
	// the function also returns 0 on success, when > 0, how many magic values
	// were incorrect

	struct lump_efield_t *efield;
	struct lump_pfield_t *pfield;
	struct lump_umesh_t *umesh;

	s32 single_elem_lump[5] = {
		MOLTLUMP_CONFIG, MOLTLUMP_RUNINFO,
		MOLTLUMP_VWEIGHT, MOLTLUMP_WWEIGHT, MOLTLUMP_VMESH
	};


	s32 single_elem_magic[5];
	s32 i, rc;

	rc = 0;

	/* get all of the lumps that have only one magic value */

	for (i = 0; i < 5; i++) {
		single_elem_magic[i] = io_lumpgetmeta(hunk, single_elem_lump[i])->magic;
	}


	// spin through and check them all
	for (i = 0; i < 5; i++) {
		if (single_elem_magic[i] != MOLTLUMP_MAGIC) {
			fprintf(stderr, "LUMP %d HAS MAGIC %#08X, should be %#08X\n",
					single_elem_lump[i], single_elem_magic[i], MOLTLUMP_MAGIC);
			rc += 1;
		}
	}

	efield = io_lumpgetbase(hunk, MOLTLUMP_EFIELD);
	pfield = io_lumpgetbase(hunk, MOLTLUMP_PFIELD);
	umesh   = io_lumpgetbase(hunk, MOLTLUMP_UMESH);

	// spin through the efield
	for (i = 0; i < MOLT_T_POINTS; i++) {
		if ((efield + i)->meta.magic != MOLTLUMP_MAGIC) {
			fprintf(stderr, "EFIELD %d HAS MAGIC %#08X, should be %#08X\n",
					i, (efield + i)->meta.magic, MOLTLUMP_MAGIC);
			rc += 1;
		}
	}

	// spin through the pfield
	for (i = 0; i < MOLT_T_POINTS; i++) {
		if ((pfield + i)->meta.magic != MOLTLUMP_MAGIC) {
			fprintf(stderr, "PFIELD %d HAS MAGIC %#08X, should be %#08X\n",
					i, (pfield + i)->meta.magic, MOLTLUMP_MAGIC);
			rc += 1;
		}
	}

	// spin through all of the umesh lumps
	for (i = 0; i < MOLT_T_PINC; i++) {
		if ((umesh + i)->meta.magic != MOLTLUMP_MAGIC) {
			fprintf(stderr, "umesh %d HAS MAGIC %#08X, should be %#08X\n",
					i, (umesh + i)->meta.magic, MOLTLUMP_MAGIC);
			rc += 1;
		}
	}

	return rc;
}

/* print_help : prints some help text */
void print_help(char *prog)
{
	fprintf(stderr, USAGE, prog);
	fprintf(stderr, "-h\t\t\tprints this help text\n");
	fprintf(stderr, "-v\t\t\tdisplays verbose simulation info\n");
	fprintf(stderr, "--viewer\t\tattempts to run the viewer\n");
}

