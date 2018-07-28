/*
 * Brian Chrzanowski
 * Fri Jul 20, 2018 13:47
 *
 * defines particles and their related structures
 *
 * Look here: http://wiki.ioquake3.org/Vectors for information on why this
 * is written as it is.
 */

#ifndef __PARTICLES__
#define __PARTICLES__

#include "vert.h"

/*
 * Not only does this dimensionaly expand as far as needed, but we can now
 * have particles in whatever dimensionality we desire.
 *
 * Shown here is a particle3_t, a particle with 3d position and velocity
 */

struct particle_t {
	unsigned int uid; /* s^32 - 1 max particles (shouldn't be an issue) */
	vec3_t pos;
	vec3_t vel;
};

#endif

