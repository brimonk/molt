/*
 * Brian Chrzanowski
 * Fri Jul 27, 2018 21:04
 *
 * a collection of calculations
 */

#include "structures.h"
#include "vect.h"
#include "calcs.h"

// updating position values
void part_pos_update(struct molt_t *molt, int id, double change)
{
	vec3_t tmp;
	VectorClear(tmp);

	tmp[0] = molt->x[id] + change * molt->vx[id];
	tmp[1] = molt->y[id] + change * molt->vy[id];
	tmp[2] = molt->z[id] + change * molt->vz[id];

	molt->x[id] = tmp[0];
	molt->y[id] = tmp[1];
	molt->z[id] = tmp[2];
}

// updating velocity values
void part_vel_update(struct molt_t *molt, int id, double change)
{
	vec3_t tmp;
	VectorClear(tmp);

	tmp[0] = molt->vx[id] + change * force_in_x(molt, id);
	tmp[1] = molt->vy[id] + change * force_in_y(molt, id);
	tmp[2] = molt->vz[id] + change * force_in_z(molt, id);

	molt->x[id] = tmp[0];
	molt->y[id] = tmp[1];
	molt->z[id] = tmp[2];
}

// dynamic force calculation
double force_in_x(struct molt_t *molt, int id)
{
	// Ex + vy*Bz - vz*By;
    return molt->e_field[0]
		+ (molt->vy[id] * molt->b_field[2])
		- (molt->vz[id] * molt->b_field[1]);
}

double force_in_y(struct molt_t *molt, int id)
{
    // Ey + vz*Bx - vx*Bz;
    // return e_fld[1] + (part->vel[2] * b_fld[0]) - (part->vel[0] * b_fld[2]);
    return molt->e_field[1]
		+ (molt->vz[id] * molt->b_field[0])
		- (molt->vx[id] * molt->b_field[2]);
}

double force_in_z(struct molt_t *molt, int id)
{
    // Ez + vx*By - vy*Bx;
    // return e_fld[2] + (part->vel[0] * b_fld[1]) - (part->vel[1] * b_fld[0]);
    return molt->e_field[2]
		+ (molt->vx[id] * molt->b_field[1])
		- (molt->vy[id] * molt->b_field[0]);
}

