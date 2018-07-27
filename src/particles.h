/*
 * Brian Chrzanowski
 * Fri Jul 20, 2018 13:47
 *
 * defines particles and their related structures
 */

struct position_t {
	double x, y, z;
};

struct velocity_t {
	double x, y, z;
};

struct particle_t {
	struct position_t pos;
	struct velocity_t vel;
};

