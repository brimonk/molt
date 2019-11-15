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
 *   1. Initializes Storage through the sys_* routines
 *   2. Initializes problem state through the setuplump_* routines
 *   3. Kicks off MOLT Timesteps
 *   4. Synchronizes after those timesteps
 *
 * TODO (Brian)
 * 1. Config File with 'config.h' providing defaults.
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

#include "common.h"
#include "calcs.h"
#include "config.h"
#include "lump.h"
#include "sys.h"
#include "test.h"

#ifdef MOLT_VIEWER
#include "viewer.h"
#endif

struct user_cfg_t {
	s64 isset;
	s64 t_start;
	s64 t_stop;
	s64 t_step;
	s64 x_start;
	s64 x_stop;
	s64 x_step;
	s64 y_start;
	s64 y_stop;
	s64 y_step;
	s64 z_start;
	s64 z_stop;
	s64 z_step;
	f64 timescale;
	f64 spacescale;
	f64 beta;
	f64 alpha;
	char *initamp;
	char *initvel;
};

/* parse_config : parses the config file */
void parse_config(struct user_cfg_t *usercfg, char *file);

void do_simulation();
void do_custom_simulation(void *lib);

void parse_config(struct user_cfg_t *usercfg, char *file);

/* lump setup functions */
void setup_simulation(void **base, u64 *size, int fd, struct user_cfg_t *usercfg);
u64 setup_lumptable(struct lump_header_t *hdr);
void setuplump_cfg(struct lump_header_t *hdr, struct molt_cfg_t *cfg, struct user_cfg_t *usercfg);
void setuplump_run(struct lump_runinfo_t *run);
void setuplump_efield(struct lump_header_t *hdr, struct lump_efield_t *efield);
void setuplump_pfield(struct lump_header_t *hdr, struct lump_pfield_t *pfield);
void setuplump_vweight(struct lump_header_t *hdr, struct lump_vweight_t *vw);
void setuplump_wweight(struct lump_header_t *hdr, struct lump_wweight_t *ww);
void setuplump_umesh(struct lump_header_t *hdr, struct lump_mesh_t *state);

void setupstate_print(void *hunk);
s32 lump_magiccheck(void *hunk);

void print_help(char *prog);

#define USAGE "USAGE : %s [--viewer] [--custom <customlib>] [--nosim] [--test] [-v] [-h] outfile\n"

#define FLAG_VERBOSE 0x01
#define FLAG_VIEWER  0x02
#define FLAG_SIM     0x04
#define FLAG_TEST    0x08
#define FLAG_CUSTOM  0x10
#define FLAG_USERCFG 0x20

#define DEFAULT_FLAGS (FLAG_SIM)

int main(int argc, char **argv)
{
	char *fname, *s, **targv, targc;
	u32 flags;
	void *lib;
	char *libname;
	struct user_cfg_t usercfg;
	char *usercfgfile;
	int rc;

	memset(&usercfg, 0, sizeof usercfg);

	flags = DEFAULT_FLAGS;
	targc = argc;
	targv = argv;

	// parse arguments
	while (--targc > 0 && (*++targv)[0] == '-') {
		s = targv[0] + 1;

		if (strcmp(s, "-viewer") == 0) {
			flags |= FLAG_VIEWER;
		} else if (strcmp(s, "-nosim") == 0) {
			flags &= ~FLAG_SIM;
		} else if (strcmp(s, "-custom") == 0) {
			flags |= FLAG_CUSTOM;
			libname = *(++targv);
			targc--;
		} else if (strcmp(s, "-test") == 0) {
			flags |= FLAG_TEST;
		} else if (strcmp(s, "-config") == 0) {
			flags |= FLAG_USERCFG;
			usercfgfile = *(++targv);
			targc--;
		} else {
			switch (*s) {
			case 'v':
				flags |= FLAG_VERBOSE;
				break;
			case 'h':
				print_help(argv[0]);
				return 1;
			default:
				fprintf(stderr, "Illegal Option '%s'\n", s);
				targc = 0;
				print_help(argv[0]);
				return 1;
			}
		}
	}

	if (targc != 1) {
		fprintf(stderr, USAGE, argv[0]);
		return 1;
	}

	if (flags & FLAG_TEST) { // execute our test cases (test.c)
		testfunc();
		exit(0);
	}

	if (flags & FLAG_USERCFG) { // read and parse our user config
		parse_config(&usercfg, usercfgfile);
	}

	if (flags & FLAG_CUSTOM) { // open and load our custom library
		lib = sys_libopen(libname);

		if (lib == NULL) {
			flags &= ~FLAG_CUSTOM; // ?
			fprintf(stderr, "Did you mean to use './' for a library in the current directory?\n");
			exit(1);
		} else {
			fprintf(stderr, "Loading lib [%s]\n", libname);
		}
	}

	rc = lump_open(targv[0]);

	if (flags & FLAG_SIM) {
		if (flags & FLAG_CUSTOM) {
			do_custom_simulation(lib);
		} else {
			do_simulation();
		}
	}

	if (flags & FLAG_VERBOSE) {
		// TODO (brian) replace this with the new lump walking magic
	}

#ifdef MOLT_VIEWER
	if (flags & FLAG_VIEWER) {
		v_run(hunk, hunksize, fd, NULL);
	}
#endif

	if (flags & FLAG_CUSTOM) {
		sys_libclose(lib);
	}

	lump_close();

	free(usercfg.initamp);
	free(usercfg.initvel);

	return 0;
}

/* do_simulation : actually does the simulating */
void do_simulation()
{
#if 0
	struct molt_cfg_t *cfg;
	struct lump_runinfo_t *run;
	struct lump_vweight_t *vw_lump;
	struct lump_wweight_t *ww_lump;
	struct lump_vmesh_t *vmesh;
	struct lump_umesh_t *umesh, *curr;

	u32 firststep_flg, normal_flg;

	pdvec3_t vol;
	pdvec6_t vw, ww;

	cfg     = sys_lumpgetbase(hunk, MOLTLUMP_CONFIG);
	run     = sys_lumpgetbase(hunk, MOLTLUMP_RUNINFO);
	vw_lump = sys_lumpgetbase(hunk, MOLTLUMP_VWEIGHT);
	ww_lump = sys_lumpgetbase(hunk, MOLTLUMP_WWEIGHT);
	vmesh   = sys_lumpgetbase(hunk, MOLTLUMP_VMESH);
	umesh   = sys_lumpgetbase(hunk, MOLTLUMP_UMESH);

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

	molt_step(cfg, vol, vw, ww, firststep_flg);

	for (run->t_idx = 1; run->t_idx < run->t_total; run->t_idx++) {
		curr = umesh + run->t_idx;

		// ensure that vol is filled with the correct parameters
		vol[MOLT_VOL_NEXT] = (curr + 1)->data;
		vol[MOLT_VOL_CURR] = (curr    )->data;
		vol[MOLT_VOL_PREV] = (curr - 1)->data;

		molt_step(cfg, vol, vw, ww, normal_flg);

		// save off whatever fields are required
	}

	// TODO (brian) move somewhere else
	molt_cfg_free_workstore(cfg);
#endif
}

/* do_custom_simulation : setsup and invokes the custom MOLT routines */
void do_custom_simulation(void *lib)
{
#if 0
	struct molt_custom_t custom;
	struct molt_cfg_t *cfg;
	struct lump_runinfo_t *run;
	struct lump_vweight_t *vw_lump;
	struct lump_wweight_t *ww_lump;
	struct lump_vmesh_t *vmesh;
	struct lump_umesh_t *umesh, *curr;

	int rc;

	memset(&custom, 0, sizeof custom);

	// TODO (brian) pull the functions we can from our handle
	custom.func_open  = sys_libsym(lib, "molt_custom_open");
	custom.func_close = sys_libsym(lib, "molt_custom_close");
	custom.func_sweep = sys_libsym(lib, "molt_custom_sweep");
	custom.func_reorg = sys_libsym(lib, "molt_custom_reorg");

	cfg     = sys_lumpgetbase(hunk, MOLTLUMP_CONFIG);
	run     = sys_lumpgetbase(hunk, MOLTLUMP_RUNINFO);
	vw_lump = sys_lumpgetbase(hunk, MOLTLUMP_VWEIGHT);
	ww_lump = sys_lumpgetbase(hunk, MOLTLUMP_WWEIGHT);
	vmesh   = sys_lumpgetbase(hunk, MOLTLUMP_VMESH);
	umesh   = sys_lumpgetbase(hunk, MOLTLUMP_UMESH);

	// setup our custom params
	custom.vlx = vw_lump->vlx;
	custom.vrx = vw_lump->vrx;
	custom.vly = vw_lump->vly;
	custom.vry = vw_lump->vry;
	custom.vlz = vw_lump->vlz;
	custom.vrz = vw_lump->vrz;

	custom.wlx = ww_lump->xl_weight;
	custom.wrx = ww_lump->xr_weight;
	custom.wly = ww_lump->yl_weight;
	custom.wry = ww_lump->yr_weight;
	custom.wlz = ww_lump->zl_weight;
	custom.wrz = ww_lump->zr_weight;

	custom.cfg = cfg;

	// setup the volume info for the FIRST step
	custom.next = (umesh + 1)->data;
	custom.curr = umesh->data;
	custom.prev = vmesh->data;

	rc = custom.func_open(&custom);

	molt_step_custom(&custom, MOLT_FLAG_FIRSTSTEP);

	for (run->t_idx = 1; run->t_idx < run->t_total; run->t_idx++) {
		curr = umesh + run->t_idx;

		custom.next = (curr + 1)->data;
		custom.curr = (curr    )->data;
		custom.prev = (curr - 1)->data;

		molt_step_custom(&custom, 0);

		// save off whatever fields are required
	}

	rc = custom.func_close(&custom);

	// TODO (brian) move somewhere else
	molt_cfg_free_workstore(cfg);
#endif
}

#if 0 // fixing storage system

/* setup_simulation : sets up simulation based on config.h */
void setup_simulation(void **base, u64 *size, int fd, struct user_cfg_t *usercfg)
{
	struct lump_header_t tmpheader;

	*size = setup_lumptable(&tmpheader);

	// resize the file (if needed (probably always needed))
	if (sys_getsize() < *size) {
		if (sys_resize(fd, *size) < 0) {
			fprintf(stderr, "Couldn't get space. Quitting\n");
			exit(1);
		}
	}

	// finally, memory map and zero out the file
	*base = sys_mmap(fd, *size);
	memset(*base, 0, *size);

	sys_lumpcheck(*base);

	// ensure we copy our lump header into the mapped memory
	memcpy(*base, &tmpheader, sizeof(tmpheader));

	printf("file size %ld\n", *size);

	setuplump_cfg(*base, sys_lumpgetbase(*base, MOLTLUMP_CONFIG), usercfg);
	setuplump_run(sys_lumpgetbase(*base, MOLTLUMP_RUNINFO));
	setuplump_efield(*base, sys_lumpgetbase(*base, MOLTLUMP_EFIELD));
	setuplump_pfield(*base, sys_lumpgetbase(*base, MOLTLUMP_PFIELD));
	setuplump_vweight(*base, sys_lumpgetbase(*base, MOLTLUMP_VWEIGHT));
	setuplump_wweight(*base, sys_lumpgetbase(*base, MOLTLUMP_WWEIGHT));
	setuplump_umesh(*base, sys_lumpgetbase(*base, MOLTLUMP_UMESH));
}

/* setup_lumptable : fills out the lump_header_t that gets passed in */
u64 setup_lumptable(struct lump_header_t *hdr)
{
	// NOTE (brian) returns the size, in bytes, the file needs to be
	s32 curr_lump;
	u64 curr_offset;

	/* begin the setup by setting up our lump header and our lump directory */

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
	hdr->lump[curr_lump].lumpsize = hdr->lump[curr_lump].elemsize;
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

/* setuplump_cfg : setup the config lump */
void setuplump_cfg(struct lump_header_t *hdr, struct molt_cfg_t *cfg, struct user_cfg_t *ucfg)
{
	s32 tpoints, xpoints, ypoints, zpoints;

	memset(cfg, 0, sizeof *cfg);

	// NOTE (brian)
	// we assume that ALL of the parameters are set in the config file
	if (ucfg->isset) {
		molt_cfg_set_spacescale(cfg, ucfg->spacescale);
		molt_cfg_set_timescale(cfg, ucfg->timescale);

		tpoints = (ucfg->t_stop - ucfg->t_start) / 2;
		xpoints = (ucfg->x_stop - ucfg->x_start) / 2;
		ypoints = (ucfg->y_stop - ucfg->y_start) / 2;
		zpoints = (ucfg->z_stop - ucfg->z_start) / 2;

		molt_cfg_dims_t(cfg, ucfg->t_start, ucfg->t_stop, ucfg->t_step, tpoints, tpoints + 1);
		molt_cfg_dims_x(cfg, ucfg->x_start, ucfg->x_stop, ucfg->x_step, xpoints, xpoints + 1);
		molt_cfg_dims_y(cfg, ucfg->y_start, ucfg->y_stop, ucfg->y_step, ypoints, ypoints + 1);
		molt_cfg_dims_z(cfg, ucfg->z_start, ucfg->z_stop, ucfg->z_step, zpoints, zpoints + 1);

		molt_cfg_set_accparams(cfg, ucfg->spacescale, ucfg->timescale);
	} else {
		molt_cfg_set_spacescale(cfg, MOLT_SPACESCALE);
		molt_cfg_set_timescale(cfg, MOLT_TIMESCALE);

		molt_cfg_dims_t(cfg, MOLT_T_START, MOLT_T_STOP, MOLT_T_STEP, MOLT_T_POINTS, MOLT_T_PINC);
		molt_cfg_dims_x(cfg, MOLT_X_START, MOLT_X_STOP, MOLT_X_STEP, MOLT_X_POINTS, MOLT_X_PINC);
		molt_cfg_dims_y(cfg, MOLT_Y_START, MOLT_Y_STOP, MOLT_Y_STEP, MOLT_Y_POINTS, MOLT_Y_PINC);
		molt_cfg_dims_z(cfg, MOLT_Z_START, MOLT_Z_STOP, MOLT_Z_STEP, MOLT_Z_POINTS, MOLT_Z_PINC);

		molt_cfg_set_accparams(cfg, MOLT_SPACEACC, MOLT_TIMEACC);
	}

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
	ivec3_t start, stop;
	ivec3_t dim;

	memset(vw, 0, sizeof *vw);

	cfg = sys_lumpgetbase(hdr, MOLTLUMP_CONFIG);

	vw->meta.magic = MOLTLUMP_MAGIC;

	molt_cfg_parampull_xyz(cfg, start, MOLT_PARAM_START);
	molt_cfg_parampull_xyz(cfg, stop, MOLT_PARAM_STOP);
	molt_cfg_parampull_xyz(cfg, dim, MOLT_PARAM_PINC);

	for (i = 0; i < dim[0]; i++) // vlx
		vw->vlx[i] = exp((-cfg->alpha) * cfg->space_scale * (i - start[0]));

	for (i = 0; i < dim[0]; i++) // vrx
		vw->vrx[i] = exp((-cfg->alpha) * cfg->space_scale * (stop[0] - i));

	for (i = 0; i < dim[1]; i++) // vly
		vw->vly[i] = exp((-cfg->alpha) * cfg->space_scale * (i - start[1]));

	for (i = 0; i < dim[1]; i++) // vry
		vw->vry[i] = exp((-cfg->alpha) * cfg->space_scale * (stop[1] - i));

	for (i = 0; i < dim[2]; i++) // vlz
		vw->vrz[i] = exp((-cfg->alpha) * cfg->space_scale * (i - start[2]));

	for (i = 0; i < dim[2]; i++) // vrz
		vw->vlz[i] = exp((-cfg->alpha) * cfg->space_scale * (stop[2] - i));
}

/* setuplump_wweight : setup the wweight lump */
void setuplump_wweight(struct lump_header_t *hdr, struct lump_wweight_t *ww)
{
	struct molt_cfg_t *cfg;

	cfg = sys_lumpgetbase(hdr, MOLTLUMP_CONFIG);

	ww->meta.magic = MOLTLUMP_MAGIC;

	// perform all of our get_exp_weights
	get_exp_weights(cfg->nu[0], ww->xl_weight, ww->xr_weight, cfg->x_params[MOLT_PARAM_POINTS], cfg->spaceacc);
	get_exp_weights(cfg->nu[1], ww->yl_weight, ww->yr_weight, cfg->y_params[MOLT_PARAM_POINTS], cfg->spaceacc);
	get_exp_weights(cfg->nu[2], ww->zl_weight, ww->zr_weight, cfg->z_params[MOLT_PARAM_POINTS], cfg->spaceacc);
}

void setuplump_vmesh(struct lump_header_t *hdr, struct lump_vmesh_t *vmesh)
{
	// NOTE (brian) not used, initial wave velocity setup
	// (look in setuplump_umesh)
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
	umesh = sys_lumpgetbase(hdr, MOLTLUMP_UMESH);
	vmesh = sys_lumpgetbase(hdr, MOLTLUMP_VMESH);
	cfg = sys_lumpgetbase(hdr, MOLTLUMP_CONFIG);

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
#if 0
				vmesh->data[i] = MOLT_TISSUESPEED * 52.0 / (xpoints * cfg->space_scale)
					* initial_scale(cfg, x, xpoints) * umesh->data[i];
#else
				vmesh->data[i] = MOLT_TISSUESPEED * 2.0 * 13.0 * 2.0 / (xpoints * cfg->space_scale)
					* initial_scale(cfg, x, xpoints) * umesh->data[i];
#endif
			}
		}
	}
}

/* setupstate_print : perform lots of printf debugging */
void setupstate_print(void *hunk)
{
	struct lump_header_t *hdr;
	struct molt_cfg_t *cfg;
	struct lump_vweight_t *vw;
	struct lump_wweight_t *ww;
	struct lump_vmesh_t *vmesh;
	struct lump_umesh_t *umesh;
	s32 i;
	ivec2_t xweight_dim, yweight_dim, zweight_dim;
	ivec3_t umesh_dim, vmesh_dim;
	char buf[BUFLARGE];

	hdr = hunk;

	cfg = sys_lumpgetbase(hunk, MOLTLUMP_CONFIG);

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
	vw = sys_lumpgetbase(hunk, MOLTLUMP_VWEIGHT);
	LOG1D(vw->vlx, cfg->x_params[MOLT_PARAM_PINC], "VWEIGHT-VLX");
	LOG1D(vw->vrx, cfg->x_params[MOLT_PARAM_PINC], "VWEIGHT-VRX");
	LOG1D(vw->vly, cfg->y_params[MOLT_PARAM_PINC], "VWEIGHT-VLY");
	LOG1D(vw->vry, cfg->y_params[MOLT_PARAM_PINC], "VWEIGHT-VRY");
	LOG1D(vw->vlz, cfg->z_params[MOLT_PARAM_PINC], "VWEIGHT-VLZ");
	LOG1D(vw->vrz, cfg->z_params[MOLT_PARAM_PINC], "VWEIGHT-VRZ");

	// configure the weights, and log wweights
	ww = sys_lumpgetbase(hunk, MOLTLUMP_WWEIGHT);

	// setup the weights for passing
#if 0
	Vec2Set(xweight_dim, cfg->spaceacc + 1, cfg->x_params[MOLT_PARAM_POINTS]);
	Vec2Set(yweight_dim, cfg->spaceacc + 1, cfg->y_params[MOLT_PARAM_POINTS]);
	Vec2Set(zweight_dim, cfg->spaceacc + 1, cfg->z_params[MOLT_PARAM_POINTS]);
#else
	Vec2Set(xweight_dim, cfg->x_params[MOLT_PARAM_POINTS], cfg->spaceacc + 1);
	Vec2Set(yweight_dim, cfg->y_params[MOLT_PARAM_POINTS], cfg->spaceacc + 1);
	Vec2Set(zweight_dim, cfg->z_params[MOLT_PARAM_POINTS], cfg->spaceacc + 1);
#endif

	LOG2D(ww->xl_weight, xweight_dim, "WWEIGHT-XL");
	LOG2D(ww->xr_weight, xweight_dim, "WWEIGHT-XR");
	LOG2D(ww->yl_weight, yweight_dim, "WWEIGHT-YL");
	LOG2D(ww->yr_weight, yweight_dim, "WWEIGHT-YR");
	LOG2D(ww->zl_weight, zweight_dim, "WWEIGHT-ZL");
	LOG2D(ww->zr_weight, zweight_dim, "WWEIGHT-ZR");

	// log out the initial state of the wave
	vmesh = sys_lumpgetbase(hunk, MOLTLUMP_VMESH);
	molt_cfg_parampull_xyz(cfg, vmesh_dim, MOLT_PARAM_POINTS);
	LOG3D(vmesh->data, vmesh_dim, "VMESH");

	// log out the 3d volume
	umesh = sys_lumpgetbase(hunk, MOLTLUMP_UMESH);
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
		single_elem_magic[i] = sys_lumpgetmeta(hunk, single_elem_lump[i])->magic;
	}


	// spin through and check them all
	for (i = 0; i < 5; i++) {
		if (single_elem_magic[i] != MOLTLUMP_MAGIC) {
			fprintf(stderr, "LUMP %d HAS MAGIC %#08X, should be %#08X\n",
					single_elem_lump[i], single_elem_magic[i], MOLTLUMP_MAGIC);
			rc += 1;
		}
	}

	efield = sys_lumpgetbase(hunk, MOLTLUMP_EFIELD);
	pfield = sys_lumpgetbase(hunk, MOLTLUMP_PFIELD);
	umesh   = sys_lumpgetbase(hunk, MOLTLUMP_UMESH);

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

#endif // fixing storage system

/* print_help : prints some help text */
void print_help(char *prog)
{
#define HELP_EXAMPLE "EX    : %s --viewer -v output.db > log.txt\n"
	fprintf(stderr, "--config <file> specifies a custom config file to load experiment parameters from\n");
	fprintf(stderr, "--custom <file> specifies a custom library to load sweep and reorg functions from\n");
	fprintf(stderr, "--nosim         runs everything BUT the simulation itself\n");
	fprintf(stderr, "--test          runs testing procedures before the simulation\n");
	fprintf(stderr, "--viewer        runs the viewer program\n");
	fprintf(stderr, "-h              prints this help text\n");
	fprintf(stderr, "-v              displays verbose simulation info\n");
	fprintf(stderr, USAGE, prog);
	fprintf(stderr, HELP_EXAMPLE, prog);
}

/* parse_config : parses the config file */
void parse_config(struct user_cfg_t *usercfg, char *file)
{
	FILE *fp;
	char *key, *val;
	char buf[BUFLARGE];

	memset(usercfg, 0, sizeof(*usercfg));

	fp = fopen(file, "r");

	if (!fp)
		return;

	while (fgets(buf, sizeof buf, fp) == buf) {
		buf[strlen(buf) - 1] = 0;

		if (strlen(buf) == 0 || buf[0] == '#') {
			continue; // skip blank lines and comments
		}

		key = buf;
		val = strchr(key, ':');
		*val = 0;
		val++;

		while (*key == ' ') {
			key++;
		}
		while (*val == ' ') {
			val++;
		}

		if (strcmp("t_start", key) == 0) {
			usercfg->t_start = atof(val);
		} else if (strcmp("t_stop", key) == 0) {
			usercfg->t_stop = atof(val);
		} else if (strcmp("t_step", key) == 0) {
			usercfg->t_step = atof(val);
		} else if (strcmp("x_start", key) == 0) {
			usercfg->x_start = atof(val);
		} else if (strcmp("x_step", key) == 0) {
			usercfg->x_step = atof(val);
		} else if (strcmp("x_stop", key) == 0) {
			usercfg->x_stop = atof(val);
		} else if (strcmp("y_start", key) == 0) {
			usercfg->y_start = atof(val);
		} else if (strcmp("y_step", key) == 0) {
			usercfg->y_step = atof(val);
		} else if (strcmp("y_stop", key) == 0) {
			usercfg->y_stop = atof(val);
		} else if (strcmp("z_start", key) == 0) {
			usercfg->z_start = atof(val);
		} else if (strcmp("z_step", key) == 0) {
			usercfg->z_step = atof(val);
		} else if (strcmp("z_stop", key) == 0) {
			usercfg->z_stop = atof(val);
		} else if (strcmp("timescale", key) == 0) {
			usercfg->timescale = atof(val);
		} else if (strcmp("spacescale", key) == 0) {
			usercfg->spacescale = atof(val);
		} else if (strcmp("beta", key) == 0) {
			usercfg->beta = atof(val);
		} else if (strcmp("alpha", key) == 0) {
			usercfg->alpha = atof(val);
		} else if (strcmp("initamp", key) == 0) {
			usercfg->initamp = strdup(val);
		} else if (strcmp("initvel", key) == 0) {
			usercfg->initvel = strdup(val);
		}
	}

	usercfg->isset = 1;
}

