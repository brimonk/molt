/*
 * Brian Chrzanowski
 * Fri Jul 27, 2018 21:04
 *
 * a collection of calculations
 */

#include "structures.h"
#include "vect.h"
#include "calcs.h"

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

