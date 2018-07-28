/*
 * Brian Chrzanowski
 * Sat Jul 28, 2018 03:03
 *
 * vert.c
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

void vect_init(void *ptr, int n); /* clear a vector (*ptr) of 'n' elements */

#endif
