/*
 * Brian Chrzanowski
 * Fri Jul 20, 2018 13:47
 *
 * defines particles and their related structures
 */

#ifndef __PARTICLES__
#define __PARTICLES__

#include "vect.h"

/*
 * Not only does this dimensionality expand as far as needed, but we can now
 * have particles in whatever dimensionality we desire.
 *
 * Shown here is a particle3_t, a particle with 3d position and velocity
 */

struct particle_t {
	unsigned int uid; /* s^32 - 1 max particles (shouldn't be an issue) */
	vec3_t pos;
	vec3_t vel;
}; // size => 52 bytes

// this should probably be handled separately
struct run_info_t {
	int run_number;
	int time_idx;
	double time_start;
	double time_step;
	double time_stop;
};

struct field_combo_t {
	vec3_t *e_field;
	vec3_t *b_field;
};

#endif

