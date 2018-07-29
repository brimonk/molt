/*
 * Brian Chrzanowski
 * Sat Jul 28, 2018 03:03
 *
 * vect.c
 *   - A small vertex library, inspired by the Quake functions of similar use.
 */

#ifndef __VERTICIES__
#define __VERTICIES__

/* begin by defining vectors of multiple dimensionality */
typedef double vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

/* 
 * we can actually define inline vector operations
 *		Quake-III-Arena:q_shared.h starting at line 445
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
#define VectorCopy(a,b)  ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[1])
#define VectorScale(v,s,o) ((o)[0]=(v)[0]*(s),\
		(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define VectorMA(v,s,b,o) ((o)[0]=(v)[0]+(b)[0]*(s),\
		(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))
#define VectorCrossProduct(a,b,c) \
	((c)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1],\
	 (c)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2],\
	 (c)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0])
#define VectorClear(a) ((a)[0]=0,(a)[1]=0,(a)[2]=0)

/* 
 * also handled in the vect library are C++-like vectors, or dynamic arrays
 * (this is a little confusing, and a better way to organize these two things
 * would be great).
 */

struct dynarr_t {
	int obj_size;
	int curr_size;
	int max_size;

	void *data;
} dynarr;

#define DYNARR_INITIAL_CAPACITY 64

void dynarr_init(struct dynarr_t *ptr, int obj_size);
void dynarr_append(struct dynarr_t *ptr, void *add_me);
void *dynarr_get(struct dynarr_t *ptr, int index);
void dynarr_set(struct dynarr_t *ptr, int index, void *value);
void dynarr_free(struct dynarr_t *ptr);

#endif
