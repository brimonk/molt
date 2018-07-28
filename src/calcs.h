/* simple function declarations */
#include "particles.h"

void part_pos_update(struct particle_t *parts, double change);
void part_vel_update(struct particle_t *part, vec3_t *e_field, vec3_t *b_field, double change);
double force_in_x(struct particle_t *part, vec3_t e_fld, vec3_t b_fld);
double force_in_y(struct particle_t *part, vec3_t e_fld, vec3_t b_fld);
double force_in_z(struct particle_t *part, vec3_t e_fld, vec3_t b_fld);
