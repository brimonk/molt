#ifndef MOLT_COMMON
#define MOLT_COMMON

#include <stdint.h>

/* macros to help index into our 2d and 3d arrays */
#define IDX2D(x, y, ylen)    ((x) + ((ylen) * (y)))
#define IDX3D(x, y, z, ylen, zlen) \
	((x) + ((y) * (ylen)) + ((z) * (ylen) * (zlen)))

#define SWAP(x, y, T) do { T SWAP = x; x = y; y = SWAP; } while (0)

void print_err_and_die(char *msg, char *file, int line);

#define PRINT_AND_DIE(x) (print_err_and_die((x), __FILE__, __LINE__))

/* quick and dirty, cleaner typedefs */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef float    f32;
typedef double   f64;

/* begin by defining vectors of multiple dimensionality */
typedef double vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];
typedef vec_t vec6_t[6];

/* now define vectors of double pointers */
typedef double * pvec_t;
typedef pvec_t pvec2_t[2];
typedef pvec_t pvec3_t[3];
typedef pvec_t pvec4_t[4];
typedef pvec_t pvec5_t[5];
typedef pvec_t pvec6_t[6];

/* 
 * we can actually define inline vector operations
 *		Quake-III-Arena:q_shared.h, ~line 445
 * Here's some explanation
 *
 * VectorDotProduct(a, b)
 *		(a[0] * b[0] + a[1] * b[1] + a[2] * b[2])
 *
 * VectorSubtract(a,b,c)
 *		Subtract b from a, storing the result in c
 *
 * VectorAdd(a,b,c)
 *		Add a to b, storing the result in c
 *
 * VectorCopy(a,b)
 *		Copies the vector a to b
 *
 * VectorScale(v,s,o)
 *		make v, s units long, storing the result in o
 *
 * VectorMA(v,s,b,o)
 *		Make b s units long, add to v, storing the result in o
 */

#define VectorDotProduct(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VectorSubtract(a,b,c) ((c)[0]=(a)[0]-(b)[0],\
		(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,c) ((c)[0]=(a)[0]+(b)[0],\
		(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define VectorCopy(a,b)  ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define VectorScale(v,s,o) ((o)[0]=(v)[0]*(s),\
		(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define VectorMA(v,s,b,o) ((o)[0]=(v)[0]+(b)[0]*(s),\
		(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))
#define VectorCrossProduct(a,b,c) \
	((c)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1],\
	 (c)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2],\
	 (c)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0])
#define VectorClear(a) ((a)[0]=0,(a)[1]=0,(a)[2]=0)
#define VectorPrint(a) \
	(printf("%lf, %lf, %lf\n", (a)[0], (a)[1], (a)[2]))

/*
 * define simulation information
 */

/* include the configuration header for config values */
#include "config.h"

#define MOLTLUMP_MAGIC *(int *)"MOLT"
#define MOLTLUMP_TOTAL 8
#define MOLTCURRVERSION 1

enum { /* type values */
	MOLTLUMP_TYPEBIO, /* biological */
};

enum {
	MOLTLUMP_CONFIG,
	MOLTLUMP_RUNINFO,
	MOLTLUMP_EFIELD,
	MOLTLUMP_PFIELD
};

struct lump_t {
	u64 offset;   /* in bytes from the beginning */
	u64 lumpsize; /* in bytes, total. record_number = size / sizeof(x) */
	u64 elemsize; /* sizeof(x) */
};

struct lump_header_t {
	u32 magic;
	u16 version;
	u16 type;
	struct lump_t lump[MOLTLUMP_TOTAL];
};

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

struct lump_runinfo_t {
	f64 time_start;
	f64 time_step;
	f64 time_stop;
	s32 curr_idx;
	s32 total_steps;
};
struct lump_efield_t {
	int xlen, ylen, zlen;
	f64 data[MOLT_XPOINTS * MOLT_YPOINTS * MOLT_ZPOINTS];
};

struct lump_pfield_t {
	int xlen, ylen, zlen;
	f64 data[MOLT_XPOINTS * MOLT_YPOINTS * MOLT_ZPOINTS];
};

#endif
