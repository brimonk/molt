/* simple function declarations */
#include "structures.h"

void part_pos_update(struct molt_t *molt, int id, double change);
void part_vel_update(struct molt_t *molt, int id, double change);
double force_in_x(struct molt_t *molt, int id);
double force_in_y(struct molt_t *molt, int id);
double force_in_z(struct molt_t *molt, int id);
