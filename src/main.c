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
 * TODO (Brian)
 *
 * Better Debugging Output (printf)
 *   We need better debugging output. lump_magiccheck isn't really quite good
 *   enough, as we probably actually want some reasonable way to get the output
 *   from Matlab and compare it with the output here.
 *
 *   I think the most reasonable thing to do is probably have a function
 *   that looks like
 *
 *   log_item(char *file, int line, char *name, struct dim3 dim, double *ptr);
 *
 *   Where log_item would output all of the contents of ptr with the
 *   dimensionality as described in the given dim3.
 *
 *   We'd also need to macro it for easier use:
 *
 *	 LOGITEM(x, y, z) ((__FILE__), (__LINE__), (x), (y), (z))
 *
 *   This is probably going to need to be called from within molt.c
 *
 * Cleaner Calling for X, Y, and Z operators
 *
 * Get Information on EField and PField simulation from Causley
 *   Look in common.h:~250 for more details
 *
 * Store Dimensionality in cfg structure
 *   Would be stored as an ivec{1,2,3}_t where necessary
 *
 * WISHLIST:
 * 1. Remapping Procedure should be rethought out (setup_simulation)
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

#include "io.h"
#include "calcs.h"
#include "config.h"
#include "molt.h"

#define DEFAULTFILE "data.dat"
#define DATADUMP 1

/* lump setup functions */
void setup_simulation(void **base, u64 *size, int fd);
u64 setup_lumps(void *base);
void setuplump_cfg(struct cfg_t *cfg);
void setuplump_run(struct lump_runinfo_t *run);
void setuplump_efield(struct lump_header_t *hdr, struct lump_efield_t *efield);
void setuplump_pfield(struct lump_header_t *hdr, struct lump_pfield_t *pfield);
void setuplump_nu(struct lump_header_t *hdr, struct lump_nu_t *nu);
void setuplump_vweight(struct lump_header_t *hdr, struct lump_vweight_t *vw);
void setuplump_wweight(struct lump_header_t *hdr, struct lump_wweight_t *ww);
void setuplump_mesh(struct lump_header_t *hdr, struct lump_mesh_t *state);

void do_simulation(void *hunk, u64 hunksize);

void setupstate_print(void *hunk);
s32 lump_magiccheck(void *hunk);

int main(int argc, char **argv)
{
	void *hunk;
	u64 hunksize;
	int fd;

	hunksize = sizeof(struct lump_header_t);

	fd = io_open(DEFAULTFILE);

	io_resize(fd, hunksize);
	hunk = io_mmap(fd, hunksize);

	setup_simulation(&hunk, &hunksize, fd);

	io_masync(hunk, hunk, hunksize);

	do_simulation(hunk, hunksize);

	/* sync the file, then clean up */
	io_mssync(hunk, hunk, hunksize);

	setupstate_print(hunk);

	io_munmap(hunk);
	io_close(fd);

	return 0;
}

/* do_simulation : actually does the simulating */
void do_simulation(void *hunk, u64 hunksize)
{
	struct cfg_t *cfg;
	struct lump_runinfo_t *run;
	struct lump_nu_t *nu;
	struct lump_vweight_t *vw;
	struct lump_wweight_t *ww;
	struct lump_mesh_t *mesh, *curr;

	cfg   = io_lumpgetbase(hunk, MOLTLUMP_CONFIG);
	run   = io_lumpgetbase(hunk, MOLTLUMP_RUNINFO);
	nu    = io_lumpgetbase(hunk, MOLTLUMP_NU);
	vw    = io_lumpgetbase(hunk, MOLTLUMP_VWEIGHT);
	ww    = io_lumpgetbase(hunk, MOLTLUMP_WWEIGHT);
	mesh  = io_lumpgetbase(hunk, MOLTLUMP_MESH);

	assert(molt_init());

	molt_firststep(mesh + 1, mesh, cfg, nu, vw, ww);

	for (run->t_idx = 1; run->t_idx < run->t_total; run->t_idx++) {
		curr = mesh + run->t_idx;
		molt_step(curr + 1, curr, cfg, nu, vw, ww);
		/* save off some fields as required */
	}

	molt_free();
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
		fprintf(stderr, "Couldn't remap the file!\n");
	} else {
		*base = newblk;
	}

	printf("file size %ld\n", *size);

	setuplump_cfg(io_lumpgetbase(*base, MOLTLUMP_CONFIG));
	setuplump_run(io_lumpgetbase(*base, MOLTLUMP_RUNINFO));
	setuplump_efield(*base, io_lumpgetbase(*base, MOLTLUMP_EFIELD));
	setuplump_pfield(*base, io_lumpgetbase(*base, MOLTLUMP_PFIELD));
	setuplump_nu(*base, io_lumpgetbase(*base, MOLTLUMP_NU));
	setuplump_vweight(*base, io_lumpgetbase(*base, MOLTLUMP_VWEIGHT));
	setuplump_wweight(*base, io_lumpgetbase(*base, MOLTLUMP_WWEIGHT));
	setuplump_mesh(*base, io_lumpgetbase(*base, MOLTLUMP_MESH));
}

/* setup_lumps : returns size of file after lumpsystem setup */
u64 setup_lumps(void *base)
{
	struct lump_header_t *hdr;
	u64 curr_offset;

	/* begin the setup by setting up our lump header and our lump directory */

	hdr = base;
	curr_offset = sizeof(struct lump_header_t);

	/* setup the lump header with the little run-time data we have */
	hdr->lump[0].offset = curr_offset;
	hdr->lump[0].elemsize = sizeof(struct cfg_t);
	hdr->lump[0].lumpsize = 1 * hdr->lump[0].elemsize;

	curr_offset += hdr->lump[0].lumpsize;

	/* setup our runtime info lump */
	hdr->lump[1].offset = curr_offset;
	hdr->lump[1].elemsize = sizeof(struct lump_runinfo_t);
	hdr->lump[1].lumpsize = 1 * hdr->lump[1].elemsize;

	curr_offset += hdr->lump[1].lumpsize;

	/* setup our nu information */
	hdr->lump[2].offset = curr_offset;
	hdr->lump[2].elemsize = sizeof(struct lump_nu_t);
	hdr->lump[2].lumpsize = hdr->lump[2].elemsize;

	curr_offset += hdr->lump[2].lumpsize;

	/* setup our vweight information */
	hdr->lump[3].offset = curr_offset;
	hdr->lump[3].elemsize = sizeof(struct lump_vweight_t);
	hdr->lump[3].lumpsize = hdr->lump[3].elemsize;

	curr_offset += hdr->lump[3].lumpsize;

	/* setup our wweight information */
	hdr->lump[4].offset = curr_offset;
	hdr->lump[4].elemsize = sizeof(struct lump_wweight_t);
	hdr->lump[4].lumpsize = hdr->lump[4].elemsize;

	curr_offset += hdr->lump[4].lumpsize;

	/* setup our efield information */
	hdr->lump[5].offset = curr_offset;
	hdr->lump[5].elemsize = sizeof(struct lump_efield_t);
	hdr->lump[5].lumpsize = ((u64)MOLT_T_POINTS) * hdr->lump[5].elemsize;

	curr_offset += hdr->lump[5].lumpsize;

	/* setup our pfield information */
	hdr->lump[6].offset = curr_offset;
	hdr->lump[6].elemsize = sizeof(struct lump_pfield_t);
	hdr->lump[6].lumpsize = ((u64)MOLT_T_POINTS) * hdr->lump[6].elemsize;

	curr_offset += hdr->lump[6].lumpsize;

	/* setup our problem state */
	hdr->lump[7].offset = curr_offset;
	hdr->lump[7].elemsize = sizeof(struct lump_mesh_t);
	hdr->lump[7].lumpsize = ((u64)MOLT_T_POINTS_INC) * hdr->lump[7].elemsize;

	curr_offset += hdr->lump[7].lumpsize;

	return curr_offset;
}

/* simsetup_cfg : setup the config lump */
void setuplump_cfg(struct cfg_t *cfg)
{
	memset(cfg, 0, sizeof *cfg);

	cfg->meta.magic = MOLTLUMP_MAGIC;

	/* free space parms */
	cfg->lightspeed = MOLT_LIGHTSPEED;
	cfg->henrymeter = MOLT_HENRYPERMETER;
	cfg->faradmeter = MOLT_FARADSPERMETER;

	/* tissue space parms */
	cfg->staticperm = MOLT_STATICPERM;
	cfg->infiniteperm = MOLT_INFPERM;
	cfg->tau = MOLT_TAU;
	cfg->distribtail = MOLT_DISTRIBTAIL;
	cfg->distribasym = MOLT_DISTRIBASYM;
	cfg->tissuespeed = MOLT_TISSUESPEED;

	/* mesh parms */
	cfg->cfl = MOLT_CFL;

	cfg->int_scale = MOLT_INTSCALE;

	/* dimension parms */
	cfg->t_start      = MOLT_T_START;
	cfg->t_stop       = MOLT_T_STOP;
	cfg->t_step       = MOLT_T_STEP;
	cfg->t_points     = MOLT_T_POINTS;
	cfg->t_points_inc = MOLT_T_POINTS_INC;

	cfg->x_start      = MOLT_X_START;
	cfg->x_stop       = MOLT_X_STOP;
	cfg->x_step       = MOLT_X_STEP;
	cfg->x_points     = MOLT_X_POINTS;
	cfg->x_points_inc = MOLT_X_POINTS_INC;

	cfg->y_start      = MOLT_Y_START;
	cfg->y_stop       = MOLT_Y_STOP;
	cfg->y_step       = MOLT_Y_STEP;
	cfg->y_points     = MOLT_Y_POINTS;
	cfg->y_points_inc = MOLT_Y_POINTS_INC;

	cfg->z_start      = MOLT_Z_START;
	cfg->z_stop       = MOLT_Z_STOP;
	cfg->z_step       = MOLT_Z_STEP;
	cfg->z_points     = MOLT_Z_POINTS;
	cfg->z_points_inc = MOLT_Z_POINTS_INC;

	/* MOLT parameters */
	cfg->space_acc = MOLT_SPACEACC;
	cfg->time_acc = MOLT_TIMEACC;
	cfg->beta = MOLT_BETA;
	cfg->beta_sq = pow(MOLT_BETA, 2);
	cfg->beta_fo = pow(MOLT_BETA, 4) / 12;
	cfg->beta_si = pow(MOLT_BETA, 6) / 360;
	cfg->alpha = MOLT_ALPHA;

	/* dimensionality setup */

	// nu and dnu
	cfg->nux_dim  = MOLT_X_POINTS;
	cfg->nuy_dim  = MOLT_Y_POINTS;
	cfg->nuz_dim  = MOLT_Z_POINTS;
	cfg->dnux_dim = MOLT_X_POINTS;
	cfg->dnuy_dim = MOLT_Y_POINTS;
	cfg->dnuz_dim = MOLT_Z_POINTS;

	// efield
	cfg->efield_data_dim[0] = MOLT_X_POINTS_INC;
	cfg->efield_data_dim[1] = MOLT_Y_POINTS_INC;
	cfg->efield_data_dim[2] = MOLT_Z_POINTS_INC;

	// pfield
	cfg->pfield_data_dim[0] = MOLT_X_POINTS_INC;
	cfg->pfield_data_dim[1] = MOLT_Y_POINTS_INC;
	cfg->pfield_data_dim[2] = MOLT_Z_POINTS_INC;

	// vweight
	cfg->vlx_dim = MOLT_X_POINTS_INC;
	cfg->vrx_dim = MOLT_X_POINTS_INC;
	cfg->vly_dim = MOLT_Y_POINTS_INC;
	cfg->vry_dim = MOLT_Y_POINTS_INC;
	cfg->vlz_dim = MOLT_Z_POINTS_INC;
	cfg->vrz_dim = MOLT_Z_POINTS_INC;

	// wweight
	cfg->xl_weight_dim[0] = MOLT_X_POINTS; cfg->xl_weight_dim[1] = MOLT_SPACEACC + 1;
	cfg->xr_weight_dim[0] = MOLT_X_POINTS; cfg->xr_weight_dim[1] = MOLT_SPACEACC + 1;
	cfg->yl_weight_dim[0] = MOLT_Y_POINTS; cfg->yl_weight_dim[1] = MOLT_SPACEACC + 1;
	cfg->yr_weight_dim[0] = MOLT_Y_POINTS; cfg->yr_weight_dim[1] = MOLT_SPACEACC + 1;
	cfg->zl_weight_dim[0] = MOLT_Z_POINTS; cfg->zl_weight_dim[1] = MOLT_SPACEACC + 1;
	cfg->zr_weight_dim[0] = MOLT_Z_POINTS; cfg->zr_weight_dim[1] = MOLT_SPACEACC + 1;

	// mesh (umesh and vmesh)
	cfg->umesh_dim[0] = MOLT_X_POINTS_INC;
	cfg->umesh_dim[1] = MOLT_Y_POINTS_INC;
	cfg->umesh_dim[2] = MOLT_Z_POINTS_INC;
	cfg->vmesh_dim[0] = MOLT_X_POINTS_INC;
	cfg->vmesh_dim[1] = MOLT_Y_POINTS_INC;
	cfg->vmesh_dim[2] = MOLT_Z_POINTS_INC;
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
	// MOLT_T_POINTS_INC as the "firststep" mesh storage
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

/* setuplump_nu : setup the nu lump */
void setuplump_nu(struct lump_header_t *hdr, struct lump_nu_t *nu)
{
	struct cfg_t *cfg;
	struct lump_t *lump;
	s32 i;

	lump = &hdr->lump[MOLTLUMP_NU];
	memset(nu, 0, lump->lumpsize);

	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);

	nu->meta.magic = MOLTLUMP_MAGIC;

	for (i = 0; i < cfg->nux_dim; i++)
		nu->nux[i] = cfg->int_scale * cfg->x_step * cfg->alpha;

	for (i = 0; i < cfg->dnux_dim; i++)
		nu->dnux[i] = exp(-nu->nux[i]);

	for (i = 0; i < cfg->nuy_dim; i++)
		nu->nuy[i] = cfg->int_scale * cfg->y_step * cfg->alpha;

	for (i = 0; i < cfg->dnuy_dim; i++)
		nu->dnuy[i] = exp(-nu->nuy[i]);

	for (i = 0; i < cfg->nuz_dim; i++)
		nu->nuz[i] = cfg->int_scale * cfg->z_step * cfg->alpha;

	for (i = 0; i < cfg->dnuz_dim; i++)
		nu->dnuz[i] = exp(-nu->nuz[i]);
}

/* setuplump_vweight : setup the vweight lump */
void setuplump_vweight(struct lump_header_t *hdr, struct lump_vweight_t *vw)
{
	struct cfg_t *cfg;
	f64 alpha;
	s64 i;

	memset(vw, 0, sizeof *vw);

	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);
	alpha = cfg->alpha;

	vw->meta.magic = MOLTLUMP_MAGIC;

	for (i = 0; i < cfg->vlx_dim; i++) // vlx
		vw->vlx[i] = exp((-alpha) * cfg->int_scale * i);

	for (i = 0; i < cfg->vrx_dim; i++) // vrx
		vw->vrx[i] = exp((-alpha) * cfg->int_scale * i);

	for (i = 0; i < cfg->vly_dim; i++) // vly
		vw->vly[i] = exp((-alpha) * cfg->int_scale * i);

	for (i = 0; i < cfg->vry_dim; i++) // vry
		vw->vry[i] = exp((-alpha) * cfg->int_scale * i);

	for (i = 0; i < cfg->vlz_dim; i++) // vlz
		vw->vrz[i] = exp((-alpha) * cfg->int_scale * i);

	for (i = 0; i < cfg->vrz_dim; i++) // vrz
		vw->vlz[i] = exp((-alpha) * cfg->int_scale * i);
}

/* setuplump_wweight : setup the wweight lump */
void setuplump_wweight(struct lump_header_t *hdr, struct lump_wweight_t *ww)
{
	struct cfg_t *cfg;
	struct lump_nu_t *nu;

	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);
	nu  = io_lumpgetbase(hdr, MOLTLUMP_NU);

	ww->meta.magic = MOLTLUMP_MAGIC;

	// perform all of our get_exp_weights
	get_exp_weights(nu->nux, ww->xl_weight, ww->xr_weight, cfg->x_points, cfg->space_acc);
	get_exp_weights(nu->nuy, ww->yl_weight, ww->yr_weight, cfg->y_points, cfg->space_acc);
	get_exp_weights(nu->nuz, ww->zl_weight, ww->zr_weight, cfg->z_points, cfg->space_acc);
}

/* setuplump_mesh : setup the "problem state" lump */
void setuplump_mesh(struct lump_header_t *hdr, struct lump_mesh_t *state)
{
	// write zeroes to the mesh
	s32 x, y, z, i;
	struct cfg_t *cfg;
	struct lump_t *lump;
	struct lump_mesh_t *mesh;
	f64 fx, fy, fz;

	lump = &hdr->lump[MOLTLUMP_MESH];
	mesh = io_lumpgetbase(hdr, MOLTLUMP_MESH);
	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);

	memset(state, 0, lump->lumpsize);

	for (i = 0; i < lump->lumpsize / lump->elemsize; i++) {
		mesh[i].meta.magic = MOLTLUMP_MAGIC;
	}

	// we only want to setup the first (0th) umesh applies function f
	for (z = 0; z < cfg->z_points_inc; z++) {
		for (y = 0; y < cfg->y_points_inc; y++) {
			for (x = 0; x < cfg->x_points_inc; x++) {
				i = IDX3D(x, y, z, cfg->y_points_inc, cfg->z_points_inc);

				fx = pow((cfg->int_scale * (x - MOLT_X_POINTS / 2)), 2);
				fy = pow((cfg->int_scale * (y - MOLT_Y_POINTS / 2)), 2);
				fz = pow((cfg->int_scale * (z - MOLT_Z_POINTS / 2)), 2);

				mesh->umesh[i] = exp(-fx - fy - fz);
			}
		}
	}
}

/* setupstate_print : perform lots of printf debugging */
void setupstate_print(void *hunk)
{
	struct lump_header_t *hdr;
	struct cfg_t *cfg;
	struct lump_nu_t *nu;
	struct lump_vweight_t *vw;
	struct lump_wweight_t *ww;
	struct lump_mesh_t *mesh;
	s32 i, x, y, z, j;

	hdr = hunk;

	cfg = io_lumpgetbase(hunk, MOLTLUMP_CONFIG);

	printf("header : 0x%8x\tver : %d\t type : 0x%02x\n", hdr->meta.magic, hdr->meta.version, hdr->meta.type);

	// check the lump metadata before anything else
	assert(!lump_magiccheck(hunk));

	/* print out lump information */
	for (i = 0; i < MOLTLUMP_TOTAL; i++) {
		printf("lump[%d] off : 0x%08lx\tlumpsz : 0x%08lx\telemsz : 0x%08lx\n",
			i, hdr->lump[i].offset, hdr->lump[i].lumpsize, hdr->lump[i].elemsize);
	}
	printf("\n");

	// dump out all of the nu segments
	nu = io_lumpgetbase(hunk, MOLTLUMP_NU);
	LOG1D(nu->nux, cfg->nux_dim, "NU-X");
	LOG1D(nu->nuy, cfg->nuy_dim, "NU-Y");
	LOG1D(nu->nuz, cfg->nuz_dim, "NU-Z");
	LOG1D(nu->dnux, cfg->dnux_dim, "DNU-X");
	LOG1D(nu->dnuy, cfg->dnuy_dim, "DNU-Y");
	LOG1D(nu->dnuz, cfg->dnuz_dim, "DNU-Z");

	// log all of the vweights
	vw = io_lumpgetbase(hunk, MOLTLUMP_VWEIGHT);
	LOG1D(vw->vlx, cfg->vlx_dim, "VWEIGHT-VLX");
	LOG1D(vw->vrx, cfg->vrx_dim, "VWEIGHT-VRX");
	LOG1D(vw->vly, cfg->vly_dim, "VWEIGHT-VLY");
	LOG1D(vw->vry, cfg->vry_dim, "VWEIGHT-VRY");
	LOG1D(vw->vlz, cfg->vlz_dim, "VWEIGHT-VLZ");
	LOG1D(vw->vrz, cfg->vrz_dim, "VWEIGHT-VRZ");

	// log all of the wweights
	ww = io_lumpgetbase(hunk, MOLTLUMP_WWEIGHT);
	LOG2D(ww->xl_weight, cfg->xl_weight_dim, "WWEIGHT-XL");
	LOG2D(ww->xr_weight, cfg->xr_weight_dim, "WWEIGHT-XR");
	LOG2D(ww->yl_weight, cfg->yl_weight_dim, "WWEIGHT-YL");
	LOG2D(ww->yr_weight, cfg->yr_weight_dim, "WWEIGHT-YR");
	LOG2D(ww->zl_weight, cfg->zl_weight_dim, "WWEIGHT-ZL");
	LOG2D(ww->zr_weight, cfg->zr_weight_dim, "WWEIGHT-ZR");

	// log out the mesh
	mesh = io_lumpgetbase(hunk, MOLTLUMP_MESH);
	LOG3D(mesh->umesh, cfg->umesh_dim, "UMESH[0]");
	LOG3D(mesh->vmesh, cfg->vmesh_dim, "VMESH[0]");
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
	struct lump_mesh_t *mesh;

	s32 simplemagic[5], meshmagic[MOLT_T_POINTS_INC];
	s32 i, rc;
	char hex1[16], hex2[16];

	rc = 0;

	// get all of the lumps that have only one magic value
	simplemagic[0] = io_lumpgetmeta(hunk, MOLTLUMP_CONFIG)->magic;
	simplemagic[1] = io_lumpgetmeta(hunk, MOLTLUMP_RUNINFO)->magic;
	simplemagic[2] = io_lumpgetmeta(hunk, MOLTLUMP_NU)->magic;
	simplemagic[3] = io_lumpgetmeta(hunk, MOLTLUMP_VWEIGHT)->magic;
	simplemagic[4] = io_lumpgetmeta(hunk, MOLTLUMP_WWEIGHT)->magic;

	// spin through and check them all
	for (i = 0; i < 5; i++) {
		if (simplemagic[i] != MOLTLUMP_MAGIC) {
			snprintf(hex1, sizeof(hex1), "0x%04x", simplemagic[i]);
			snprintf(hex2, sizeof(hex2), "0x%04x", MOLTLUMP_MAGIC);
			fprintf(stderr, "LUMP %d HAS MAGIC %s, should be %s\n", i, hex1, hex2);
			rc += 1;
		}
	}

	efield = io_lumpgetbase(hunk, MOLTLUMP_EFIELD);
	pfield = io_lumpgetbase(hunk, MOLTLUMP_PFIELD);
	mesh   = io_lumpgetbase(hunk, MOLTLUMP_MESH);

	// spin through the efield
	for (i = 0; i < MOLT_T_POINTS; i++) {
		if ((efield + i)->meta.magic != MOLTLUMP_MAGIC) {
			snprintf(hex1, sizeof(hex1), "0x%04x", (efield + i)->meta.magic);
			snprintf(hex2, sizeof(hex2), "0x%04x", MOLTLUMP_MAGIC);
			fprintf(stderr, "EFIELD %d HAS MAGIC %s, should be %s\n", i, hex1, hex2);
			rc += 1;
		}
	}

	// spin through the pfield
	for (i = 0; i < MOLT_T_POINTS; i++) {
		if ((pfield + i)->meta.magic != MOLTLUMP_MAGIC) {
			snprintf(hex1, sizeof(hex1), "0x%04x", (pfield + i)->meta.magic);
			snprintf(hex2, sizeof(hex2), "0x%04x", MOLTLUMP_MAGIC);
			fprintf(stderr, "PFIELD %d HAS MAGIC %s, should be %s\n", i, hex1, hex2);
			rc += 1;
		}
	}

	// spin through all of the mesh lumps
	for (i = 0; i < MOLT_T_POINTS_INC; i++) {
		if ((mesh + i)->meta.magic != MOLTLUMP_MAGIC) {
			snprintf(hex1, sizeof(hex1), "0x%04x", (mesh + i)->meta.magic);
			snprintf(hex2, sizeof(hex2), "0x%04x", MOLTLUMP_MAGIC);
			fprintf(stderr, "MESH %d HAS MAGIC %s, should be %s\n", i, hex1, hex2);
			rc += 1;
		}
	}

	return rc;
}

