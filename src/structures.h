#ifndef STRUCTURES
#define STRUCTURES

#include "vect.h"

// this should probably be handled separately
struct run_info_t {
	int run_number;
	int time_idx;
	double time_start;
	double time_step;
	double time_stop;
};

struct molt_t {
	double *x;
	double *y;
	double *z;
	double *vx;
	double *vy;
	double *vz;
	struct run_info_t info;
	long part_total;
	vec3_t e_field;
	vec3_t b_field;
};

#endif

