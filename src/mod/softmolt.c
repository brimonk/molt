/*
 * Brian Chrzanowski
 * Tue Jan 01, 2019 12:25 
 *
 * "Software MOLT" Module
 *
 * Dynamically loadable MOLT module that implements the computation for one
 * timeslice. These computations include, but are not limited to (TODO: fill out)
 *
 * 1. Force Updates
 * 2. Particle Updates
 * 2a. Particle Position
 * 2b. Particle Velocity
 * 3. Field Updates
 * 4. Vertex Summation 
 *
 * MODULE REQUIREMENTS
 *
 * Each module has very different requirements for startup, shutdown, etc. 
 * It is EXPECTED that each module use GCC and CLANG attributes to mark
 * functions as the module's constructor or destructor. The function marked as
 * a constructor gets executed as the shared object is loaded into memory, and
 * before returning an opaque handle to the object, and likewise, the destructor
 * is executed just before the shared object's memory gets released.
 *
 * For functions that don't need any constructing, or deconstructing, these
 * could (probably) be safely excluded. Here, they're included for simplicity.
 *
 * As of now it is ALSO EXPECTED that the function that will perform the
 * timeslice be called "doslice". As of writing, I'm looking into a way to mark
 * any function with an attribute (something like __attribute__((doslice))) and
 * load the function to perform the timeslice off of that marked attribute, but
 * I have a feeling that isn't possible. Still looking though.
 */

#include "../structures.h"
#include "../vect.h"

void part_pos_update(struct molt_t *molt, long id, double change);
void part_vel_update(struct molt_t *molt, long id, double change);
double force_in_x(struct molt_t *molt, int id);
double force_in_y(struct molt_t *molt, int id);
double force_in_z(struct molt_t *molt, int id);

/* softmolt_init : libsoftmolt constructor */
__attribute__((constructor))
int softmolt_init()
{
	return 0;
}

/* softmolt_free : libsoftmolt deconstructor */
__attribute__((destructor))
int softmolt_free()
{
	return 0;
}

/* doslice : computes the next molt timeslice using the CPU "naively" */
int doslice(struct molt_t *molt)
{
	long i;

	for (i = 0; i < molt->part_total; i++) {
		part_pos_update(molt, i, molt->info.time_step);
		part_vel_update(molt, i, molt->info.time_step);
	}

	return 0;
}

/* part_pos_update : update a particle's position */
void part_pos_update(struct molt_t *molt, long id, double change)
{
	vec3_t tmp;
	VectorClear(tmp);

	tmp[0] = molt->parts[id].x + change * molt->parts[id].vx;
	tmp[1] = molt->parts[id].y + change * molt->parts[id].vy;
	tmp[2] = molt->parts[id].z + change * molt->parts[id].vz;

	molt->parts[id].x = tmp[0];
	molt->parts[id].y = tmp[1];
	molt->parts[id].z = tmp[2];
}

/* part_vel_update : update a particle's velocity */
void part_vel_update(struct molt_t *molt, long id, double change)
{
	vec3_t tmp;
	VectorClear(tmp);

	tmp[0] = molt->parts[id].vx + change * force_in_x(molt, id);
	tmp[1] = molt->parts[id].vy + change * force_in_y(molt, id);
	tmp[2] = molt->parts[id].vz + change * force_in_z(molt, id);

	molt->parts[id].x = tmp[0];
	molt->parts[id].y = tmp[1];
	molt->parts[id].z = tmp[2];
}

/* force_in_x : calculate the electric field in X */
double force_in_x(struct molt_t *molt, int id)
{
	// Ex + vy*Bz - vz*By;
    return molt->e_field[0]
		+ (molt->parts[id].vy * molt->b_field[2])
		- (molt->parts[id].vz * molt->b_field[1]);
}

/* force_in_y : calculate the electric field in Y */
double force_in_y(struct molt_t *molt, int id)
{
    // Ey + vz*Bx - vx*Bz;
    return molt->e_field[1]
		+ (molt->parts[id].vz * molt->b_field[0])
		- (molt->parts[id].vx * molt->b_field[2]);
}

/* force_in_z : calculate the electric field in Z */
double force_in_z(struct molt_t *molt, int id)
{
    // Ez + vx*By - vy*Bx;
    return molt->e_field[2]
		+ (molt->parts[id].vx * molt->b_field[1])
		- (molt->parts[id].vy * molt->b_field[0]);
}

