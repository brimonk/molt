/*
 * Brian Chrzanowski
 * Tue Jan 22, 2019 15:17
 *
 * Commonly Shared Structures and Definitions
 */

#ifndef MOLTSHARED
#define MOLTSHARED

#include <stdint.h>

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

/* define simulation information */

#define MOLTLUMP_MAGIC *(int *)"MOLT"
#define MOLTCURRVERSION 1

enum { /* type values */
	MOLTLUMP_TYPEBIO, /* biological */
	MOLTLUMP_TYPEPLA, /* plasma kinematics */
};

enum {
	MOLTLUMP_POSITIONS,
	MOLTLUMP_MATRIX
};

struct lump_t {
	uint32_t offset; /* in bytes from the beginning */
	uint32_t size;   /* in bytes, total. record_number = size / sizeof(x) */
};

struct lump_header_t {
	uint32_t magic;
	uint16_t version;
	uint16_t type;
	struct lump_t lump[2];
};

struct lump_pos_t {
	/* uses MOLTLUMP_POSITIONS */
	u16 x;
	u16 y;
	u16 z;
};

struct lump_mat_t {
	/* uses MOLTLUMP_MATRIX */
	u16 n;
	f64 *mat;
};

#endif

