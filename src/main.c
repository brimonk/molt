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

#include "sqlite3.h"
#include "particles.h"
#include "io.h"

#include "field_updates.h"
#include "calcs.h"

// necessary prototypes
struct vector vectorize(float, float, float);
float Fx(float, float, float, float, float, float, float, float, float);
float Fy(float, float, float, float, float, float, float, float, float);
float Fz(float, float, float, float, float, float, float, float, float);
int molt_run(sqlite3 *db);
void vect_init(void *ptr, int n);

// declaring global variables
// struct vector s[999][999], v[999][999]; // position, velocity

// user defined libraries
// #include "forceCalculation.h" // functinos to calculate the force

#define DATABASE "molt_output.db"
#define CURR_FLOATING_TIME(idx, step, init) ((idx + step + init))

char *io_particle_insert =
"insert into particles (run_index, time, particle_index, x_pos, y_pos, z_pos,"\
		"x_vel, y_vel, z_vel) values (?, ?, ?, ?, ?, ?, ?, ?, ?);";

int main(int argc, char **argv)
{
    int val;
	sqlite3 *db;
    
	/* set up the database */
	val = sqlite3_open(DATABASE, &db);

	if (val != SQLITE_OK) {
		SQLITE3_ERR(val);
	}

	io_exec_sql_tbls(db, io_db_tbls);

	molt_run(db);

	sqlite3_close(db);

	return 0;
}

int molt_run(sqlite3 *db)
{
	vec3_t e_field, b_field;
	vec3_t tm_vect;
	double tm_initial, tm_curr, tm_step, tm_final = 0;
    int time_i, max_iter, i, total_parts = 0; // timeInd indicates the iteration in time that the simulation is at
	struct particle_t *dummy_arr;

	vect_init(&e_field, 3);
	vect_init(&b_field, 3);

	/* do some initialization right here */

	total_parts = 5;
	max_iter = (int)(tm_final / tm_step);

	/* iterate over each time step, taking our snapshot */
	for (time_i = 0; time_i < max_iter; time_i++) {
        for(i = 0; i < total_parts; i++) // repeating funtions per number of particles
        {
            field_update(&dummy_arr[i], &e_field, &b_field);
			part_pos_update(&dummy_arr[i], tm_step);

			/* store to disk here??? */

			part_vel_update(&dummy_arr[i], &e_field, &b_field, tm_step);
            // updatePosition(i, timeInd); // updating position values
            // updateVelocity(i, timeInd); // updating velocity values
        }

		tm_curr = CURR_FLOATING_TIME(time_i, tm_step, tm_initial);

		/* done with a time iteration, dump to the output */
		// io_insert(db, io_particle_insert, NULL);
    }
    
    // fileManipulation(timeInd, loopCount, parNo, noPar); // writes all the data to .csv files, to be later read and plotted by Matlab

	return 0;
}

void vect_init(void *ptr, int n)
{
	/* initializes a vector of size 'n' with zeroes */
	memset(ptr, 0, sizeof(double) * n);
}
