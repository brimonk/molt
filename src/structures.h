#ifndef MOLT_STRUCTURES
#define MOLT_STRUCTURES

#include "vect.h"

/*
 * Here, nearly every fixed length record is defined.
 */

/* moltcfg : to keep simulation parameters within the storage system */
struct moltcfg_t {
	/* free space parms */
	double lightspeed;
	double henrymeter;
	double faradmeter;

	/* tissue space parms */
	double staticperm;
	double infiniteperm;
	double tau;
	double distribtail;
	double distribasym;
	double tissuespeed;

	/* mesh parms */
	double step_in_sec;
	double step_in_x;
	double step_in_y;
	double step_in_z;
	double cfl;

	/* dimension parms */
	double sim_time;
	double sim_x;
	double sim_y;
	double sim_z;
	long sim_time_steps;
	long sim_x_steps;
	long sim_y_steps;
	long sim_z_steps;

	/* MOLT parameters */
	double space_accuracy;
	double time_accuracy;
	double beta;
	double alpha;
};

// this should probably be handled separately
struct run_info_t {
	double time_start;
	double time_step;
	double time_stop;
	int run_number;
	int time_idx;
};

struct part_t {
	double x;
	double y;
	double z;
	double vx;
	double vy;
	double vz;
};

struct molt_t {
	struct part_t *parts;
	long part_total;
	struct run_info_t info;
	vec3_t e_field;
	vec3_t b_field;
};

#endif

