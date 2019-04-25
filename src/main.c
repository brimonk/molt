/*
 * Brian Chrzanowski
 * Sun Jan 20, 2019 14:43
 *
 * Method Of Lines Transpose - Implicit Wave Solver with Pluggable Modules
 *
 * TODO (Brian)
 * 3. time looping
 * 3a. first_timestep
 * 3b. d and c operators
 * 3c. 3d array reorg
 * 3d. do_sweep
 * 3e. gf_quad
 *
 * * Use Print And Die Macro Instead of fprintf ... exit
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

/* lump setup functions */
void setup_simulation(void **base, u64 *size, int fd);
u64 setup_lumps(void *base);
void setuplump_cfg(struct moltcfg_t *cfg);
void setuplump_run(struct lump_runinfo_t *run);
void setuplump_efield(struct lump_header_t *hdr, struct lump_efield_t *efield);
void setuplump_pfield(struct lump_header_t *hdr, struct lump_pfield_t *pfield);
void setuplump_nu(struct lump_header_t *hdr, struct lump_nu_t *nu);
void setuplump_vweight(struct lump_header_t *hdr, struct lump_vweight_t *vw);
void setuplump_wweight(struct lump_header_t *hdr, struct lump_wweight_t *ww);
void setuplump_mesh(struct lump_header_t *hdr, struct lump_mesh_t *state);

void do_simulation(void *hunk, u64 hunksize);

void timestep_first(lcfg_t *cfg, lrun_t *run, lnu_t *nu, lvweight_t *vw,
					lwweight_t *ww, lmesh_t *mesh);
void timestep(
		lcfg_t *cfg, lrun_t *run, lnu_t *nu, lvweight_t *vw,
		lwweight_t *ww, lmesh_t *mesh);

void applied_funcf(void *base);
void applied_funcg(void *base);

void debug(void *hunk);

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

	debug(hunk);

	io_munmap(hunk);
	io_close(fd);

	return 0;
}

/* do_simulation : actually does the simulating */
void do_simulation(void *hunk, u64 hunksize)
{
	struct moltcfg_t *cfg;
	struct lump_runinfo_t *run;
	struct lump_nu_t *nu;
	struct lump_vweight_t *vw;
	struct lump_wweight_t *ww;
	struct lump_mesh_t *mesh, *curr, *next;

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
		next = curr + 1;
		molt_step(next, curr, cfg, nu, vw, ww);
		/* save off some fields as required */
	}

	molt_free();
}

/* setup_simulation : sets up simulation based on config.h */
void setup_simulation(void **base, u64 *size, int fd)
{
	void *newblk;
	u64 oldsize;

	// if we have a valid file, we won't set it up again
	// just run the simulation
#if 0
	// setup the file every time for debugging
	if (!io_lumpcheck(*base)) { return; }
#else
	io_lumpcheck(*base);
#endif

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
		io_fprintf(stderr, "Couldn't remap the file!\n");
	} else {
		*base = newblk;
	}

	io_fprintf(stdout, "file size %ld\n", *size);

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

	/*
	 * begin the setup by setting up our lump header and our lump directory
	 */

	hdr = base;
	curr_offset = sizeof(struct lump_header_t);

	/* setup the lump header with the little run-time data we have */
	hdr->lump[0].offset = curr_offset;
	hdr->lump[0].elemsize = sizeof(struct moltcfg_t);
	hdr->lump[0].lumpsize = 1 * hdr->lump[0].elemsize;

	curr_offset += hdr->lump[0].lumpsize;

	/* setup our runtime info lump */
	hdr->lump[1].offset = curr_offset;
	hdr->lump[1].elemsize = sizeof(struct lump_runinfo_t);
	hdr->lump[1].lumpsize = 1 * hdr->lump[1].elemsize;

	curr_offset += hdr->lump[1].lumpsize;

	/* setup our efield information */
	hdr->lump[2].offset = curr_offset;
	hdr->lump[2].elemsize = sizeof(struct lump_efield_t);
	hdr->lump[2].lumpsize = ((u64)MOLT_T_POINTS) * hdr->lump[2].elemsize;

	curr_offset += hdr->lump[2].lumpsize;

	/* setup our pfield information */
	hdr->lump[3].offset = curr_offset;
	hdr->lump[3].elemsize = sizeof(struct lump_pfield_t);
	hdr->lump[3].lumpsize = ((u64)MOLT_T_POINTS) * hdr->lump[3].elemsize;

	curr_offset += hdr->lump[3].lumpsize;

	/* setup our nu information */
	hdr->lump[4].offset = curr_offset;
	hdr->lump[4].elemsize = sizeof(struct lump_nu_t);
	hdr->lump[4].lumpsize = hdr->lump[4].elemsize;

	curr_offset += hdr->lump[4].lumpsize;

	/* setup our vweight information */
	hdr->lump[5].offset = curr_offset;
	hdr->lump[5].elemsize = sizeof(struct lump_vweight_t);
	hdr->lump[5].lumpsize = hdr->lump[5].elemsize;

	curr_offset += hdr->lump[5].lumpsize;

	/* setup our wweight information */
	hdr->lump[6].offset = curr_offset;
	hdr->lump[6].elemsize = sizeof(struct lump_wweight_t);
	hdr->lump[6].lumpsize = hdr->lump[6].elemsize;

	curr_offset += hdr->lump[6].lumpsize;

	/* setup our problem state */
	hdr->lump[7].offset = curr_offset;
	hdr->lump[7].elemsize = sizeof(struct lump_mesh_t);
	hdr->lump[7].lumpsize = ((u64)MOLT_T_POINTS_INC) * hdr->lump[7].elemsize;

	curr_offset += hdr->lump[7].lumpsize;

	return curr_offset;
}

/* simsetup_cfg : setup the config lump */
void setuplump_cfg(struct moltcfg_t *cfg)
{
	cfg->magic = MOLTLUMP_MAGIC;
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
}

/* simsetup_run : setup run information */
void setuplump_run(struct lump_runinfo_t *run)
{
	run->magic = MOLTLUMP_MAGIC;
	run->t_start = 0;
	run->t_step  = .5;
	run->t_stop  = 10;
	run->t_idx   = 0;
	run->t_total = MOLT_T_POINTS_INC;
}

/* setuplump_efield : setup the efield lump */
void setuplump_efield(struct lump_header_t *hdr, struct lump_efield_t *efield)
{
	// write zeroes to the efield
	struct lump_t *lump;
	char *ptr;

	ptr = (char *)hdr;
	lump = &hdr->lump[MOLTLUMP_EFIELD];

	memset(ptr + lump->offset, 0, lump->lumpsize);
	efield->magic = MOLTLUMP_MAGIC;
}

/* setuplump_pfield : setup the pfield lump */
void setuplump_pfield(struct lump_header_t *hdr, struct lump_pfield_t *pfield)
{
	// write zeroes to the pfield
	struct lump_t *lump;
	char *ptr;

	ptr = (char *)hdr;
	lump = &hdr->lump[MOLTLUMP_PFIELD];

	memset(ptr + lump->offset, 0, lump->lumpsize);
	pfield->magic = MOLTLUMP_MAGIC;
}

/* setuplump_nu : setup the nu lump */
void setuplump_nu(struct lump_header_t *hdr, struct lump_nu_t *nu)
{
	struct moltcfg_t *cfg;
	s64 i;

	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);

	nu->magic = MOLTLUMP_MAGIC;
	for (i = 0; i < cfg->x_points; i++) {
		nu->nux[i] = cfg->int_scale * cfg->x_step * cfg->alpha;
		nu->dnux[i] = exp(-nu->nux[i]);
	}

	for (i = 0; i < cfg->y_points; i++) {
		nu->nuy[i] = cfg->int_scale * cfg->y_step * cfg->alpha;
		nu->dnuy[i] = exp(-nu->nuy[i]);
	}

	for (i = 0; i < cfg->z_points; i++) {
		nu->nuz[i] = cfg->int_scale * cfg->z_step * cfg->alpha;
		nu->dnuz[i] = exp(-nu->nuz[i]);
	}
}

/* setuplump_vweight : setup the vweight lump */
void setuplump_vweight(struct lump_header_t *hdr, struct lump_vweight_t *vw)
{
	struct moltcfg_t *cfg;
	f64 alpha;
	s64 i;

	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);
	alpha = cfg->alpha;

	vw->magic = MOLTLUMP_MAGIC;
	// first, fill the arrays with our values

	for (i = 0; i < cfg->x_points_inc; i++) {
		vw->vrx[i] = exp((-alpha) * cfg->int_scale * i);
		vw->vlx[i] = exp((-alpha) * cfg->int_scale * i);
	}

	for (i = 0; i < cfg->y_points_inc; i++) {
		vw->vry[i] = exp((-alpha) * cfg->int_scale * i);
		vw->vly[i] = exp((-alpha) * cfg->int_scale * i);
	}

	for (i = 0; i < cfg->z_points_inc; i++) {
		vw->vrz[i] = exp((-alpha) * cfg->int_scale * i);
		vw->vlz[i] = exp((-alpha) * cfg->int_scale * i);
	}

}

/* setuplump_wweight : setup the wweight lump */
void setuplump_wweight(struct lump_header_t *hdr, struct lump_wweight_t *ww)
{
	struct moltcfg_t *cfg;
	struct lump_nu_t *nu;

	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);
	nu  = io_lumpgetbase(hdr, MOLTLUMP_NU);

	// perform all of our get_exp_weights
	ww->magic = MOLTLUMP_MAGIC;
	get_exp_weights(nu->nux, ww->xl_weight, ww->xr_weight,
			cfg->x_points, cfg->space_acc);
	get_exp_weights(nu->nuy, ww->yl_weight, ww->yr_weight,
			cfg->y_points, cfg->space_acc);
	get_exp_weights(nu->nuz, ww->zl_weight, ww->zr_weight,
			cfg->z_points, cfg->space_acc);
}

/* setuplump_mesh : setup the "problem state" lump */
void setuplump_mesh(struct lump_header_t *hdr, struct lump_mesh_t *state)
{
	// write zeroes to the mesh
	s32 x, y, z, i;
	struct moltcfg_t *cfg;
	struct lump_t *lump;
	struct lump_mesh_t *mesh;
	f64 fx, fy, fz;

	lump = &hdr->lump[MOLTLUMP_MESH];
	mesh = io_lumpgetbase(hdr, MOLTLUMP_MESH);
	cfg = io_lumpgetbase(hdr, MOLTLUMP_CONFIG);

	for (i = 0; i < lump->lumpsize / lump->elemsize; i++) {
		mesh[i].magic = MOLTLUMP_MAGIC;
	}

	memset(((char *)hdr) + lump->offset, 0, lump->lumpsize);

	// we only want to setup the first (0th) umesh applies function f
	for (z = 0; z < cfg->z_points_inc; z++) {
		for (y = 0; y < cfg->y_points_inc; y++) {
			for (x = 0; x < cfg->x_points_inc; x++) {
				i = IDX3D(x, y, z, cfg->y_points_inc, cfg->z_points_inc);

				fx = pow((x * MOLT_X_STEP * cfg->int_scale - 1), 2);
				fy = pow((y * MOLT_Y_STEP * cfg->int_scale - 1), 2);
				fz = pow((z * MOLT_Z_STEP * cfg->int_scale - 1), 2);

				mesh->umesh[i] = exp(-fx - fy - fz);
			}
		}
	}
}

void debug(void *hunk)
{
	struct lump_header_t *hdr;
	struct moltcfg_t *cfg;
	struct lump_nu_t *nu;
	struct lump_vweight_t *vw;
	struct lump_wweight_t *ww;
	struct lump_mesh_t *mesh;
	s32 i, x, y, z;

	hdr = hunk;

	cfg = io_lumpgetbase(hunk, MOLTLUMP_CONFIG);

	io_fprintf(stdout, "header : 0x%8x\tver : %d\t type : 0x%02x\n",
			hdr->magic, hdr->version, hdr->type);

#if 1
	for (i = 0; i < MOLTLUMP_TOTAL; i++) {
		io_fprintf(stdout,
				"lump[%d] off : 0x%08lx\tlumpsz : 0x%08lx\telemsz : 0x%08lx\n",
				i, hdr->lump[i].offset,
				hdr->lump[i].lumpsize, hdr->lump[i].elemsize);
	}
#endif

#if 1
	nu = io_lumpgetbase(hunk, MOLTLUMP_NU);
	for (i = 0; i < cfg->x_points; i++) {
		printf("nux [%03d] : %4.6e\n", i, nu->nux[i]);
	}

	for (i = 0; i < cfg->y_points; i++) {
		printf("nuy [%03d] : %4.6e\n", i, nu->nuy[i]);
	}

	for (i = 0; i < cfg->z_points; i++) {
		printf("nuz [%03d] : %4.6e\n", i, nu->nuz[i]);
	}
#endif
#if 1
	vw = io_lumpgetbase(hunk, MOLTLUMP_VWEIGHT);
	for (i = 0; i < cfg->x_points_inc; i++) {
		printf("vlx[%03d] : %4e\t vrx[%03d] : %4e\n",
				i, vw->vlx[i], i, vw->vrx[i]);
	}

	for (i = 0; i < cfg->y_points_inc; i++) {
		printf("vly[%03d] : %4e\t vry[%03d] : %4e\n",
				i, vw->vly[i], i, vw->vry[i]);
	}

	for (i = 0; i < cfg->z_points_inc; i++) {
		printf("vlz[%03d] : %4e\t vrz[%03d] : %4e\n",
				i, vw->vlz[i], i, vw->vrz[i]);
	}
#endif

#if 1
	ww = io_lumpgetbase(hunk, MOLTLUMP_WWEIGHT);
	for (i = 0; i < cfg->x_points * (cfg->space_acc + 1); i++) {
		printf("wxl[%03d] : % 04.6e\twxr[%03d] : % 04.6e\n",
				i, ww->xl_weight[i], i, ww->xr_weight[i]);
	}

	printf("wx compare : %d\n",
			memcmp(ww->xl_weight, ww->xr_weight, sizeof(ww->xl_weight)));

	for (i = 0; i < cfg->y_points * (cfg->space_acc + 1); i++) {
		printf("wyl[%03d] : % 04.6e\twyr[%03d] : % 04.6e\n",
				i, ww->yl_weight[i], i, ww->yr_weight[i]);
	}

	printf("wy compare : %d\n",
			memcmp(ww->yl_weight, ww->yr_weight, sizeof(ww->yl_weight)));

	for (i = 0; i < cfg->x_points * (cfg->space_acc + 1); i++) {
		printf("wzl[%03d] : % 04.6e\twzr[%03d] : % 04.6e\n",
				i, ww->zl_weight[i], i, ww->zr_weight[i]);
	}

	printf("wz compare : %d\n",
			memcmp(ww->zl_weight, ww->zr_weight, sizeof(ww->zl_weight)));
#endif

#if 1
	mesh = io_lumpgetbase(hunk, MOLTLUMP_MESH);
	printf("Magic : 0x%x\n", mesh->magic);
	for (z = 0; z < cfg->z_points_inc; z++) {
		for (y = 0; y < cfg->y_points_inc; y++) {
			for (x = 0; x < cfg->x_points_inc; x++) {
				i = IDX3D(x, y, z, cfg->y_points_inc, cfg->z_points_inc);
				printf("[%d] umesh[%d,%d,%d] : %6.16lf\n", i, x, y, z, mesh->umesh[i]);
			}
		}
	}
#endif
}

