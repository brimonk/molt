/*
 * Brian Chrzanowski
 * Sun Jan 20, 2019 14:43
 *
 * Method Of Lines Transpose - Implicit Wave Solver with Pluggable Modules
 *
 * TODO (Brian)
 * 1. Simulation Setup With Hunks
 * 2. Function Handles (f, g)
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

void setup_simulation(void *base);
void simsetup_cfg(struct moltcfg_t *cfg);
void applied_funcf(void *base);
void applied_funcg(void *base);

int main(int argc, char **argv)
{
	void *hunk;
	int fd, i;

	fd = io_open(DEFAULTFILE);

	io_resize(fd, 1024 * 16);

	hunk = io_mmap(fd, 1024 * 16);
	memset(hunk, 0, 1024 * 16);

	setup_simulation(hunk);

	io_mssync(hunk, hunk, 1024 * 16);

	for (i = 0; i < MOLTLUMP_TOTAL; i++) {
		printf("lump %d size : %ld\n",
				i,
				((struct lump_header_t *)hunk)->lump[i].elemsize);
	}

	printf("steps\tx : %d y : %d z : %d\n",
			MOLT_XPOINTS, MOLT_YPOINTS, MOLT_ZPOINTS);

	io_munmap(hunk);
	io_close(fd);

	return 0;
}

/* setup_simulation : sets up simulation based on config.h */
void setup_simulation(void *base)
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
	hdr->lump[2].lumpsize = MOLT_TOTALSTEPS * hdr->lump[2].elemsize;

	curr_offset += hdr->lump[2].lumpsize;

	/* setup our pfield information */
	hdr->lump[3].offset = curr_offset;
	hdr->lump[3].elemsize = sizeof(struct lump_pfield_t);
	hdr->lump[3].lumpsize = MOLT_TOTALSTEPS * hdr->lump[3].elemsize;

	curr_offset += hdr->lump[3].lumpsize;

	/* setup our vweight information */
	hdr->lump[4].offset = curr_offset;
	hdr->lump[4].elemsize = sizeof(struct lump_vweight_t);
	hdr->lump[4].lumpsize = MOLT_TOTALSTEPS * hdr->lump[4].elemsize;

	curr_offset += hdr->lump[4].lumpsize;

	/* setup our wweight information */
	hdr->lump[5].offset = curr_offset;
	hdr->lump[5].elemsize = sizeof(struct lump_wweight_t);
	hdr->lump[5].lumpsize = MOLT_TOTALSTEPS * hdr->lump[5].elemsize;

	curr_offset += hdr->lump[5].lumpsize;

	/* setup our problem state */
	hdr->lump[6].offset = curr_offset;
	hdr->lump[6].elemsize = sizeof(struct lump_pstate_t);
	hdr->lump[6].lumpsize = MOLT_TOTALSTEPS * hdr->lump[6].elemsize;

	curr_offset += hdr->lump[6].lumpsize;
}

void simsetup_cfg(struct moltcfg_t *cfg)
{
}

/* misc math functions */

/* applied_func : applies function "f" from the original source to data */
void applied_funcf(void *base)
{
}

/* applied_func : applies function "g" from the original source to data */
void applied_funcg(void *base)
{
}

