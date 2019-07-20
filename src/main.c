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
 * Viewer
 * 1. Parse Arguments
 * 2. Read Keyboard/Mouse Inputs
 * 3. Frame Timing Controls
 * 4. Display 3d Room
 * 5. Display Simulation Bounded Cube (Perform in-sim Rotations)
 *
 * WISHLIST
 * 1. setuplump_* should really be "init" or "fillup" lump
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
#include "common.h"
#include "viewer.h"

#define MOLT_IMPLEMENTATION
#include "molt.h"

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

void print_help(char *prog);

#define USAGE "USAGE : %s [--viewer] [-v] [-h] outfile\n"

#define VERBOSE 0x01
#define VIEWER  0x02

int main(int argc, char **argv)
{
	void *hunk;
	char *fname, *s, **targv, targc;
	u64 hunksize;
	s32 fd, fexists, longopt;
	u32 flags;

	flags = 0, targc = argc, targv = argv;

	// parse arguments
	while (--targc > 0 && (*++targv)[0] == '-') {
		for (s = targv[0] + 1, longopt = 0; *s && !longopt; s++) {

			// parse long options first to avoid getting all illegal options
			if (strcmp(s, "-viewer") == 0) {
				flags |= VIEWER;
				longopt = 1;
				continue;
			}

			switch (*s) {
				case 'v':
					flags |= VERBOSE;
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
	fexists = io_exists(fname);

	hunksize = sizeof(struct lump_header_t);

	fd = io_open(fname);

	// NOTE (brian) we only want to resize the file and zero out the sim data
	// if we have a completely new file, otherwise we might want to see it in
	// the viewer, or re-perform the calculations or something
	if (!fexists) {
		io_resize(fd, hunksize);
	}

	hunk = io_mmap(fd, hunksize);
	if (!fexists) {
		setup_simulation(&hunk, &hunksize, fd);
	}

	io_masync(hunk, hunk, hunksize);

	if (flags & VIEWER) {
		viewer_run(hunk, hunksize, fd, NULL);
	} else {
		do_simulation(hunk, hunksize);
	}

	/* sync the file, then clean up */
	io_mssync(hunk, hunk, hunksize);

	if (flags & VERBOSE) {
		lump_magiccheck(hunk);
		setupstate_print(hunk);
	}

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
// void setuplump_cfg(struct molt_cfg_t *cfg, int argc, char **argv)
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
	// TODO (brian) can we get alpha setting into the library?
	cfg->alpha = cfg->beta /
		(MOLT_TISSUESPEED * cfg->t_params[MOLT_PARAM_STEP] * cfg->int_scale);
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
	ivec3_t dim, step;

	lump = &hdr->lump[MOLTLUMP_NU];
	memset(nu, 0, lump->lumpsize);

	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);

	nu->meta.magic = MOLTLUMP_MAGIC;

	molt_cfg_parampull_xyz(cfg, dim,  MOLT_PARAM_POINTS);
	molt_cfg_parampull_xyz(cfg, step, MOLT_PARAM_STEP);

	for (i = 0; i < dim[0]; i++)
		nu->nux[i] = cfg->int_scale * step[0] * cfg->alpha;

	for (i = 0; i < dim[0]; i++)
		nu->dnux[i] = exp(-nu->nux[i]);

	for (i = 0; i < dim[1]; i++)
		nu->nuy[i] = cfg->int_scale * step[1] * cfg->alpha;

	for (i = 0; i < dim[1]; i++)
		nu->dnuy[i] = exp(-nu->nuy[i]);

	for (i = 0; i < dim[2]; i++)
		nu->nuz[i] = cfg->int_scale * step[2] * cfg->alpha;

	for (i = 0; i < dim[2]; i++)
		nu->dnuz[i] = exp(-nu->nuz[i]);
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
		vw->vlx[i] = exp((-cfg->alpha) * cfg->int_scale * i);

	for (i = 0; i < dim[0]; i++) // vrx
		vw->vrx[i] = exp((-cfg->alpha) * cfg->int_scale * i);

	for (i = 0; i < dim[1]; i++) // vly
		vw->vly[i] = exp((-cfg->alpha) * cfg->int_scale * i);

	for (i = 0; i < dim[1]; i++) // vry
		vw->vry[i] = exp((-cfg->alpha) * cfg->int_scale * i);

	for (i = 0; i < dim[2]; i++) // vlz
		vw->vrz[i] = exp((-cfg->alpha) * cfg->int_scale * i);

	for (i = 0; i < dim[2]; i++) // vrz
		vw->vlz[i] = exp((-cfg->alpha) * cfg->int_scale * i);
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
	ivec2_t xweight_dim, yweight_dim, zweight_dim;
	ivec3_t mesh_dim;

	hdr = hunk;

	cfg = io_lumpgetbase(hunk, MOLTLUMP_CONFIG);

	/* print out lump & storage information */
	printf("header : 0x%8x\tver : %d\t type : 0x%02x\n", hdr->meta.magic, hdr->meta.version, hdr->meta.type);
	for (i = 0; i < MOLTLUMP_TOTAL; i++) {
		printf("lump[%d] off : 0x%08lx\tlumpsz : 0x%08lx\telemsz : 0x%08lx\n",
			i, hdr->lump[i].offset, hdr->lump[i].lumpsize, hdr->lump[i].elemsize);
	}
	printf("\n");

	// dump out all of the nu segments
	nu = io_lumpgetbase(hunk, MOLTLUMP_NU);
	LOG1D(nu->nux, cfg->x_params[MOLT_PARAM_POINTS], "NU-X");
	LOG1D(nu->nuy, cfg->y_params[MOLT_PARAM_POINTS], "NU-Y");
	LOG1D(nu->nuz, cfg->z_params[MOLT_PARAM_POINTS], "NU-Z");
	LOG1D(nu->dnux, cfg->x_params[MOLT_PARAM_POINTS], "DNU-X");
	LOG1D(nu->dnuy, cfg->y_params[MOLT_PARAM_POINTS], "DNU-Y");
	LOG1D(nu->dnuz, cfg->z_params[MOLT_PARAM_POINTS], "DNU-Z");

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

	// log out the mesh
	mesh = io_lumpgetbase(hunk, MOLTLUMP_MESH);

	molt_cfg_parampull_xyz(cfg, mesh_dim, MOLT_PARAM_POINTS);
	molt_cfg_parampull_xyz(cfg, mesh_dim, MOLT_PARAM_POINTS);

	// LOG3DORD(mesh->umesh, cfg->umesh_dim, "UMESH[0]", ord_y_x_z);
	LOG3D(mesh->umesh, mesh_dim, "UMESH[0]");
	LOG3D(mesh->vmesh, mesh_dim, "VMESH[0]");
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

/* print_help : prints some help text */
void print_help(char *prog)
{
	fprintf(stderr, USAGE, prog);
	fprintf(stderr, "-h\t\t\tprints this help text\n");
	fprintf(stderr, "-v\t\t\tdisplays verbose simulation info\n");
	fprintf(stderr, "--viewer\t\tattempts to run the viewer\n");
}

