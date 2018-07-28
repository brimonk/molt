/*
 * Brian Chrzanowski
 * Fri Jul 27, 2018 21:04
 *
 * an aggregation of functions related to any semi-complex calculation
 */

#include "particles.h"
#include "calcs.h"

// updating position values
void part_pos_update(struct particle_t *parts, double change)
{
	parts->pos[0] = parts->pos[0] + change * parts->vel[0];
	parts->pos[1] = parts->pos[1] + change * parts->vel[1];
	parts->pos[2] = parts->pos[2] + change * parts->vel[2];
}

// updating velocity values
void part_vel_update(struct particle_t *part, vec3_t *e_field, vec3_t *b_field, double change)
{
	part->vel[0] = part->vel[0] + change * force_in_x(part, *e_field, *b_field);
	part->vel[1] = part->vel[1] + change * force_in_y(part, *e_field, *b_field);
	part->vel[2] = part->vel[2] + change * force_in_z(part, *e_field, *b_field);
}

// dynamic force calculation
double force_in_x(struct particle_t *part, vec3_t e_fld, vec3_t b_fld)
{
	// Ex + vy*Bz - vz*By;
    return e_fld[0] + (part->vel[0] * b_fld[2]) - (part->vel[2] * b_fld[1]);
}

double force_in_y(struct particle_t *part, vec3_t e_fld, vec3_t b_fld)
{
    // Ey + vz*Bx - vx*Bz;
    return e_fld[1] + (part->vel[2] * b_fld[0]) - (part->vel[0] * b_fld[2]);
}

double force_in_z(struct particle_t *part, vec3_t e_fld, vec3_t b_fld)
{
    // Ez + vx*By - vy*Bx;
    return e_fld[2] + (part->vel[0] * b_fld[1]) - (part->vel[1] * b_fld[0]);
}

