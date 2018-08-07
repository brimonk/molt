/*
 * Brian Chrzanowski
 * Fri Jul 20, 2018 13:46
 *
 * Extended from
 *		Mathematical work done by Matthew Causley, Andrew Christlieb, et al
 *		Particle simulation skeletoned by Khari Gray
 *
 * In-Memory Datastructuring
 *
 *		To prevent as many cache-misses as possible, we (ab)use the concepts
 *		of virtual memory, and effectively keep everything as an array.
 *
 *		Particles
 *			stored as a collection of vec3_t's, behind a dynarr structure
 *			to make adding them easier
 *
 *		Verticies
 *			also stored as a collection of vec3_t's behind a dynarr
 *			structure to make adding them easier
 *
 *		Electric and Magnetic Fields
 *			Because the problem assumes uniform electric and magnetic fields,
 *			they're simply stored as individual vec3_ts. If this changes,
 *			there might be a bit of reworking to do.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <ctype.h>

#include "sqlite3.h"
#include "particles.h"
#include "io.h"

#include "calcs.h"
#include "field_updates.h"
#include "vect.h"

int molt_run(sqlite3 *db, struct dynarr_t *parts,
		struct dynarr_t *verticies, vec3_t *e_field, vec3_t *b_field,
		vec3_t *timevals);
int parse_args(int argc, char **argv, struct dynarr_t *parts,
		struct dynarr_t *verts, vec3_t *e_fld, vec3_t *b_fld,
		vec3_t *time_flds);
void random_init(struct dynarr_t *parts, struct dynarr_t *verts,
		vec3_t *e_fld, vec3_t *b_fld, vec3_t *times);
int parse_args_vec3(vec3_t *ptr, char *opt, char *str);
int parse_args_partt(struct particle_t *ptr, char *opt, char *str);
int parse_double(char *str, double *out);

/* helper functions */
void memerranddie(char *file, int line);

#define DATABASE "molt_output.db"
#define DEFAULT_TIME_INITIAL 0
#define DEFAULT_TIME_FINAL 5
#define DEFAULT_TIME_STEP 0.5
#define DEFAULT_EDGE_SIZE 64
#define DEFAULT_FLOATING_HIGH 64.0
#define GET_RAND_DOUBLE() (((double)rand()/(double)(RAND_MAX)) \
		* DEFAULT_FLOATING_HIGH)
#define USAGE "%s [--vert x,y,z --vert ...] [-e x,y,z] [-b x,y,z] "\
	"[-p number] [--part x,y,z,vx,vy,vz --part .. ] filename\n"
#define RUN_LOOP_FORMAT "[%d] p-%d pos %.3lf, %.3lf, %.3lf "\
	"vel %.3lf, %.3lf, %.3lf\n"

#define CURR_FLOATING_TIME(idx, step, init) ((idx + step + init))
#define MEM_ERR() (memerranddie(__FILE__, __LINE__))

int main(int argc, char **argv)
{
	sqlite3 *db;
	struct dynarr_t particles = {0};
	struct dynarr_t verticies = {0};
	vec3_t e_field = {0};
	vec3_t b_field = {0};
	vec3_t timevals = {0};
	int val;

	/* init, and parse command line arguments (and other launch parameters) */
	dynarr_init(&particles, sizeof(struct particle_t));
	dynarr_init(&verticies, sizeof(vec3_t));
	VectorClear(timevals);
	val = parse_args(argc, argv,
			&particles, &verticies, &e_field, &b_field, &timevals);

	if (val) {
		fprintf(stderr, USAGE, argv[0]);
		exit(1);
	}

	/* for the values that are NULL, go initialize them with random data */
	random_init(&particles, &verticies, &e_field, &b_field, &timevals);

	/* set up the database */
	val = sqlite3_open(DATABASE, &db);

	if (val != SQLITE_OK) {
		SQLITE3_ERR(val);
	}

	/* make sure we have tables to dump data to */
	io_exec_sql_tbls(db, io_db_tbls);

	molt_run(db, &particles, &verticies, &e_field, &b_field, &timevals);

	sqlite3_close(db);

	/* clean up vert and particles */
	dynarr_free(&particles);
	dynarr_free(&verticies);

	return 0;
}

int molt_run(sqlite3 *db, struct dynarr_t *parts,
		struct dynarr_t *verticies, vec3_t *e_field, vec3_t *b_field,
		vec3_t *timevals)
{
	struct particle_t *ptr;
	double tm_initial, tm_curr, tm_step, tm_final;
	int time_i, max_iter, i;

	tm_initial = (*timevals)[0]; // convenience and ease of reading
	tm_final = (*timevals)[1];
	tm_step = (*timevals)[2];

	max_iter = (int)(tm_final / tm_step);

	/* iterate over each time step, taking our snapshot */
	for (time_i = 0; time_i < max_iter; time_i++) {
		// ptr = (struct particle_t *)parts->data;
		ptr = dynarr_get(parts, 0);
		for(i = 0; i < parts->curr_size; i++) // foreach particle
		{
			/* store to disk here */
			printf(RUN_LOOP_FORMAT,
					time_i, ptr[i].uid,
					ptr[i].pos[0], ptr[i].pos[1], ptr[i].pos[2],
					ptr[i].vel[0], ptr[i].vel[1], ptr[i].vel[2]);


			field_update(&ptr[i], e_field, b_field);
			part_pos_update(&ptr[i], tm_step);
			part_vel_update(ptr, e_field, b_field, tm_step);
		}

		tm_curr = CURR_FLOATING_TIME(time_i, tm_step, tm_initial);
	}

	return 0;
}

/*
 * command arguments
 * --vert x[,y[,z]]			adds a new vertex with the requested coords
 *
 * -e	 x[,y[,z]]			defines the initial electric field
 * -b	 x[,y[,z]]			defines the initial magnetic field
 *
 * -p	 number			defines the total number of particles
 * --part x[,y[,z]]			defines a particle from the total allowed particles
 *
 * tstart					simulation's time start (default 0)
 * tend						simulation's time end (default 5)
 * tstep					simulation's time step (default .5)
 *
 * filename					the output database you'd like to use
 */

int parse_args(int argc, char **argv, struct dynarr_t *parts,
		struct dynarr_t *verts, vec3_t *e_fld, vec3_t *b_fld,
		vec3_t *time_flds)
{
	static struct option long_options[] = {
		{"vert", required_argument, 0, 0},
		{"part", required_argument, 0, 0},
		{"tstart", required_argument, 0, 0},
		{"tstep", required_argument, 0, 0},
		{"tend", required_argument, 0, 0},
		{0, 0, 0, 0},
	};

	int val, pn, c, option_index;
	double tmptime;
	vec3_t tmp;
	struct particle_t part_tmp;

	pn = val = 0;

	if (argc < 2) {
		return 1;
	}

	while (1) {
		c = getopt_long(argc, argv, "e:b:p:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 0:
			/* long options - if chain because no easy, reliable way to index */
			if (strcmp("vert", long_options[option_index].name) == 0)
			{
				if (parse_args_vec3(&tmp, "--vert", optarg)) {
					/* append tmp to the vertex dynarr, then clear tmp */
					dynarr_append(verts, &tmp, sizeof(tmp));
					VectorClear(tmp);
				}
			} // vert

			if (strcmp("part", long_options[option_index].name) == 0) {
				if (parse_args_partt(&part_tmp, "--part", optarg)) {
					part_tmp.uid = parts->curr_size;
					dynarr_append(parts, &part_tmp, sizeof(part_tmp));

					VectorClear(part_tmp.pos);
					VectorClear(part_tmp.vel);
					part_tmp.uid = 0;
				}
			} // part

			if (strcmp("tstart", long_options[option_index].name) == 0) {
				if (optarg) {
					if (parse_double(optarg, &tmptime)) {
						(*time_flds)[0] = tmptime;
						tmptime = 0.0;
					}
				}
			}

			if (strcmp("tend", long_options[option_index].name) == 0) {
				if (optarg) {
					if (parse_double(optarg, &tmptime)) {
						(*time_flds)[1] = tmptime;
						tmptime = 0.0;
					}
				}
			}

			if (strcmp("tstep", long_options[option_index].name) == 0) {
				if (optarg) {
					if (parse_double(optarg, &tmptime)) {
						(*time_flds)[2] = tmptime;
						tmptime = 0.0;
					}
				}
			}

			break;

		case 'b':
			/* set the b field based on input */
			if (parse_args_vec3(&tmp, "-b", optarg)) {
				VectorCopy(tmp, *b_fld);
				VectorClear(tmp);
			}
			break;

		case 'e':
			/* set the e field based on input */
			if (parse_args_vec3(&tmp, "-e", optarg)) {
				VectorCopy(tmp, (*e_fld));
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
		}
	}

	if (pn) {
		dynarr_setmaxsize(parts, pn);
	}

	return val;
}

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


int parse_args_partt(struct particle_t *ptr, char *opt, char *str)
{
	/* parse a vec3_t from a string; looks like "0.4,0.2,0.5" */
	vec3_t tmp;
	char *split;
	char sep_char = ':';
	int scanned;

	split = strchr(str, sep_char);

	// get the positions
	if ((scanned = parse_args_vec3(&tmp, "", str))) {
		VectorCopy(tmp, ptr->pos);
		VectorClear(tmp);
	}

	if (split) { // if we have a ':' in the str, get the vel
		if ((scanned = parse_args_vec3(&tmp, "", ++split))) {
			VectorCopy(tmp, ptr->vel);
			VectorClear(tmp);
		}
	}

	return scanned;
}

void random_init(struct dynarr_t *parts, struct dynarr_t *verts,
		vec3_t *e_fld, vec3_t *b_fld, vec3_t *times)
{
	/* finish initializing the data the user HASN'T (or can't) put in */
	int i, j, k;
	struct particle_t *part_ptr, part_tmp;
	vec3_t tmp;

	srand((unsigned int)time(NULL));

	// check the e field
	if (!((*e_fld)[0] || (*e_fld)[1] || (*e_fld)[2])) {
		(*e_fld)[0] = GET_RAND_DOUBLE();
		(*e_fld)[1] = GET_RAND_DOUBLE();
		(*e_fld)[2] = GET_RAND_DOUBLE();
	}

	// check the b field
	if (!((*e_fld)[0] || (*e_fld)[1] || (*e_fld)[2])) {
		(*b_fld)[0] = GET_RAND_DOUBLE();
		(*b_fld)[1] = GET_RAND_DOUBLE();
		(*b_fld)[2] = GET_RAND_DOUBLE();
	}

	// fill out our verticies.
	// TODO: figure out if we'll ever need more than 9 verticies or more than
	// 8 cubes for this simulation. For now, this will provide the simulation
	// with 8 cubes, all of DEFAULT_EDGE_SIZE length. Keeping it a power of 2
	// guarantees that it can be properly represented in a double.
	for (i = -1; i < 2; i++) {
		for (j = -1; j < 2; j++) {
			for (k = -1; k < 2; k++) {
				tmp[0] = DEFAULT_EDGE_SIZE * i;// x
				tmp[1] = DEFAULT_EDGE_SIZE * j;// x
				tmp[2] = DEFAULT_EDGE_SIZE * k;// x

				// copy to our verts array
				dynarr_append(verts, &tmp, sizeof(vec3_t));
			}
		}
	}

	// check if we have enough particles. if not, fill out the rest
	// with ensuring it's within the bounds of the vector
	if (parts->max_size != parts->curr_size) {
		part_ptr = parts->data;
		for (i = parts->curr_size; i < parts->max_size; i++) {
			part_tmp.uid = i;
			// position
			part_tmp.pos[0] = GET_RAND_DOUBLE();
			part_tmp.pos[1] = GET_RAND_DOUBLE();
			part_tmp.pos[2] = GET_RAND_DOUBLE();

			// velocity
			part_tmp.vel[0] = GET_RAND_DOUBLE();
			part_tmp.vel[1] = GET_RAND_DOUBLE();
			part_tmp.vel[2] = GET_RAND_DOUBLE();
			dynarr_append(parts, &part_tmp, sizeof(part_tmp));
		}
	}

	// set the default times if everything's zeroes
	if ((*times)[0] == 0 && (*times)[1] == 0 && (*times)[2] == 0) {
		(*times)[0] = DEFAULT_TIME_INITIAL;
		(*times)[1] = DEFAULT_TIME_FINAL;
		(*times)[2] = DEFAULT_TIME_STEP;
	}
}

void memerranddie(char *file, int line)
{
	fprintf(stderr, "Memory Error in %s:%d\n", file, line);
	exit(1);
}

