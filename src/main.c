/*
 * Brian Chrzanowski
 * Sun Jan 20, 2019 14:43
 *
 * Method Of Lines Transpose - Implicit Wave Solver with Pluggable Modules
 *
 * TODO (Brian)
 * 2. Setup Simulation Data
 * 2c. wweight
 * 2d. program state
 * 2e. nu-lumps
 * 3. time looping
 * 3a. first_timestep
 * 3b. d and c operators
 * 3c. 3d array reorg
 * 3d. do_sweep
 * 3e. gf_quad
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "io.h"
#include "calcs.h"
#include "config.h"
#include "gfquad.h"

#define DEFAULTFILE "data.dat"
#define DBL_MACRO_SIZE(x) sizeof(x) / sizeof(double)

/* lump setup functions */
void setup_simulation(void **base, u64 *size, int fd);
u64 setup_lumps(void *base);
void setuplump_cfg(struct moltcfg_t *cfg);
void setuplump_run(struct lump_runinfo_t *run);
void setuplump_efield(struct lump_header_t *hdr, struct lump_efield_t *efield);
void setuplump_pfield(struct lump_header_t *hdr, struct lump_pfield_t *pfield);
void setuplump_vweight(struct lump_header_t *hdr, struct lump_vweight_t *vw);
void setuplump_wweight(struct lump_header_t *hdr, struct lump_wweight_t *ww);
void setuplump_pstate(struct lump_header_t *hdr, struct lump_pstate_t *state);

void do_simulation(void *hunk, u64 hunksize);

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
	memset(hunk, 0, hunksize);

	setup_simulation(&hunk, &hunksize, fd);

	debug(hunk);

	do_simulation(hunk, hunksize);

	/* sync the file, then clean up */
	io_mssync(hunk, hunk, hunksize);
	io_munmap(hunk);
	io_close(fd);

	return 0;
}

/* do_simulation : actually does the simulating */
void do_simulation(void *hunk, u64 hunksize)
{
}

/* setup_simulation : sets up simulation based on config.h */
void setup_simulation(void **base, u64 *size, int fd)
{
	void *newblk;
	u64 oldsize;

	oldsize = *size;
	*size = setup_lumps(*base);

	/* resize the file to get enough simulation space */
	if (io_resize(fd, *size) < 0) {
		fprintf(stderr, "Couldn't get space. Quitting\n");
		exit(1);
	}

	/* remmap it */
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
	setuplump_vweight(*base, io_lumpgetbase(*base, MOLTLUMP_VWEIGHT));
	setuplump_wweight(*base, io_lumpgetbase(*base, MOLTLUMP_WWEIGHT));
	setuplump_pstate(*base, io_lumpgetbase(*base, MOLTLUMP_PSTATE));
}

/* setup_lumps : returns size of file after lumpsystem setup */
u64 setup_lumps(void *base)
{
	struct lump_header_t *hdr;
	u64 curr_offset;

	// ensure we have a valid memory mapped file
	if (io_lumpcheck(base)) {
		io_fprintf(stdout, "Setting Up New File %s\n", io_getfilename());
	}

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
	hdr->lump[2].lumpsize = ((u64)MOLT_TOTALSTEPS) * hdr->lump[2].elemsize;

	curr_offset += hdr->lump[2].lumpsize;

	/* setup our pfield information */
	hdr->lump[3].offset = curr_offset;
	hdr->lump[3].elemsize = sizeof(struct lump_pfield_t);
	hdr->lump[3].lumpsize = ((u64)MOLT_TOTALSTEPS) * hdr->lump[3].elemsize;

	curr_offset += hdr->lump[3].lumpsize;

	/* setup our vweight information */
	hdr->lump[4].offset = curr_offset;
	hdr->lump[4].elemsize = sizeof(struct lump_vweight_t);
	hdr->lump[4].lumpsize = ((u64)MOLT_TOTALSTEPS) * hdr->lump[4].elemsize;

	curr_offset += hdr->lump[4].lumpsize;

	/* setup our wweight information */
	hdr->lump[5].offset = curr_offset;
	hdr->lump[5].elemsize = sizeof(struct lump_wweight_t);
	hdr->lump[5].lumpsize = ((u64)MOLT_TOTALSTEPS) * hdr->lump[5].elemsize;

	curr_offset += hdr->lump[5].lumpsize;

	/* setup our problem state */
	hdr->lump[6].offset = curr_offset;
	hdr->lump[6].elemsize = sizeof(struct lump_pstate_t);
	hdr->lump[6].lumpsize = ((u64)MOLT_TOTALSTEPS) * hdr->lump[6].elemsize;

	curr_offset += hdr->lump[6].lumpsize;

	return curr_offset;
}

/* simsetup_cfg : setup the config lump */
void setuplump_cfg(struct moltcfg_t *cfg)
{
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
	cfg->step_in_sec = MOLT_STEPINSEC;
	cfg->step_in_x = MOLT_STEPINX;
	cfg->step_in_y = MOLT_STEPINY;
	cfg->step_in_z = MOLT_STEPINZ;
	cfg->cfl = MOLT_CFL;
	/* dimension parms */
	cfg->sim_time = MOLT_SIMTIME;
	cfg->sim_x = MOLT_DOMAINWIDTH;
	cfg->sim_y = MOLT_DOMAINDEPTH;
	cfg->sim_z = MOLT_DOMAINHEIGHT;
	cfg->sim_time_steps = MOLT_TOTALSTEPS;
	cfg->sim_x_steps = MOLT_TOTALWIDTH;
	cfg->sim_y_steps = MOLT_TOTALDEEP;
	cfg->sim_z_steps = MOLT_TOTALHEIGHT;
	/* MOLT parameters */
	cfg->space_accuracy = MOLT_SPACEACC;
	cfg->time_accuracy = MOLT_TIMEACC;
	cfg->beta = MOLT_BETA;
	cfg->alpha = MOLT_ALPHA;
}

/* simsetup_run : setup run information */
void setuplump_run(struct lump_runinfo_t *run)
{
	run->time_start = 0;
	run->time_step = .5;
	run->time_stop = 10;
	run->curr_idx = 0;
	run->total_steps = (run->time_stop - run->time_start) / run->time_step;
}

/* setuplump_efield : setup the efield lump */
void setuplump_efield(struct lump_header_t *hdr, struct lump_efield_t *efield)
{
}

/* setuplump_pfield : setup the pfield lump */
void setuplump_pfield(struct lump_header_t *hdr, struct lump_pfield_t *pfield)
{
}

/* setuplump_vweight : setup the vweight lump */
void setuplump_vweight(struct lump_header_t *hdr, struct lump_vweight_t *vw)
{
}

/* setuplump_wweight : setup the wweight lump */
void setuplump_wweight(struct lump_header_t *hdr, struct lump_wweight_t *ww)
{
}

/* setuplump_pstate : setup the "problem state" lump */
void setuplump_pstate(struct lump_header_t *hdr, struct lump_pstate_t *state)
{
}

#if 0
/* misc math functions */

/* applied_func : applies function "f" from the original source to data */
void applied_funcf(void *base)
{
}

/* applied_func : applies function "g" from the original source to data */
void applied_funcg(void *base)
{
}
#endif

void debug(void *hunk)
{
	struct lump_header_t *hdr;
	int i;

	hdr = hunk;

	io_fprintf(stdout, "header : 0x%8x\tver : %d\t type : 0x%02x\n",
			hdr->magic, hdr->version, hdr->type);

	for (i = 0; i < MOLTLUMP_TOTAL; i++) {
		io_fprintf(stdout,
				"lump[%d] off : 0x%08lx\tlumpsz : 0x%08lx\telemsz : 0x%08lx\n",
				i, hdr->lump[i].offset,
				hdr->lump[i].lumpsize, hdr->lump[i].elemsize);
	}
}

