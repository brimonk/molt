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
#include "thpool.h"
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
	f64 scale_time;
	f64 scale_space;
	s64 acc_time;
	s64 acc_space;
	f64 beta;
	f64 alpha;
	char *initamp;
	char *initvel;
};

struct configthreadargs_t {
	FILE *fp;
	struct user_cfg_t *usercfg;
	ivec3_t dim;
	dvec3_t fdim;
	f64 *vol;
};

/* parse_config : parses the config file */
int parse_config(struct user_cfg_t *usercfg, char *file);

/* do_simulation : actually does the simulating */
int do_simulation();

/* do_custom_simulation : actually does the simulating, with custom functions */
int do_custom_simulation(void *lib);

/* setup : sets up the simulation */
int setup(struct user_cfg_t *usercfg);

/* setup_config : setup the config lump */
int setup_config(struct user_cfg_t *ucfg);

/* setup_vweight : sets up the v weights */
int setup_vweight();

/* setup_wweight : sets up the w weights */
int setup_wweight();

/* setup_initvelocity : inits velocity, uses custom command if available */
int setup_initvelocity(struct user_cfg_t *usercfg);
/* setup_initvelocity_config : sets up the initial velocity, from the config */
int setup_initvelocity_config(f64 *initvelocity, struct user_cfg_t *usercfg);

/* setup_initamplitude : inits amplitude, uses custom command if available */
int setup_initamplitude(struct user_cfg_t *usercfg);
/* setup_initamplitude_config : sets up the initial amplitude from the config */
int setup_initamplitude_config(f64 *initamplitude, struct user_cfg_t *usercfg);

/* setup_customprog_write : function for threading setup */
void *setup_customprog_write(void *arg);
/* setup_customprog_read : function for setup reading (parent <- child) */
void *setup_customprog_read(void *arg);

/* dump_lumps : prints a dump of all the lumps in the lump system */
int dump_lumps();

void print_help(char *prog);

#define MOLTSTR_CONFIG "CONFIG"
#define MOLTSTR_VLX    "VLX"
#define MOLTSTR_VRX    "VRX"
#define MOLTSTR_VLY    "VLY"
#define MOLTSTR_VRY    "VRY"
#define MOLTSTR_VLZ    "VLZ"
#define MOLTSTR_VRZ    "VRZ"
#define MOLTSTR_WLX    "WLX"
#define MOLTSTR_WRX    "WRX"
#define MOLTSTR_WLY    "WLY"
#define MOLTSTR_WRY    "WRY"
#define MOLTSTR_WLZ    "WLZ"
#define MOLTSTR_WRZ    "WRZ"
#define MOLTSTR_VEL    "VEL"
#define MOLTSTR_AMP    "AMP"

#define USAGE "USAGE : %s [--config <config>] [--viewer] [--custom <customlib>] [--nosim] [--test] [-v] [-h] outfile\n"

#define FLAG_VERBOSE 0x01
#define FLAG_VIEWER  0x02
#define FLAG_SIM     0x04
#define FLAG_TEST    0x08
#define FLAG_CUSTOM  0x10
#define FLAG_USERCFG 0x20

#define DEFAULT_FLAGS (FLAG_SIM)

int main(int argc, char **argv)
{
	char *s, **targv, targc;
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

	rc = setup(&usercfg);
	if (rc < 0) {
		exit(1); // we failed setup somehow
	}

	if (flags & FLAG_SIM) {
		if (flags & FLAG_CUSTOM) {
			do_custom_simulation(lib);
		} else {
			do_simulation();
		}
	}

	if (flags & FLAG_VERBOSE) {
		dump_lumps();
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

#define PRINTANDFAIL(x)  ({fprintf(stderr, "ERR : %s\n", (x)); return -1;})

/* do_simulation : actually does the simulating */
int do_simulation()
{
	struct molt_cfg_t config;
	pdvec6_t vw, ww;
	pdvec3_t vol;
	f64 *prev, *curr, *next;
	u32 flags;
	u64 elems, i, volumebytes;;
	ivec3_t pinc;
	ivec3_t points;
	int rc;

	rc = lump_read(MOLTSTR_CONFIG, 0, &config);
	if (rc < 0) { PRINTANDFAIL("couldn't read config from lump system"); }

	molt_cfg_parampull_xyz(&config, pinc, MOLT_PARAM_PINC);
	molt_cfg_parampull_xyz(&config, points, MOLT_PARAM_POINTS);

	elems = pinc[0] * (u64)pinc[1] * pinc[2];

	// get working memory for all of our data points we need

	vw[0] = calloc(sizeof(f64), pinc[0]);
	vw[1] = calloc(sizeof(f64), pinc[0]);
	vw[2] = calloc(sizeof(f64), pinc[1]);
	vw[3] = calloc(sizeof(f64), pinc[1]);
	vw[4] = calloc(sizeof(f64), pinc[2]);
	vw[5] = calloc(sizeof(f64), pinc[2]);

	ww[0] = calloc(sizeof(f64), points[0] * (config.spaceacc + 1));
	ww[1] = calloc(sizeof(f64), points[0] * (config.spaceacc + 1));
	ww[2] = calloc(sizeof(f64), points[1] * (config.spaceacc + 1));
	ww[3] = calloc(sizeof(f64), points[1] * (config.spaceacc + 1));
	ww[4] = calloc(sizeof(f64), points[2] * (config.spaceacc + 1));
	ww[5] = calloc(sizeof(f64), points[2] * (config.spaceacc + 1));

	prev = calloc(sizeof(f64), elems);
	curr = calloc(sizeof(f64), elems);
	next = calloc(sizeof(f64), elems);

	volumebytes = sizeof(f64) * elems;

	// now that we have memory, we can fully load all of our data
	rc = lump_read(MOLTSTR_VLX, 0, vw[0]);
	if (rc < 0) { PRINTANDFAIL("couldn't read VLX from lump system"); }
	rc = lump_read(MOLTSTR_VRX, 0, vw[1]);
	if (rc < 0) { PRINTANDFAIL("couldn't read VRX from lump system"); }
	rc = lump_read(MOLTSTR_VLY, 0, vw[2]);
	if (rc < 0) { PRINTANDFAIL("couldn't read VLY from lump system"); }
	rc = lump_read(MOLTSTR_VRY, 0, vw[3]);
	if (rc < 0) { PRINTANDFAIL("couldn't read VRY from lump system"); }
	rc = lump_read(MOLTSTR_VLZ, 0, vw[4]);
	if (rc < 0) { PRINTANDFAIL("couldn't read VLZ from lump system"); }
	rc = lump_read(MOLTSTR_VRZ, 0, vw[5]);
	if (rc < 0) { PRINTANDFAIL("couldn't read VRZ from lump system"); }

	rc = lump_read(MOLTSTR_WLX, 0, ww[0]);
	if (rc < 0) { PRINTANDFAIL("couldn't read WLX from lump system"); }
	rc = lump_read(MOLTSTR_WRX, 0, ww[1]);
	if (rc < 0) { PRINTANDFAIL("couldn't read WRX from lump system"); }
	rc = lump_read(MOLTSTR_WLY, 0, ww[2]);
	if (rc < 0) { PRINTANDFAIL("couldn't read WLY from lump system"); }
	rc = lump_read(MOLTSTR_WRY, 0, ww[3]);
	if (rc < 0) { PRINTANDFAIL("couldn't read WRY from lump system"); }
	rc = lump_read(MOLTSTR_WLZ, 0, ww[4]);
	if (rc < 0) { PRINTANDFAIL("couldn't read WLZ from lump system"); }
	rc = lump_read(MOLTSTR_WRZ, 0, ww[5]);
	if (rc < 0) { PRINTANDFAIL("couldn't read WRZ from lump system"); }

	rc = lump_read(MOLTSTR_VEL, 0, prev);
	if (rc < 0) { PRINTANDFAIL("couldn't read initial velocity from lump system"); }
	rc = lump_read(MOLTSTR_AMP, 0, curr);
	if (rc < 0) { PRINTANDFAIL("couldn't read initial amplitude from lump system"); }

	vol[0] = next;
	vol[1] = curr;
	vol[2] = prev;

	molt_cfg_set_workstore(&config);

	i = config.t_params[MOLT_PARAM_START];
	flags = MOLT_FLAG_FIRSTSTEP;

	do {
		molt_step(&config, vol, vw, ww, flags);

		rc = lump_write(MOLTSTR_AMP, sizeof(f64) * elems, next, NULL);

		memcpy(prev, curr, volumebytes);
		memcpy(curr, next, volumebytes);
		memset(next, 0, volumebytes);

		flags = 0;
		i += config.t_params[MOLT_PARAM_STEP];
	} while (i < config.t_params[MOLT_PARAM_STOP]);

	molt_cfg_free_workstore(&config);

	free(prev);
	free(curr);
	free(next);

	for (i = 0; i < 5; i++) {
		free(vw[i]);
		free(ww[i]);
	}

	return 0;
}

/* do_custom_simulation : setsup and invokes the custom MOLT routines */
int do_custom_simulation(void *lib)
{
	struct molt_cfg_t config;
	struct molt_custom_t custom;
	u32 flags;
	u64 elems, i, volumebytes;;
	ivec3_t pinc;
	ivec3_t points;
	int rc;

	rc = lump_read(MOLTSTR_CONFIG, 0, &config);
	if (rc < 0) { PRINTANDFAIL("couldn't read config from lump system"); }

	molt_cfg_parampull_xyz(&config, pinc,   MOLT_PARAM_PINC);
	molt_cfg_parampull_xyz(&config, points, MOLT_PARAM_POINTS);

	elems = pinc[0] * (u64)pinc[1] * pinc[2];

	// get working memory for all of our data points we need

	custom.vlx = calloc(sizeof(f64), pinc[0]);
	custom.vrx = calloc(sizeof(f64), pinc[0]);
	custom.vly = calloc(sizeof(f64), pinc[1]);
	custom.vry = calloc(sizeof(f64), pinc[1]);
	custom.vlz = calloc(sizeof(f64), pinc[2]);
	custom.vrz = calloc(sizeof(f64), pinc[2]);

	custom.wlx = calloc(sizeof(f64), points[0] * (config.spaceacc + 1));
	custom.wrx = calloc(sizeof(f64), points[0] * (config.spaceacc + 1));
	custom.wly = calloc(sizeof(f64), points[1] * (config.spaceacc + 1));
	custom.wry = calloc(sizeof(f64), points[1] * (config.spaceacc + 1));
	custom.wlz = calloc(sizeof(f64), points[2] * (config.spaceacc + 1));
	custom.wrz = calloc(sizeof(f64), points[2] * (config.spaceacc + 1));

	custom.prev  = calloc(sizeof(f64), elems);
	custom.curr  = calloc(sizeof(f64), elems);
	custom.next  = calloc(sizeof(f64), elems);

	volumebytes = sizeof(f64) * elems;

	// now that we have memory, we can fully load all of our data
	rc = lump_read(MOLTSTR_VLX, 0, custom.vlx);
	if (rc < 0) { PRINTANDFAIL("couldn't read VLX from lump system"); }
	rc = lump_read(MOLTSTR_VRX, 0, custom.vrx);
	if (rc < 0) { PRINTANDFAIL("couldn't read VRX from lump system"); }
	rc = lump_read(MOLTSTR_VLY, 0, custom.vly);
	if (rc < 0) { PRINTANDFAIL("couldn't read VLY from lump system"); }
	rc = lump_read(MOLTSTR_VRY, 0, custom.vry);
	if (rc < 0) { PRINTANDFAIL("couldn't read VRY from lump system"); }
	rc = lump_read(MOLTSTR_VLZ, 0, custom.vlz);
	if (rc < 0) { PRINTANDFAIL("couldn't read VLZ from lump system"); }
	rc = lump_read(MOLTSTR_VRZ, 0, custom.vlz);
	if (rc < 0) { PRINTANDFAIL("couldn't read VRZ from lump system"); }

	rc = lump_read(MOLTSTR_WLX, 0, custom.wlx);
	if (rc < 0) { PRINTANDFAIL("couldn't read WLX from lump system"); }
	rc = lump_read(MOLTSTR_WRX, 0, custom.wrx);
	if (rc < 0) { PRINTANDFAIL("couldn't read WRX from lump system"); }
	rc = lump_read(MOLTSTR_WLY, 0, custom.wly);
	if (rc < 0) { PRINTANDFAIL("couldn't read WLY from lump system"); }
	rc = lump_read(MOLTSTR_WRY, 0, custom.wry);
	if (rc < 0) { PRINTANDFAIL("couldn't read WRY from lump system"); }
	rc = lump_read(MOLTSTR_WLZ, 0, custom.wlz);
	if (rc < 0) { PRINTANDFAIL("couldn't read WLZ from lump system"); }
	rc = lump_read(MOLTSTR_WRZ, 0, custom.wrz);
	if (rc < 0) { PRINTANDFAIL("couldn't read WRZ from lump system"); }

	rc = lump_read(MOLTSTR_VEL, 0, custom.prev);
	if (rc < 0) { PRINTANDFAIL("couldn't read initial velocity from lump system"); }
	rc = lump_read(MOLTSTR_AMP, 0, custom.curr);
	if (rc < 0) { PRINTANDFAIL("couldn't read initial amplitude from lump system"); }

	custom.cfg = &config;

	molt_cfg_set_workstore(&config);

	// the final setup step is to load the custom functions from the lib
	custom.func_open  = sys_libsym(lib, "molt_custom_open");
	custom.func_close = sys_libsym(lib, "molt_custom_close");
	custom.func_sweep = sys_libsym(lib, "molt_custom_sweep");
	custom.func_reorg = sys_libsym(lib, "molt_custom_reorg");

	custom.func_open(&custom);

	i = config.t_params[MOLT_PARAM_START];
	flags = MOLT_FLAG_FIRSTSTEP;

	do {
		molt_step_custom(&custom, flags);

		rc = lump_write(MOLTSTR_AMP, sizeof(f64) * elems, custom.next, NULL);

		memcpy(custom.prev, custom.curr, volumebytes);
		memcpy(custom.curr, custom.next, volumebytes);
		memset(custom.next, 0, volumebytes);

		flags = 0;
		i += config.t_params[MOLT_PARAM_STEP];
	} while (i < config.t_params[MOLT_PARAM_STOP]);

	custom.func_close(&custom);

	molt_cfg_free_workstore(&config);

	free(custom.prev);
	free(custom.curr);
	free(custom.next);

	free(custom.vlx);
	free(custom.vrx);
	free(custom.vly);
	free(custom.vry);
	free(custom.vlz);
	free(custom.vrz);

	free(custom.wlx);
	free(custom.wrx);
	free(custom.wly);
	free(custom.wry);
	free(custom.wlz);
	free(custom.wrz);

	return 0;

}

/* setup : sets up the simulation */
int setup(struct user_cfg_t *usercfg)
{
	int rc;

	rc = setup_config(usercfg);
	if (rc < 0) {
		fprintf(stderr, "ERR : config setup failed!\n");
		return -1;
	}

	rc = setup_vweight();
	if (rc < 0) {
		fprintf(stderr, "ERR : v weight setup failed!\n");
		return -1;
	}

	rc = setup_wweight();
	if (rc < 0) {
		fprintf(stderr, "ERR : w weight setup failed!\n");
		return -1;
	}

	rc = setup_initvelocity(usercfg);
	if (rc < 0) {
		fprintf(stderr, "ERR : initial velocity setup failed!\n");
		return -1;
	}

	rc = setup_initamplitude(usercfg);
	if (rc < 0) {
		fprintf(stderr, "ERR : initial amplitude setup failed!\n");
		return -1;
	}

	return 0;
}

/* setup_config : setup the config lump */
int setup_config(struct user_cfg_t *ucfg)
{
	struct molt_cfg_t config;
	s32 tpoints, xpoints, ypoints, zpoints;

	memset(&config, 0, sizeof(config));

	// NOTE (brian)
	// we assume that ALL of the parameters are set in the config file
	if (ucfg->isset) {
		molt_cfg_set_spacescale(&config, ucfg->scale_space);
		molt_cfg_set_timescale(&config, ucfg->scale_time);

		tpoints = (ucfg->t_stop - ucfg->t_start) / ucfg->t_step;
		xpoints = (ucfg->x_stop - ucfg->x_start) / ucfg->x_step;
		ypoints = (ucfg->y_stop - ucfg->y_start) / ucfg->y_step;
		zpoints = (ucfg->z_stop - ucfg->z_start) / ucfg->z_step;

		molt_cfg_dims_t(&config, ucfg->t_start, ucfg->t_stop, ucfg->t_step, tpoints, tpoints + 1);
		molt_cfg_dims_x(&config, ucfg->x_start, ucfg->x_stop, ucfg->x_step, xpoints, xpoints + 1);
		molt_cfg_dims_y(&config, ucfg->y_start, ucfg->y_stop, ucfg->y_step, ypoints, ypoints + 1);
		molt_cfg_dims_z(&config, ucfg->z_start, ucfg->z_stop, ucfg->z_step, zpoints, zpoints + 1);

		molt_cfg_set_accparams(&config, ucfg->acc_space, ucfg->acc_time);
	} else {
		molt_cfg_set_spacescale(&config, MOLT_SPACESCALE);
		molt_cfg_set_timescale(&config, MOLT_TIMESCALE);

		molt_cfg_dims_t(&config, MOLT_T_START, MOLT_T_STOP, MOLT_T_STEP, MOLT_T_POINTS, MOLT_T_PINC);
		molt_cfg_dims_x(&config, MOLT_X_START, MOLT_X_STOP, MOLT_X_STEP, MOLT_X_POINTS, MOLT_X_PINC);
		molt_cfg_dims_y(&config, MOLT_Y_START, MOLT_Y_STOP, MOLT_Y_STEP, MOLT_Y_POINTS, MOLT_Y_PINC);
		molt_cfg_dims_z(&config, MOLT_Z_START, MOLT_Z_STOP, MOLT_Z_STEP, MOLT_Z_POINTS, MOLT_Z_PINC);

		molt_cfg_set_accparams(&config, MOLT_SPACEACC, MOLT_TIMEACC);
	}

	// compute alpha by hand because it relies on simulation specific params
	// (beta is set by the previous function)
	// TODO (brian) can we get alpha setting into the library?
	config.alpha = config.beta /
		(MOLT_TISSUESPEED * config.t_params[MOLT_PARAM_STEP] * config.time_scale);

	molt_cfg_set_nu(&config);

	return lump_write(MOLTSTR_CONFIG, sizeof(config), &config, NULL);
}

/* setup_vweight : sets up the v weights */
int setup_vweight()
{
	struct molt_cfg_t config;
	f64 *vlx, *vrx;
	f64 *vly, *vry;
	f64 *vlz, *vrz;
	ivec3_t start, stop;
	ivec3_t dim;
	s64 i;
	int rc;

	rc = lump_read(MOLTSTR_CONFIG, 0, &config);
	if (rc < 0) {
		return -1;
	}

	molt_cfg_parampull_xyz(&config, start, MOLT_PARAM_START);
	molt_cfg_parampull_xyz(&config, stop, MOLT_PARAM_STOP);
	molt_cfg_parampull_xyz(&config, dim, MOLT_PARAM_PINC);

	vlx = calloc(sizeof(f64), dim[0]);
	vrx = calloc(sizeof(f64), dim[0]);
	vly = calloc(sizeof(f64), dim[1]);
	vry = calloc(sizeof(f64), dim[1]);
	vlz = calloc(sizeof(f64), dim[2]);
	vrz = calloc(sizeof(f64), dim[2]);

	for (i = 0; i < dim[0]; i++) { // vlx
		vlx[i] = exp((-config.alpha) * config.space_scale * (i - start[0]));
	}

	for (i = 0; i < dim[0]; i++) { // vrx
		vrx[i] = exp((-config.alpha) * config.space_scale * (stop[0] - i));
	}

	for (i = 0; i < dim[1]; i++) { // vly
		vly[i] = exp((-config.alpha) * config.space_scale * (i - start[1]));
	}

	for (i = 0; i < dim[1]; i++) { // vry
		vry[i] = exp((-config.alpha) * config.space_scale * (stop[1] - i));
	}

	for (i = 0; i < dim[2]; i++) { // vlz
		vrz[i] = exp((-config.alpha) * config.space_scale * (i - start[2]));
	}

	for (i = 0; i < dim[2]; i++) { // vrz
		vlz[i] = exp((-config.alpha) * config.space_scale * (stop[2] - i));
	}

	lump_write(MOLTSTR_VLX, sizeof(f64) * dim[0], vlx, NULL);
	lump_write(MOLTSTR_VRX, sizeof(f64) * dim[0], vrx, NULL);
	lump_write(MOLTSTR_VLY, sizeof(f64) * dim[1], vly, NULL);
	lump_write(MOLTSTR_VRY, sizeof(f64) * dim[1], vry, NULL);
	lump_write(MOLTSTR_VLZ, sizeof(f64) * dim[2], vlz, NULL);
	lump_write(MOLTSTR_VRZ, sizeof(f64) * dim[2], vrz, NULL);

	free(vlx);
	free(vrx);
	free(vly);
	free(vry);
	free(vlz);
	free(vrz);

	return 0;
}

/* setup_wweight : sets up the w weights */
int setup_wweight()
{
	s64 wlx_items, wrx_items;
	s64 wly_items, wry_items;
	s64 wlz_items, wrz_items;
	f64 *wlx, *wrx;
	f64 *wly, *wry;
	f64 *wlz, *wrz;
	struct molt_cfg_t config;
	int rc;

	rc = lump_read(MOLTSTR_CONFIG, 0, &config);
	if (rc < 0) {
		return -1;
	}

	wlx_items = config.x_params[MOLT_PARAM_POINTS] * (config.spaceacc + 1);
	wrx_items = config.x_params[MOLT_PARAM_POINTS] * (config.spaceacc + 1);
	wly_items = config.y_params[MOLT_PARAM_POINTS] * (config.spaceacc + 1);
	wry_items = config.y_params[MOLT_PARAM_POINTS] * (config.spaceacc + 1);
	wlz_items = config.z_params[MOLT_PARAM_POINTS] * (config.spaceacc + 1);
	wrz_items = config.z_params[MOLT_PARAM_POINTS] * (config.spaceacc + 1);

	// allocate memory for the results
	wlx = calloc(sizeof(f64), wlx_items);
	wrx = calloc(sizeof(f64), wrx_items);
	wly = calloc(sizeof(f64), wly_items);
	wry = calloc(sizeof(f64), wry_items);
	wlz = calloc(sizeof(f64), wlz_items);
	wrz = calloc(sizeof(f64), wrz_items);

	get_exp_weights(config.nu[0], wlx, wrx, config.x_params[MOLT_PARAM_POINTS], config.spaceacc);
	get_exp_weights(config.nu[1], wly, wry, config.y_params[MOLT_PARAM_POINTS], config.spaceacc);
	get_exp_weights(config.nu[2], wlz, wrz, config.z_params[MOLT_PARAM_POINTS], config.spaceacc);

	// TODO (brian)
	// think of an elegant way to error and return here if needed
	lump_write(MOLTSTR_WLX, sizeof(f64) * wlx_items, wlx, NULL);
	lump_write(MOLTSTR_WRX, sizeof(f64) * wrx_items, wrx, NULL);
	lump_write(MOLTSTR_WLY, sizeof(f64) * wly_items, wly, NULL);
	lump_write(MOLTSTR_WRY, sizeof(f64) * wry_items, wry, NULL);
	lump_write(MOLTSTR_WLZ, sizeof(f64) * wlz_items, wlz, NULL);
	lump_write(MOLTSTR_WRZ, sizeof(f64) * wrz_items, wrz, NULL);

	free(wlx);
	free(wrx);
	free(wly);
	free(wry);
	free(wlz);
	free(wrz);

	return 0;
}

/* setup_initvelocity : inits velocity, uses custom command if available */
int setup_initvelocity(struct user_cfg_t *usercfg)
{
	// NOTE
	// this function allocates storage and commite it into the lump system
	struct molt_cfg_t config;
	u64 elements;
	f64 *initvelocity;
	ivec3_t dim;
	dvec3_t fdim;
	FILE *pipe_read, *pipe_write;
	struct configthreadargs_t args_read, args_write;
	struct sys_thread thread_reader, thread_writer;
	int rc;

	rc = lump_read(MOLTSTR_CONFIG, 0, &config);
	if (rc < 0) {
		return -1;
	}

	molt_cfg_parampull_xyz(&config, dim, MOLT_PARAM_PINC);

	Vec3Scale(fdim, dim, usercfg->scale_space);

	elements = dim[0] * (u64)dim[1] * dim[2];

	initvelocity = calloc(sizeof(f64), elements);

	if (usercfg && usercfg->isset) {
		rc = sys_bipopen(&pipe_read, &pipe_write, usercfg->initvel);
		if (rc < 0) {
			return -1;
		}

		args_read.fp       = pipe_read;
		args_write.fp      = pipe_write;
		args_read.usercfg  = usercfg;
		args_write.usercfg = usercfg;
		args_read.vol      = initvelocity;
		args_write.vol     = initvelocity;
		Vec3Copy(args_read.dim, dim);
		Vec3Copy(args_write.dim, dim);
		Vec3Copy(args_read.fdim, fdim);
		Vec3Copy(args_write.fdim, fdim);

		thread_reader.func = setup_customprog_read;
		thread_writer.func = setup_customprog_write;
		thread_reader.arg = &args_read;
		thread_writer.arg = &args_write;

		sys_threadcreate(&thread_writer);
		sys_threadcreate(&thread_reader);

		sys_threadwait(&thread_writer);
		sys_threadwait(&thread_reader);
	}

	rc = lump_write(MOLTSTR_VEL, sizeof(f64) * elements, initvelocity, NULL);

	free(initvelocity);

	return rc;
}

/* setup_customprog_write : function for threading setup */
void *setup_customprog_write(void *arg)
{
	struct configthreadargs_t *cargs;
	f64 scale;
	dvec3_t fout;
	ivec3_t curr;
	u64 lim;

	cargs = arg;

	scale = cargs->usercfg->scale_space;

	Vec3Copy(fout, cargs->fdim);

	fprintf(cargs->fp, "%lf\t%lf\t%lf\n", fout[0], fout[1], fout[2]);
	fflush(cargs->fp);

	lim = 0;

	for (curr[0] = 0; curr[0] < cargs->dim[0]; curr[0]++) {
		for (curr[1] = 0; curr[1] < cargs->dim[1]; curr[1]++) {
			for (curr[2] = 0; curr[2] < cargs->dim[2]; curr[2]++) {
				Vec3Copy(fout, curr);
				Vec3Scale(fout, curr, scale);
				fprintf(cargs->fp, "%lf\t%lf\t%lf\n", fout[0], fout[1], fout[2]);
				fflush(cargs->fp);
				lim++;
			}
		}
	}

	printf("we read %ld lines\n", lim);

	fclose(cargs->fp);

	return NULL;
}

/* setup_customprog_read : function for setup reading (parent <- child) */
void *setup_customprog_read(void *arg)
{
	struct configthreadargs_t *cargs;
	u64 pos;
	u64 lim;
	ivec3_t curr;
	char buf[BUFLARGE];

	cargs = arg;

	lim = 0;

	for (curr[0] = 0; curr[0] < cargs->dim[0]; curr[0]++) {
		for (curr[1] = 0; curr[1] < cargs->dim[1]; curr[1]++) {
			for (curr[2] = 0; curr[2] < cargs->dim[2]; curr[2]++) {
				fgets(buf, sizeof(buf), cargs->fp);
				pos = IDX3D(curr[0], curr[1], curr[2], cargs->dim[1], cargs->dim[2]);
				cargs->vol[pos] = atof(buf);
				lim++;
			}
		}
	}

	printf("we read %ld lines\n", lim);

	fclose(cargs->fp);

	return NULL;
}

/* setup_initamplitude : inits amplitude, uses custom command if available */
int setup_initamplitude(struct user_cfg_t *usercfg)
{
	// NOTE
	// this function allocates storage and commite it into the lump system
	struct molt_cfg_t config;
	u64 elements;
	f64 *initamplitude;
	ivec3_t dim;
	int rc;

	rc = lump_read(MOLTSTR_CONFIG, 0, &config);
	if (rc < 0) {
		return -1;
	}

	molt_cfg_parampull_xyz(&config, dim, MOLT_PARAM_PINC);

	elements = dim[0] * (u64)dim[1] * dim[2];

	initamplitude = calloc(sizeof(f64), elements);

	if (usercfg && usercfg->isset) {
		// TODO use the initial amplitude config
		// rc = setup_initvelocity_config(initamplitude, usercfg);
	}

	rc = lump_write(MOLTSTR_AMP, sizeof(f64) * elements, initamplitude, NULL);

	free(initamplitude);

	return rc;
}

/* setup_initamplitude_config : sets up the initial amplitude from the config */
int setup_initamplitude_config(f64 *initamplitude, struct user_cfg_t *usercfg)
{
	return 0;
}

/* dump_lumps : prints a dump of all the lumps in the lump system */
int dump_lumps()
{
	struct lumpheader_t lheader;
	struct lumpinfo_t linfo;
	struct molt_cfg_t config;
	f64 *fptr;
	u64 i;
	ivec3_t dim;
	ivec2_t weight_dim;
	int rc;
	char buf[BUFSMALL];

	// NOTE
	// we follow a super easy pattern
	// read data from the lump system, and print it to stdout
	
	rc = lump_getheader(&lheader);
	if (rc < 0) {
		return -1;
	}

	printf("header:\n");
	printf("\tmagic      : %s\n",   (char *)&lheader.magic);
	printf("\tflags      : 0x%X\n", lheader.flags);
	printf("\tts_created : %ld\n",  lheader.ts_created);
	printf("\tsize       : %ld\n",  lheader.size);
	printf("\tlumps      : %ld\n",  lheader.lumps);

	rc = lump_read(MOLTSTR_CONFIG, 0, &config);
	if (rc < 0) {
		return -1;
	}

	molt_cfg_parampull_xyz(&config, dim, MOLT_PARAM_PINC);
	fptr = calloc(sizeof(f64), dim[0] * (u64)dim[1] * dim[2]);

	// iterate through the lump table to dump the table metadata
	for (i = 0; i < lheader.lumps; i++) {
		rc = lump_getinfo(&linfo, i);

		printf("Lump [%.*s][%4ld] off: 0x%010lX, entry: %4ld, bytes : %ld\n",
				(int)sizeof(linfo.tag), linfo.tag, i, linfo.offset, linfo.entry, linfo.size);
	}

	// iterate through the lumps to dump the data
	for (i = 0; i < lheader.lumps; i++) {
		rc = lump_getinfo(&linfo, i);

		if (strncmp(linfo.tag, MOLTSTR_CONFIG, sizeof(linfo.tag)) == 0) {
		} else if (strncmp(linfo.tag, MOLTSTR_VLX, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_VLX, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			LOG1D(fptr, config.x_params[MOLT_PARAM_PINC], MOLTSTR_VLX);

		} else if (strncmp(linfo.tag, MOLTSTR_VRX, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_VRX, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			LOG1D(fptr, config.x_params[MOLT_PARAM_PINC], MOLTSTR_VRX);

		} else if (strncmp(linfo.tag, MOLTSTR_VLY, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_VLY, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			LOG1D(fptr, config.x_params[MOLT_PARAM_PINC], MOLTSTR_VLY);

		} else if (strncmp(linfo.tag, MOLTSTR_VRY, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_VRY, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			LOG1D(fptr, config.x_params[MOLT_PARAM_PINC], MOLTSTR_VRY);

		} else if (strncmp(linfo.tag, MOLTSTR_VLZ, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_VLZ, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			LOG1D(fptr, config.x_params[MOLT_PARAM_PINC], MOLTSTR_VLZ);

		} else if (strncmp(linfo.tag, MOLTSTR_VRZ, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_VRZ, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			LOG1D(fptr, config.x_params[MOLT_PARAM_PINC], MOLTSTR_VRZ);

		} else if (strncmp(linfo.tag, MOLTSTR_WLX, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_WLX, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			Vec2Set(weight_dim, config.x_params[MOLT_PARAM_POINTS], config.spaceacc + 1);
			LOG2D(fptr, weight_dim, MOLTSTR_WLX);

		} else if (strncmp(linfo.tag, MOLTSTR_WRX, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_WRX, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			Vec2Set(weight_dim, config.x_params[MOLT_PARAM_POINTS], config.spaceacc + 1);
			LOG2D(fptr, weight_dim, MOLTSTR_WRX);

		} else if (strncmp(linfo.tag, MOLTSTR_WLY, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_WLY, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			Vec2Set(weight_dim, config.y_params[MOLT_PARAM_POINTS], config.spaceacc + 1);
			LOG2D(fptr, weight_dim, MOLTSTR_WLY);

		} else if (strncmp(linfo.tag, MOLTSTR_WRY, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_WRY, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			Vec2Set(weight_dim, config.y_params[MOLT_PARAM_POINTS], config.spaceacc + 1);
			LOG2D(fptr, weight_dim, MOLTSTR_WRY);

		} else if (strncmp(linfo.tag, MOLTSTR_WLZ, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_WLZ, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			Vec2Set(weight_dim, config.z_params[MOLT_PARAM_POINTS], config.spaceacc + 1);
			LOG2D(fptr, weight_dim, MOLTSTR_WLZ);

		} else if (strncmp(linfo.tag, MOLTSTR_WRZ, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_WRZ, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			Vec2Set(weight_dim, config.z_params[MOLT_PARAM_POINTS], config.spaceacc + 1);
			LOG2D(fptr, weight_dim, MOLTSTR_WRZ);

		} else if (strncmp(linfo.tag, MOLTSTR_VEL, sizeof(linfo.tag)) == 0) {
			rc = lump_read(MOLTSTR_VEL, 0, fptr);
			if (rc < 0) {
				return -1;
			}
			LOG3D(fptr, dim, MOLTSTR_VEL);

		} else if (strncmp(linfo.tag, MOLTSTR_AMP, sizeof(linfo.tag)) == 0) {
			if (rc < 0) { return -1; }
			snprintf(buf, sizeof buf, "AMP[%ld]", linfo.entry);
			LOG3D(fptr, dim, buf);

		} else {
			printf("Lump [%.*s] doesn't have any logging logic!\n", (int)sizeof(linfo.tag), linfo.tag);
		}
	}

	free(fptr);

	return rc;
}

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
int parse_config(struct user_cfg_t *usercfg, char *file)
{
	FILE *fp;
	char *key, *val, *tmp;
	char buf[BUFLARGE];

	memset(usercfg, 0, sizeof(*usercfg));

	fp = fopen(file, "r");

	if (!fp) {
		return -1;
	}

	while (fgets(buf, sizeof buf, fp) == buf) {
		buf[strlen(buf) - 1] = 0;

		if (strlen(buf) == 0 || buf[0] == '#') {
			continue; // skip blank lines and comments
		}

		key = buf;
		val = strchr(key, ':');
		*val = 0;
		val++;

		// make sure our key is NULL terminated
		tmp = strchr(key, ' ');
		if (tmp) {
			*tmp = 0;
		}

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
		} else if (strcmp("scale_time", key) == 0) {
			usercfg->scale_time = atof(val);
		} else if (strcmp("scale_space", key) == 0) {
			usercfg->scale_space = atof(val);
		} else if (strcmp("acc_time", key) == 0) {
			usercfg->acc_time = atol(val);
		} else if (strcmp("acc_space", key) == 0) {
			usercfg->acc_space = atol(val);
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

	fclose(fp);

	usercfg->isset = 1;

	return 0;
}

