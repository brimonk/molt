#ifndef MOLT_COMMON
#define MOLT_COMMON

#include <stdint.h>

/* macros to help index into our 2d and 3d arrays */
#define ARRSIZE(x)           ((sizeof(x)) / sizeof(*(x)))
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
	MOLTLUMP_TYPEBIO = 1 /* biological */
};

enum {
	MOLTLUMP_CONFIG,
	MOLTLUMP_RUNINFO,
	MOLTLUMP_EFIELD,
	MOLTLUMP_PFIELD,
	MOLTLUMP_NU,
	MOLTLUMP_VWEIGHT,
	MOLTLUMP_WWEIGHT,
	MOLTLUMP_PSTATE
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
	f64 lightspeed;
	f64 henrymeter;
	f64 faradmeter;
	/* tissue space parms */
	f64 staticperm;
	f64 infiniteperm;
	f64 tau;
	f64 distribtail;
	f64 distribasym;
	f64 tissuespeed;
	/* mesh parms */
	f64 int_scale;
	f64 step_in_sec;
	f64 step_in_x;
	f64 step_in_y;
	f64 step_in_z;
	f64 cfl;
	/* dimension parms */
	f64 sim_time;
	f64 sim_x;
	f64 sim_y;
	f64 sim_z;
	s64 sim_time_steps;
	s64 sim_x_steps;
	s64 sim_y_steps;
	s64 sim_z_steps;
	/* MOLT parameters */
	f64 space_accuracy;
	f64 time_accuracy;
	f64 beta;
	f64 alpha;
};

struct lump_runinfo_t {
	f64 time_start;
	f64 time_step;
	f64 time_stop;
	s32 curr_idx;
	s32 total_steps;
};

struct lump_efield_t {
	f64 data[MOLT_XPOINTS * MOLT_YPOINTS * MOLT_ZPOINTS];
};

struct lump_pfield_t {
	f64 data[MOLT_XPOINTS * MOLT_YPOINTS * MOLT_ZPOINTS];
};

struct lump_nu_t {
	f64 nux[MOLT_TOTALWIDTH];
	f64 nuy[MOLT_TOTALDEEP];
	f64 nuz[MOLT_TOTALHEIGHT];
};

struct lump_vweight_t {
	f64 vrx[MOLT_XPOINTS];
	f64 vlx[MOLT_XPOINTS];
	f64 vry[MOLT_YPOINTS];
	f64 vly[MOLT_YPOINTS];
	f64 vrz[MOLT_ZPOINTS];
	f64 vlz[MOLT_ZPOINTS];
};

struct lump_wweight_t {
	f64 xr_weight[MOLT_TOTALWIDTH * (MOLT_SPACEACC + 1)];
	f64 xl_weight[MOLT_TOTALWIDTH * (MOLT_SPACEACC + 1)];
	f64 yr_weight[MOLT_TOTALDEEP * (MOLT_SPACEACC + 1)];
	f64 yl_weight[MOLT_TOTALDEEP * (MOLT_SPACEACC + 1)];
	f64 zr_weight[MOLT_TOTALHEIGHT * (MOLT_SPACEACC + 1)];
	f64 zl_weight[MOLT_TOTALHEIGHT * (MOLT_SPACEACC + 1)];
};

struct lump_pstate_t { // problem state (the thing we simulate)
	f64 data[MOLT_XPOINTS * MOLT_YPOINTS * MOLT_ZPOINTS];
};

#endif
