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
 * Cleaner Calling for X, Y, and Z operators
 *
 * Get Information on EField and PField simulation from Causley
 *   Look in common.h:~250 for more details
 *
 * Store Dimensionality in cfg structure
 *   Would be stored as an ivec{1,2,3}_t where necessary
 *
 * WISHLIST
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

#define MOLT_IMPLEMENTATION
#include "molt.h"

#define DEFAULTFILE "data.dat"
#define DATADUMP 1

/* lump setup functions */
void setup_simulation(void **base, u64 *size, int fd);
u64 setup_lumps(void *base);
void setuplump_cfg(struct lump_header_t *hdr, struct molt_cfg_t *cfg);
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

	lump_magiccheck(hunk);
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

#if 0
	molt_firststep(mesh + 1, mesh, cfg, nu, vw, ww);

	for (run->t_idx = 1; run->t_idx < run->t_total; run->t_idx++) {
		curr = mesh + run->t_idx;
		molt_step(curr + 1, curr, cfg, nu, vw, ww);
		/* save off some fields as required */
	}
#endif
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
	hdr->lump[0].elemsize = sizeof(struct molt_cfg_t);
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
	hdr->lump[7].lumpsize = ((u64)MOLT_T_PINC) * hdr->lump[7].elemsize;

	curr_offset += hdr->lump[7].lumpsize;

	return curr_offset;
}

/* simsetup_cfg : setup the config lump */
void setuplump_cfg(struct lump_header_t *hdr, struct molt_cfg_t *cfg)
{
	memset(cfg, 0, sizeof *cfg);

	molt_cfg_set_intscale(cfg, MOLT_INTSCALE);

	molt_cfg_dims_t(cfg, MOLT_T_START, MOLT_T_STOP, MOLT_T_STEP, MOLT_T_POINTS, MOLT_T_PINC);
	molt_cfg_dims_x(cfg, MOLT_X_START, MOLT_X_STOP, MOLT_X_STEP, MOLT_X_POINTS, MOLT_X_PINC);
	molt_cfg_dims_y(cfg, MOLT_Y_START, MOLT_Y_STOP, MOLT_Y_STEP, MOLT_Y_POINTS, MOLT_Y_PINC);
	molt_cfg_dims_z(cfg, MOLT_Z_START, MOLT_Z_STOP, MOLT_Z_STEP, MOLT_Z_POINTS, MOLT_Z_PINC);

	molt_cfg_set_accparams(cfg, MOLT_SPACEACC, MOLT_TIMEACC);

	// compute alpha by hand because it relies on simulation specific params
	// (beta is set by the previous function)
	// TODO (brian) can we get alpha setting into the library
	cfg->alpha = cfg->beta /
		(MOLT_TISSUESPEED * cfg->t_params[MOLT_PARAM_STEP] * cfg->int_scale);

	// nu and dnu
	cfg->nux_dim  = MOLT_X_POINTS;
	cfg->nuy_dim  = MOLT_Y_POINTS;
	cfg->nuz_dim  = MOLT_Z_POINTS;
	cfg->dnux_dim = MOLT_X_POINTS;
	cfg->dnuy_dim = MOLT_Y_POINTS;
	cfg->dnuz_dim = MOLT_Z_POINTS;

	// efield
	cfg->efield_data_dim[0] = MOLT_X_PINC;
	cfg->efield_data_dim[1] = MOLT_Y_PINC;
	cfg->efield_data_dim[2] = MOLT_Z_PINC;

	// pfield
	cfg->pfield_data_dim[0] = MOLT_X_PINC;
	cfg->pfield_data_dim[1] = MOLT_Y_PINC;
	cfg->pfield_data_dim[2] = MOLT_Z_PINC;

	// vweight
	cfg->vlx_dim = MOLT_X_PINC;
	cfg->vrx_dim = MOLT_X_PINC;
	cfg->vly_dim = MOLT_Y_PINC;
	cfg->vry_dim = MOLT_Y_PINC;
	cfg->vlz_dim = MOLT_Z_PINC;
	cfg->vrz_dim = MOLT_Z_PINC;

	// wweight
	cfg->xl_weight_dim[0] = MOLT_X_POINTS; cfg->xl_weight_dim[1] = MOLT_SPACEACC + 1;
	cfg->xr_weight_dim[0] = MOLT_X_POINTS; cfg->xr_weight_dim[1] = MOLT_SPACEACC + 1;
	cfg->yl_weight_dim[0] = MOLT_Y_POINTS; cfg->yl_weight_dim[1] = MOLT_SPACEACC + 1;
	cfg->yr_weight_dim[0] = MOLT_Y_POINTS; cfg->yr_weight_dim[1] = MOLT_SPACEACC + 1;
	cfg->zl_weight_dim[0] = MOLT_Z_POINTS; cfg->zl_weight_dim[1] = MOLT_SPACEACC + 1;
	cfg->zr_weight_dim[0] = MOLT_Z_POINTS; cfg->zr_weight_dim[1] = MOLT_SPACEACC + 1;

	// mesh (umesh and vmesh)
	cfg->umesh_dim[0] = MOLT_X_PINC;
	cfg->umesh_dim[1] = MOLT_Y_PINC;
	cfg->umesh_dim[2] = MOLT_Z_PINC;
	cfg->vmesh_dim[0] = MOLT_X_PINC;
	cfg->vmesh_dim[1] = MOLT_Y_PINC;
	cfg->vmesh_dim[2] = MOLT_Z_PINC;
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

/* setuplump_nu : setup the nu lump */
void setuplump_nu(struct lump_header_t *hdr, struct lump_nu_t *nu)
{
	struct molt_cfg_t *cfg;
	struct lump_t *lump;
	s32 i;

	lump = &hdr->lump[MOLTLUMP_NU];
	memset(nu, 0, lump->lumpsize);

	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);

	nu->meta.magic = MOLTLUMP_MAGIC;

	for (i = 0; i < cfg->nux_dim; i++)
		nu->nux[i] = cfg->int_scale * cfg->x_params[MOLT_PARAM_STEP] * cfg->alpha;

	for (i = 0; i < cfg->dnux_dim; i++)
		nu->dnux[i] = exp(-nu->nux[i]);

	for (i = 0; i < cfg->nuy_dim; i++)
		nu->nuy[i] = cfg->int_scale * cfg->y_params[MOLT_PARAM_STEP] * cfg->alpha;

	for (i = 0; i < cfg->dnuy_dim; i++)
		nu->dnuy[i] = exp(-nu->nuy[i]);

	for (i = 0; i < cfg->nuz_dim; i++)
		nu->nuz[i] = cfg->int_scale * cfg->z_params[MOLT_PARAM_STEP] * cfg->alpha;

	for (i = 0; i < cfg->dnuz_dim; i++)
		nu->dnuz[i] = exp(-nu->nuz[i]);
}

/* setuplump_vweight : setup the vweight lump */
void setuplump_vweight(struct lump_header_t *hdr, struct lump_vweight_t *vw)
{
	struct molt_cfg_t *cfg;
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
	struct molt_cfg_t *cfg;
	struct lump_nu_t *nu;

	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);
	nu  = io_lumpgetbase(hdr, MOLTLUMP_NU);

	ww->meta.magic = MOLTLUMP_MAGIC;

	// perform all of our get_exp_weights
	get_exp_weights(nu->nux, ww->xl_weight, ww->xr_weight, cfg->x_params[MOLT_PARAM_POINTS], cfg->spaceacc);
	get_exp_weights(nu->nuy, ww->yl_weight, ww->yr_weight, cfg->y_params[MOLT_PARAM_POINTS], cfg->spaceacc);
	get_exp_weights(nu->nuz, ww->zl_weight, ww->zr_weight, cfg->z_params[MOLT_PARAM_POINTS], cfg->spaceacc);
}

/* setuplump_mesh : setup the "problem state" lump */
void setuplump_mesh(struct lump_header_t *hdr, struct lump_mesh_t *state)
{
	// write zeroes to the mesh
	s32 x, y, z, i, xpinc, ypinc, zpinc;
	struct molt_cfg_t *cfg;
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

	xpinc = cfg->x_params[MOLT_PARAM_PINC];
	ypinc = cfg->y_params[MOLT_PARAM_PINC];
	zpinc = cfg->y_params[MOLT_PARAM_PINC];

	// we only want to setup the first (0th) umesh applies function f
	for (z = 0; z < zpinc; z++) {
		for (y = 0; y < ypinc; y++) {
			for (x = 0; x < xpinc; x++) {
				i = IDX3D(x, y, z, ypinc, zpinc);

				fx = pow((cfg->int_scale * (x - cfg->x_params[MOLT_PARAM_POINTS] / 2)), 2);
				fy = pow((cfg->int_scale * (y - cfg->y_params[MOLT_PARAM_POINTS] / 2)), 2);
				fz = pow((cfg->int_scale * (z - cfg->z_params[MOLT_PARAM_POINTS] / 2)), 2);

				mesh->umesh[i] = exp(-fx - fy - fz);
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
	struct lump_mesh_t *mesh;
	s32 i;

	hdr = hunk;

	cfg = io_lumpgetbase(hunk, MOLTLUMP_CONFIG);

	printf("header : 0x%8x\tver : %d\t type : 0x%02x\n", hdr->meta.magic, hdr->meta.version, hdr->meta.type);

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
	// LOG3DORD(mesh->umesh, cfg->umesh_dim, "UMESH[0]", ord_y_x_z);

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

	s32 simplemagic[5];
	s32 i, rc;

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
			fprintf(stderr, "LUMP %d HAS MAGIC %#08X, should be %#08X\n",
					i, simplemagic[i], MOLTLUMP_MAGIC);
			rc += 1;
		}
	}

	efield = io_lumpgetbase(hunk, MOLTLUMP_EFIELD);
	pfield = io_lumpgetbase(hunk, MOLTLUMP_PFIELD);
	mesh   = io_lumpgetbase(hunk, MOLTLUMP_MESH);

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

	// spin through all of the mesh lumps
	for (i = 0; i < MOLT_T_PINC; i++) {
		if ((mesh + i)->meta.magic != MOLTLUMP_MAGIC) {
			fprintf(stderr, "MESH %d HAS MAGIC %#08X, should be %#08X\n",
					i, (mesh + i)->meta.magic, MOLTLUMP_MAGIC);
			rc += 1;
		}
	}

	return rc;
}

