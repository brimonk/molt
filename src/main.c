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
		struct dynarr_t *verticies, vec3_t *e_field, vec3_t *b_field);
int parse_args(int argc, char **argv, struct dynarr_t *parts,
		struct dynarr_t *verts, vec3_t *e_fld, vec3_t *b_fld);
int parse_args_vec3(vec3_t *ptr, char *opt, char *str);
void random_init(struct dynarr_t *part_ptr, struct dynarr_t *verts,
		vec3_t *e_fld, vec3_t *b_fld);

/* helper functions */
void memerranddie(char *file, int line);
int isnumericstr(char *str);

#define DATABASE "molt_output.db"
#define DEFAULT_PART_SIZE 64
#define USAGE "%s [--vert x,y,z --vert ...] [-e x,y,z] [-b x,y,z] "\
	"[-p number] [--part x,y,z --part .. ] filename\n"

#define CURR_FLOATING_TIME(idx, step, init) ((idx + step + init))
#define MEM_ERR() (memerranddie(__FILE__, __LINE__))

int main(int argc, char **argv)
{
	sqlite3 *db;
	struct dynarr_t particles, verticies;
	vec3_t e_field, b_field;
    int val;

	/* parse command line arguments (and other launch parameters) */
	dynarr_init(&particles, sizeof(vec3_t));
	dynarr_init(&verticies, sizeof(vec3_t));
	val = parse_args(argc, argv, &particles, &verticies, &e_field, &b_field);

	if (val) {
		fprintf(stderr, USAGE, argv[0]);
		exit(1);
	}

	/* for the values that are NULL, go initialize them with random data */
	random_init(&particles, &verticies, &e_field, &b_field);

	/* set up the database */
	val = sqlite3_open(DATABASE, &db);

	if (val != SQLITE_OK) {
		SQLITE3_ERR(val);
	}

	/* make sure we have tables to dump data to */
	io_exec_sql_tbls(db, io_db_tbls);

	molt_run(db, &particles, &verticies, &e_field, &b_field);

	sqlite3_close(db);

	/* clean up vert and particles */
	dynarr_free(&particles);
	dynarr_free(&verticies);

	return 0;
}

int molt_run(sqlite3 *db, struct dynarr_t *parts,
		struct dynarr_t *verticies, vec3_t *e_field, vec3_t *b_field)
{
	struct particle_t *ptr;
	double tm_initial = 0, tm_curr, tm_step = 0.25, tm_final = 30;
    int time_i, max_iter, i;

	max_iter = (int)(tm_final / tm_step);

	/* iterate over each time step, taking our snapshot */
	for (time_i = 0; time_i < max_iter; time_i++) {
        for(i = 0; i < parts->curr_size; i++) // foreach particle
        {
			ptr = DyNARR_GetPtr(parts, i); // ((char *)(parts->data)) + (parts->obj_size * i);
            field_update(ptr, e_field, b_field);
			part_pos_update(ptr, tm_step);

			/* store to disk here??? */
			printf("[%d] p-%d (%.3lf, %.3lf, %.3lf)\n",
					time_i, i, ptr[i].pos[0], ptr[i].pos[1], ptr[i].pos[2]);


			part_vel_update(ptr, e_field, b_field, tm_step);

            // updatePosition(i, timeInd); // updating position values
            // updateVelocity(i, timeInd); // updating velocity values
        }

		tm_curr = CURR_FLOATING_TIME(time_i, tm_step, tm_initial);
    }

	return 0;
}

/*
 * command arguments
 * --vert x[,y[,z]]			adds a new vertex with the requested coords
 *
 * -e     x[,y[,z]]			defines the initial electric field
 * -b     x[,y[,z]]			defines the initial magnetic field
 *
 * -p     number			defines the total number of particles
 * --part x[,y[,z]]			defines a particle from the total allowed particles
 *
 * filename					the output database you'd like to use
 */

int parse_args(int argc, char **argv, struct dynarr_t *parts,
		struct dynarr_t *verts, vec3_t *e_fld, vec3_t *b_fld)
{
	static struct option long_options[] = {
		{"vert", required_argument, 0, 0},
		{"part", required_argument, 0, 0},
		{0, 0, 0, 0},
	};

	int val, pn, c, option_index;
	vec3_t tmp;

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
				if (parse_args_vec3(&tmp, "--part", optarg)) {
					dynarr_append(parts, &tmp, sizeof(tmp));
					VectorClear(tmp);
				}
			} // part

			break;

		case 'b':
			/* set the b field based on input */
			if (parse_args_vec3(&tmp, "-b", optarg)) {
				VectorCopy(tmp, (*b_fld));
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
				pn = atoi(optarg);
			}
			break;
		}
	}

	return val;
}

int parse_args_vec3(vec3_t *ptr, char *opt, char *str)
{
	/* parse a vec3_t from a string looking like "0.4,0.2,0.5" */
	double x, y, z;
	int scanned;

	x = y = z = 0;

	/* use the standard library where possible */
	scanned = sscanf(str, "%lf,%lf,%lf", &x, &y, &z);

	if (3 < scanned) {
		fprintf(stderr,
				"WARNING, received %d items for '%s',"\
				"where expected 3 or less\n",
				scanned, opt);
	}

	(*ptr)[0] = x;
	(*ptr)[1] = y;
	(*ptr)[2] = z;

	return scanned;
}


void random_init(struct dynarr_t *part_ptr, struct dynarr_t *verts,
		vec3_t *e_fld, vec3_t *b_fld)
{
	/* finish initializing the data the user HASN'T put in */
}

void memerranddie(char *file, int line)
{
	fprintf(stderr, "Memory Error in %s:%d\n", file, line);
	exit(1);
}

int isnumericstr(char *str)
{
	int i;
	for (i = 0; i < strlen(str); i++) {
		if (!isdigit(str[i])) {
			break;
		}
	}

	return i == strlen(str);
}
