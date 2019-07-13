#ifndef MOLT_COMMON
#define MOLT_COMMON

#include <stdint.h>

/* macros to help index into our 2d and 3d arrays */
#define IDX2D(x, y, ylen)    ((x) + ((ylen) * (y)))
#define IDX3D(x, y, z, ylen, zlen) ((x) + (y) * (ylen) + (z) * (zlen) * (ylen))

#define SWAP(x, y, T) do { T SWAP = x; x = y; y = SWAP; } while (0)

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
typedef s32 ivec_t;
typedef ivec_t ivec2_t[2];
typedef ivec_t ivec3_t[3];
typedef ivec_t ivec4_t[4];
typedef ivec_t ivec5_t[5];
typedef ivec_t ivec6_t[6];

typedef void * pvec_t;
typedef pvec_t pvec2_t[2];
typedef pvec_t pvec3_t[3];
typedef pvec_t pvec4_t[4];
typedef pvec_t pvec5_t[5];
typedef pvec_t pvec6_t[6];

typedef f64 dvec_t; /* floating point vectors */
typedef dvec_t dvec2_t[2];
typedef dvec_t dvec3_t[3];
typedef dvec_t dvec4_t[4];
typedef dvec_t dvec5_t[5];
typedef dvec_t dvec6_t[6];

typedef char cvec_t; /* some less useful character vectors */
typedef cvec_t cvec2_t[2];
typedef cvec_t cvec3_t[3];
typedef cvec_t cvec4_t[4];
typedef cvec_t cvec5_t[5];
typedef cvec_t cvec6_t[6];

#define BUFSMALL 32
#define BUFLARGE 128

void print_err_and_die(char *msg, char *file, int line);
#define PRINT_AND_DIE(x) (print_err_and_die((x), __FILE__, __LINE__))

// logging, provide a pointer to some f64 data, the dimensionality of the data,
// and a short message, and it'll be logged to stdout
s32 hunklog_1(char *file, int line, char *msg, ivec_t dim, f64 *p);
s32 hunklog_2(char *file, int line, char *msg, ivec2_t dim, f64 *p);
s32 hunklog_3(char *file, int line, char *msg, ivec3_t dim, f64 *p);

#define LOG1D(p, d, m) hunklog_1(__FILE__, __LINE__, (m), (d), (p))
#define LOG2D(p, d, m) hunklog_2(__FILE__, __LINE__, (m), (d), (p))
#define LOG3D(p, d, m) hunklog_3(__FILE__, __LINE__, (m), (d), (p))
#define LOG_NEWLINESEP 1
#define LOG_FLOATFMT "% 4.6e"

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
#define VectorAdd(a,b,c) \
		((c)[0]=(a)[0]+(b)[0],\
		 (c)[1]=(a)[1]+(b)[1],\
		 (c)[2]=(a)[2]+(b)[2])
#define VectorCopy(a,b)  ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define VectorScale(v,s,o) \
	((o)[0]=(v)[0]*(s), (o)[1]=(v)[1]*(s), (o)[2]=(v)[2]*(s))
#define VectorMA(v,s,b,o) \
	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))
#define VectorCrossProduct(a,b,c) \
	((c)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1],\
	 (c)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2],\
	 (c)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0])
#define VectorClear(a) ((a)[0]=0,(a)[1]=0,(a)[2]=0)
#define VectorPrint(a) (printf("%lf, %lf, %lf\n", (a)[0], (a)[1], (a)[2]))

/* simulation information */

/* include the configuration header for config values */
#include "config.h"

#define MOLTLUMP_MAGIC *(s32 *)"MOLT"
#define MOLTLUMP_TOTAL 8
#define MOLTCURRVERSION 1

enum { /* type values */
	MOLTLUMP_TYPENULL,
	MOLTLUMP_TYPEBIO,   // bio
	MOLTLUMP_TYPETOTAL
};

enum {
	MOLTLUMP_CONFIG,
	MOLTLUMP_RUNINFO,
	MOLTLUMP_NU,
	MOLTLUMP_VWEIGHT,
	MOLTLUMP_WWEIGHT,
	MOLTLUMP_EFIELD,
	MOLTLUMP_PFIELD,
	MOLTLUMP_MESH
};

// meta data for a single lump
// the version and type fields are only used in the header, but for debugging
// purposes, every structure has these fields
struct lumpmeta_t {
	u32 magic;
	u16 version;
	u16 type;
};

struct lump_t {
	u64 offset;   /* in bytes from the beginning */
	u64 lumpsize; /* in bytes, total. record_number = size / sizeof(x) */
	u64 elemsize; /* sizeof(x) */
};

struct lump_header_t {
	struct lumpmeta_t meta;
	struct lump_t lump[MOLTLUMP_TOTAL];
};

struct cfg_t {
	struct lumpmeta_t meta;

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
	f64 cfl;

	// simulation values are kept as integers, and are scaled by the
	// following value
	f64 int_scale;

	s32 t_start;
	s32 t_stop;
	s32 t_step;
	s32 t_points;
	s32 t_points_inc;

	s32 x_start;
	s32 x_stop;
	s32 x_step;
	s32 x_points;
	s32 x_points_inc;

	s32 y_start;
	s32 y_stop;
	s32 y_step;
	s32 y_points;
	s32 y_points_inc;

	s32 z_start;
	s32 z_stop;
	s32 z_step;
	s32 z_points;
	s32 z_points_inc;

	/* MOLT parameters */
	f64 space_acc;
	f64 time_acc;
	f64 beta;
	f64 beta_sq; // b^2 term in molt (for precomputation)
	f64 beta_fo; // b^4 term in molt
	f64 beta_si; // b^6 term in molt
	f64 alpha;

	/* 
	 * store some dimensionality for easier use
	 * NOTE (brian)
	 *
	 * these dimensions are supposed to be used for the ease of printf
	 * debugging and implementation
	 */

	// lump_nu_t dimensions
	ivec_t nux_dim;
	ivec_t nuy_dim;
	ivec_t nuz_dim;
	ivec_t dnux_dim;
	ivec_t dnuy_dim;
	ivec_t dnuz_dim;

	// lump_vweight_t dimensions
	ivec_t vlx_dim;
	ivec_t vrx_dim;
	ivec_t vly_dim;
	ivec_t vry_dim;
	ivec_t vlz_dim;
	ivec_t vrz_dim;

	// lump_wweight_t dimensions
	ivec2_t xl_weight_dim;
	ivec2_t xr_weight_dim;
	ivec2_t yl_weight_dim;
	ivec2_t yr_weight_dim;
	ivec2_t zl_weight_dim;
	ivec2_t zr_weight_dim;

	// the large hunk dimensionality
	ivec3_t efield_data_dim;
	ivec3_t pfield_data_dim;
	ivec3_t umesh_dim;
	ivec3_t vmesh_dim;
};

struct lump_runinfo_t {
	struct lumpmeta_t meta;
	f64 t_start;
	f64 t_step;
	f64 t_stop;
	s32 t_idx;
	s32 t_total;
};

struct lump_nu_t {
	struct lumpmeta_t meta;
	f64 nux[MOLT_X_POINTS];
	f64 nuy[MOLT_Y_POINTS];
	f64 nuz[MOLT_Z_POINTS];
	f64 dnux[MOLT_X_POINTS];
	f64 dnuy[MOLT_Y_POINTS];
	f64 dnuz[MOLT_Z_POINTS];
};

struct lump_vweight_t {
	struct lumpmeta_t meta;
	f64 vlx[MOLT_X_POINTS_INC];
	f64 vrx[MOLT_X_POINTS_INC];
	f64 vly[MOLT_Y_POINTS_INC];
	f64 vry[MOLT_Y_POINTS_INC];
	f64 vlz[MOLT_Z_POINTS_INC];
	f64 vrz[MOLT_Z_POINTS_INC];
};

struct lump_wweight_t {
	struct lumpmeta_t meta;
	f64 xl_weight[MOLT_X_POINTS * (MOLT_SPACEACC + 1)];
	f64 xr_weight[MOLT_X_POINTS * (MOLT_SPACEACC + 1)];
	f64 yl_weight[MOLT_Y_POINTS * (MOLT_SPACEACC + 1)];
	f64 yr_weight[MOLT_Y_POINTS * (MOLT_SPACEACC + 1)];
	f64 zl_weight[MOLT_Z_POINTS * (MOLT_SPACEACC + 1)];
	f64 zr_weight[MOLT_Z_POINTS * (MOLT_SPACEACC + 1)];
};

// the efield and the pfield are fields for storing more simulation data
// TODO : get details on this from Causley, we currently don't use this
struct lump_efield_t {
	struct lumpmeta_t meta;
	f64 data[MOLT_X_POINTS_INC * MOLT_Y_POINTS_INC * MOLT_Z_POINTS_INC];
};

struct lump_pfield_t {
	struct lumpmeta_t meta;
	f64 data[MOLT_X_POINTS_INC * MOLT_Y_POINTS_INC * MOLT_Z_POINTS_INC];
};

struct lump_mesh_t { // problem state (the thing we simulate)
	struct lumpmeta_t meta;
	f64 umesh[MOLT_X_POINTS_INC * MOLT_Y_POINTS_INC * MOLT_Z_POINTS_INC];
	f64 vmesh[MOLT_X_POINTS_INC * MOLT_Y_POINTS_INC * MOLT_Z_POINTS_INC];
};

/* typedefs for all of the structures */
typedef struct lump_runinfo_t lrun_t;
typedef struct lump_efield_t  lefield_t;
typedef struct lump_pfield_t  lpfield_t;
typedef struct lump_nu_t      lnu_t;
typedef struct lump_vweight_t lvweight_t;
typedef struct lump_wweight_t lwweight_t;
typedef struct lump_mesh_t    lmesh_t;

// helper structures
struct vweight {
	f64 *read;
	f64 *write;
};

#endif

