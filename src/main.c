/*
 * Brian Chrzanowski
 * Fri Jul 20, 2018 13:46
 *
 * Extended from
 *		Mathematical work done by Matthew Causley, Andrew Christlieb, et al
 *		Particle simulation skeletoned by Khari Gray
 *
 * Organization
 *		For maximum cache coherency, I've decided that the entire program not
 *		have a ton of context switches. From a command line argument, the
 *		program retrieves all memory required to keep the simulation's state,
 *		and whatever extra memory a compute device might need, nearly equivalent
 *		to the size of a struct molt_t. The idea being, everything for the
 *		problem is relayed in a molt_t structure.
 *
 * ToDo
 * 1. Rewrite program to be organized in a single structure
 * 1a. Argument Parsing and Initialization from those arguments
 * 1b. IO Routine Rewrite
 * 1c. IO Error Code Checking
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <ctype.h>

#include "sqlite3.h"
#include "io.h"

#include "calcs.h"
#include "field_updates.h"
#include "vect.h"

int molt_init(sqlite3 *db, struct molt_t *molt, int argc, char **argv);
int molt_run(sqlite3 *db, struct molt_t *molt);
void molt_cleanup(sqlite3 *db, struct molt_t *molt);

int parse_args(int argc, char **argv, struct molt_t *molt);
int parse_args_vec3(vec3_t *ptr, char *opt, char *str);
int parse_double(char *str, double *out);
void set_default_args(struct molt_t *molt);

/* helper functions */
void erranddie(char *file, char *errstr, int line);

#define DATABASE "molt_output.db"
#define DEFAULT_TIME_START 0
#define DEFAULT_TIME_STEP  0.5
#define DEFAULT_TIME_STOP  5
#define DEFAULT_EDGE_SIZE  64
#define USAGE "%s [--vert x,y,z --vert ...] [-e x,y,z] [-b x,y,z] [-p num]"\
  "	filename\n"
#define GET_RAND_DOUBLE() ((((double)rand()/(double)(RAND_MAX)) * 512) - 256)

#define CURR_FLOATING_TIME(idx, step, init) ((idx + step + init))
#define ERRANDDIE(a) (erranddie(__FILE__, (a), __LINE__))

int main(int argc, char **argv)
{
	sqlite3 *db;
	struct molt_t molt;
	int val;

	/* set up the database */
	val = sqlite3_open(DATABASE, &db);
	if (val != SQLITE_OK) {
		SQLITE3_ERR(db);
	}

	io_db_setup(db);
	io_exec_sql_tbls(db, io_db_tbls); /* ensure tables exist */

	molt_init(db, &molt, argc, argv);
	molt_run(db, &molt);
	molt_cleanup(db, &molt);

	sqlite3_close(db);

	return 0;
}

int molt_init(sqlite3 *db, struct molt_t *molt, int argc, char **argv)
{
	long i;
	int val;

	/* init, and parse command line arguments (and other launch parameters) */
	val = parse_args(argc, argv, molt);

	if (val) {
		fprintf(stderr, USAGE, argv[0]);
		exit(1);
	}

	/* retrieve the current run information */
	molt->info.run_number = io_get_runid(db, molt);

	/* memory allocation occurs in one hunk, and subdivided */
	molt->x = malloc(sizeof(double) * molt->part_total * 6);
	if (!molt->x) {
		ERRANDDIE("Couldn't get simulation memory");
	}

	memset(molt->x, 0, sizeof(double) * molt->part_total * 6);

	/* set up the rest of the simulation's pointers */
	molt->x  = molt->x + 0;
	molt->y  = molt->x + (molt->part_total * 1);
	molt->z  = molt->x + (molt->part_total * 2);
	molt->vx = molt->x + (molt->part_total * 3);
	molt->vy = molt->x + (molt->part_total * 4);
	molt->vz = molt->x + (molt->part_total * 5);

	/* randomize ALL OF THE PARTICLE INFORMATION */
	for (i = 0; i < molt->part_total; i++) {
		molt->x[i]  = GET_RAND_DOUBLE();
		molt->y[i]  = GET_RAND_DOUBLE();
		molt->z[i]  = GET_RAND_DOUBLE();
		molt->vx[i] = GET_RAND_DOUBLE();
		molt->vy[i] = GET_RAND_DOUBLE();
		molt->vz[i] = GET_RAND_DOUBLE();
	}

	/* make sure we store the static, common run information */
	io_store_inforun(db, molt);

	return 0;
}

int molt_run(sqlite3 *db, struct molt_t *molt)
{
	struct run_info_t *info;
	int max_iter, i;

	info = &molt->info;

	max_iter = (int)((info->time_stop - info->time_start) / info->time_step);

	/* iterate over each time step, taking our snapshot, computing the next  */
	for (info->time_idx = 0; info->time_idx < max_iter; info->time_idx++) {
		if (io_store_infots(db, molt)) {
			SQLITE3_ERR(db);
		}

		for(i = 0; i < molt->part_total; i++) { // foreach particle
			field_update(molt);
			part_pos_update(molt, i, info->time_step);
			part_vel_update(molt, i, info->time_step);
		}
	}

	return 0;
}

/* molt_cleanup : cleanup the molt structure and finalize the db actions */
void molt_cleanup(sqlite3 *db, struct molt_t *molt)
{
}

/*
 * command arguments
 * --vert x[,y[,z]]			adds a new vertex with the requested coords
 *
 * -e	 x[,y[,z]]			defines the initial electric field
 * -b	 x[,y[,z]]			defines the initial magnetic field
 *
 * -p	 number			    defines the total number of particles
 *
 * -h                       prints usage information
 *
 * --tstart					simulation's time start (default 0)
 * --tend					simulation's time end (default 5)
 * --tstep					simulation's time step (default .5)
 *
 * filename					the output database you'd like to use
 */

/* parse_args : parse command line arguments, storing in existing molt struct */
int parse_args(int argc, char **argv, struct molt_t *molt)
{
	static struct option long_options[] = {
		// {"vert", required_argument, 0, 0},
		{"tstart", required_argument, 0, 0},
		{"tstep", required_argument, 0, 0},
		{"tend", required_argument, 0, 0},
		{0, 0, 0, 0},
	};

	int val, pn, c, option_index;
	double tmptime;
	vec3_t tmp;

	/* set defaults before we parse the arguments */
	set_default_args(molt);

	pn = val = 0;

	if (argc < 2) {
		return 1;
	}

	while (1) {
		c = getopt_long(argc, argv, "e:b:p:h", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			/* long options - if chain because no easy, reliable way to index */
#if 0
			if (strcmp("vert", long_options[option_index].name) == 0)
			{
				if (parse_args_vec3(&tmp, "--vert", optarg)) {
					/* append tmp to the vertex dynarr, then clear tmp */
					dynarr_append(verts, &tmp, sizeof(tmp));
					VectorClear(tmp);
				}
			} // vert
#endif

			if (strcmp("tstart", long_options[option_index].name) == 0) {
				if (optarg) {
					if (parse_double(optarg, &tmptime)) {
						molt->info.time_start = tmptime;
						tmptime = 0.0;
					}
				}
			}

			if (strcmp("tend", long_options[option_index].name) == 0) {
				if (optarg) {
					if (parse_double(optarg, &tmptime)) {
						molt->info.time_stop = tmptime;
						tmptime = 0.0;
					}
				}
			}

			if (strcmp("tstep", long_options[option_index].name) == 0) {
				if (optarg) {
					if (parse_double(optarg, &tmptime)) {
						molt->info.time_step = tmptime;
						tmptime = 0.0;
					}
				}
			}

			break;

		case 'b':
			/* set the b field based on input */
			if (parse_args_vec3(&tmp, "-b", optarg)) {
				VectorCopy(tmp, molt->b_field);
				VectorClear(tmp);
			}
			break;

		case 'e':
			/* set the e field based on input */
			if (parse_args_vec3(&tmp, "-e", optarg)) {
				VectorCopy(tmp, molt->e_field);
				VectorClear(tmp);
			}
			break;

		case 'p':
			/* set the number of total particles in the system */
			if (optarg) {
				// error checking
				pn = atoi(optarg);
			}
			break;

		case 'h':
			/* print usage information and quit */
			fprintf(stderr, USAGE, argv[0]);
			exit(1);
		}
	}

	if (pn) {
		molt->part_total = pn;
	}

	return val;
}

/* set up the program's default arguments */
void set_default_args(struct molt_t *molt)
{
	srand(time(NULL));

	if (molt) { /* just in case */
		molt->info.time_start = DEFAULT_TIME_START;
		molt->info.time_step  = DEFAULT_TIME_STEP;
		molt->info.time_stop  = DEFAULT_TIME_STOP;
		molt->info.time_idx   = 0;
		molt->part_total      = 256;

		/* randomize constant fields */
		molt->e_field[0] = GET_RAND_DOUBLE();
		molt->e_field[1] = GET_RAND_DOUBLE();
		molt->e_field[2] = GET_RAND_DOUBLE();
		molt->b_field[0] = GET_RAND_DOUBLE();
		molt->b_field[1] = GET_RAND_DOUBLE();
		molt->b_field[2] = GET_RAND_DOUBLE();
	}
}

int parse_double(char *str, double *out)
{
	int scanned;
	double tmp;

	scanned = sscanf(str, "%lf", &tmp);

	if (scanned) {
		*out = tmp;
	}

	return scanned;
}

void random_init(struct molt_t *molt)
{
}

/* parse_args_vec3 : parse a comma delimted vector string into a vec3_t */
int parse_args_vec3(vec3_t *ptr, char *opt, char *str)
{
	/* parse a vec3_t from a string; looks like "0.4,0.2,0.5" */
	vec3_t tmp = {0};
	int scanned;

	/* use the standard library where possible */
	scanned = sscanf(str, "%lf,%lf,%lf", &tmp[0], &tmp[1], &tmp[2]);

	if (3 < scanned) {
		fprintf(stderr,
				"WARNING, received %d items for '%s',"\
				"where expected 3 or less\n",
				scanned, opt);
	}

	VectorCopy(tmp, *ptr);

	return scanned;
}

void erranddie(char *file, char *errstr, int line)
{
	fprintf(stderr, "%s:%d %s\n", file, line, errstr);
	exit(1);
}

