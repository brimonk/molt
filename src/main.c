/*
 * Brian Chrzanowski
 * Fri Jul 20, 2018 13:46
 *
 * Extended from
 *		Mathematical work done by Matthew Causley, Andrew Christlieb, et al
 *		Particle simulation skeletoned by Khari Gray
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <getopt.h>

#include "sqlite3.h"
#include "particles.h"
#include "io.h"

#include "vect.h"
#include "calcs.h"
#include "field_updates.h"

int molt_run(sqlite3 *db, struct particle_t *parts, int part_size,
		vec3_t *verticies, vec3_t *e_field, vec3_t *b_field);
int parse_args(int argc, char **argv, struct particle_t **part_ptr, int *pn,
		vec3_t **verts, vec3_t **e_fld, vec3_t **b_fld);
int parse_args_vec3(vec3_t *ptr, char *opt, char *str);
void random_init(struct particle_t **part_ptr, int pn, vec3_t **verts,
		vec3_t **e_fld, vec3_t **b_fld);
void memerranddie(char *file, int line);

#define DATABASE "molt_output.db"
#define USAGE "%s [--vert x,y,z --vert ...] [-e x,y,z] [-b x,y,z] "\
	"[-p number] [--part x,y,z --part .. ] filename\n"
#define CURR_FLOATING_TIME(idx, step, init) ((idx + step + init))
#define MEM_ERR() (memerranddie(__FILE__, __LINE__))

int main(int argc, char **argv)
{
	sqlite3 *db;
	struct particle_t *particles = NULL;
	vec3_t *verticies = NULL, *e_field = NULL, *b_field = NULL;
    int val, part_num;
	part_num = 0;

	/* parse command line arguments (and other launch parameters) */
	val = parse_args(argc, argv, &particles, &part_num,
			&verticies, &e_field, &b_field);

	if (val) {
		fprintf(stderr, USAGE, argv[0]);
		exit(1);
	}

	/* for the values that are NULL, go initialize them with random data */
	random_init(&particles, part_num, &verticies, &e_field, &b_field);

	/* set up the database */
	val = sqlite3_open(DATABASE, &db);

	if (val != SQLITE_OK) {
		SQLITE3_ERR(val);
	}

	/* make sure we have tables to dump data to */
	io_exec_sql_tbls(db, io_db_tbls);

	molt_run(db, particles, part_num, verticies, e_field, b_field);

	sqlite3_close(db);

	/* free all of the memory we've allocated in the init procedure */
	if (e_field)
		free(e_field);

	if (b_field)
		free(b_field);

	if (verticies)
		free(verticies);

	if (particles)
		free(particles);

	return 0;
}

int molt_run(sqlite3 *db, struct particle_t *parts, int part_size,
		vec3_t *verticies, vec3_t *e_field, vec3_t *b_field)
{
	double tm_initial = 0, tm_curr, tm_step = 0.25, tm_final = 30;
    int time_i, max_iter, i, total_parts = 0; // timeInd indicates the iteration in time that the simulation is at

	VectorClear((*e_field));
	VectorClear((*b_field));

	/* do some initialization right here */

	max_iter = (int)(tm_final / tm_step);

	/* iterate over each time step, taking our snapshot */
	for (time_i = 0; time_i < max_iter; time_i++) {
        for(i = 0; i < total_parts; i++) // repeating funtions per number of particles
        {
            field_update(&parts[i], e_field, b_field);
			part_pos_update(&parts[i], tm_step);

			/* store to disk here??? */
			printf("[%d] p-%d (%.3lf, %.3lf, %.3lf)\n",
					time_i, i,
					parts[i].pos[0],
					parts[i].pos[1],
					parts[i].pos[2]);


			part_vel_update(&parts[i], e_field, b_field, tm_step);

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

int
parse_args(int argc, char **argv, struct particle_t **part_ptr, int *pn,
		vec3_t **verts, vec3_t **e_fld, vec3_t **b_fld)
{
	static struct option long_options[] = {
		{"vert", required_argument, 0, 0},
		{"part", required_argument, 0, 0},
		{0, 0, 0, 0},
	};

	int val, c, option_index;
	vec3_t tmp;

	val = 0;

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
				}
			} // vert

			if (strcmp("part", long_options[option_index].name) == 0) {
			} // part

			break;

		case 'b':
			/* set the b field based on input */
			if (parse_args_vec3(&tmp, "-b", optarg)) {
				*b_fld = malloc(sizeof(vec3_t));
				if (*b_fld) {
					(**b_fld)[0] = tmp[0];
					(**b_fld)[1] = tmp[1];
					(**b_fld)[2] = tmp[2];
					VectorClear(tmp);
				} else {
					MEM_ERR();
				}
			}
			break;

		case 'e':
			/* set the e field based on input */
			if (parse_args_vec3(&tmp, "-e", optarg)) {
				*e_fld = malloc(sizeof(vec3_t));
				if (*e_fld) {
					(**e_fld)[0] = tmp[0];
					(**e_fld)[1] = tmp[1];
					(**e_fld)[2] = tmp[2];
					VectorClear(tmp);
				} else {
					MEM_ERR();
				}
			}
			break;

		case 'p':
			/* set the number of total particles in the system */
			if (optarg) {
				*pn = atoi(optarg);
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


void
random_init(struct particle_t **part_ptr, int pn, vec3_t **verts,
		vec3_t **e_fld, vec3_t **b_fld)
{
	/* finish initializing the data the user HASN'T put in */
}

void memerranddie(char *file, int line)
{
	fprintf(stderr, "Memory Error in %s:%d\n", file, line);
	exit(1);
}
