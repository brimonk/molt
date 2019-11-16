#ifndef MOLT_COMMON
#define MOLT_COMMON

#include <stdint.h>
#include <stdarg.h>

/* macros to help index into our 2d and 3d arrays */
#define IDX2D(x, y, ylen)          ((x) + (y) * (ylen))
#define IDX3D(x, y, z, ylen, zlen) ((x) + (y) * (ylen) + (z) * (zlen) * (ylen))

#define SWAP(x, y, T) do { T SWAP = x; x = y; y = SWAP; } while (0)
	
#define ARRSIZE(x)   (sizeof((x))/sizeof((x)[0]))

// some fun macros for variadic functions :^)
#define PP_ARG_N( \
          _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
         _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
         _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
         _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
         _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
         _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
         _61, _62, _63, N, ...) N

#define PP_RSEQ_N()                                        \
         63, 62, 61, 60,                                   \
         59, 58, 57, 56, 55, 54, 53, 52, 51, 50,           \
         49, 48, 47, 46, 45, 44, 43, 42, 41, 40,           \
         39, 38, 37, 36, 35, 34, 33, 32, 31, 30,           \
         29, 28, 27, 26, 25, 24, 23, 22, 21, 20,           \
         19, 18, 17, 16, 15, 14, 13, 12, 11, 10,           \
          9,  8,  7,  6,  5,  4,  3,  2,  1,  0

#define PP_NARG_(...)    PP_ARG_N(__VA_ARGS__)

#define PP_NARG(...)     (sizeof(#__VA_ARGS__) - 1 ? PP_NARG_(__VA_ARGS__, PP_RSEQ_N()) : 0)

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

typedef u32 uivec_t;
typedef uivec_t uivec2_t[2];
typedef uivec_t uivec3_t[3];
typedef uivec_t uivec4_t[4];
typedef uivec_t uivec5_t[5];
typedef uivec_t uivec6_t[6];

typedef s64 lvec_t;
typedef lvec_t lvec2_t[2];
typedef lvec_t lvec3_t[3];
typedef lvec_t lvec4_t[4];
typedef lvec_t lvec5_t[5];
typedef lvec_t lvec6_t[6];

typedef u64 ulvec_t;
typedef ulvec_t ulvec2_t[2];
typedef ulvec_t ulvec3_t[3];
typedef ulvec_t ulvec4_t[4];
typedef ulvec_t ulvec5_t[5];
typedef ulvec_t ulvec6_t[6];

typedef void * pvec_t;
typedef pvec_t pvec2_t[2];
typedef pvec_t pvec3_t[3];
typedef pvec_t pvec4_t[4];
typedef pvec_t pvec5_t[5];
typedef pvec_t pvec6_t[6];

typedef f64 dvec_t; // double
typedef dvec_t dvec2_t[2];
typedef dvec_t dvec3_t[3];
typedef dvec_t dvec4_t[4];
typedef dvec_t dvec5_t[5];
typedef dvec_t dvec6_t[6];

typedef f32 fvec_t; // float
typedef fvec_t fvec2_t[2];
typedef fvec_t fvec3_t[3];
typedef fvec_t fvec4_t[4];
typedef fvec_t fvec5_t[5];
typedef fvec_t fvec6_t[6];

typedef f64 * pdvec_t; // double *
typedef pdvec_t pdvec1_t[1]; // double *
typedef pdvec_t pdvec2_t[2];
typedef pdvec_t pdvec3_t[3];
typedef pdvec_t pdvec4_t[4];
typedef pdvec_t pdvec5_t[5];
typedef pdvec_t pdvec6_t[6];

typedef char cvec_t; // character vector
typedef cvec_t cvec2_t[2];
typedef cvec_t cvec3_t[3];
typedef cvec_t cvec4_t[4];
typedef cvec_t cvec5_t[5];
typedef cvec_t cvec6_t[6];

#define BUFSMALL 256
#define BUFLARGE 4096

void print_err_and_die(char *msg, char *file, int line);
#define PRINT_AND_DIE(x) (print_err_and_die((x), __FILE__, __LINE__))

// logging, provide a pointer to some f64 data, the dimensionality of the data,
// and a short message, and it'll be logged to stdout
s32 hunklog_1(char *file, int line, char *msg, ivec_t dim, f64 *p);
s32 hunklog_2(char *file, int line, char *msg, ivec2_t dim, f64 *p);
s32 hunklog_3(char *file, int line, char *msg, ivec3_t dim, f64 *p);
s32 hunklog_3ord(char *file, int line, char *msg, ivec3_t dim, f64 *p, cvec3_t ord);
void perflog_append(char *file, char *func, char *str, s32 line);
void perflog_print(s32 from, s32 to);
void perflog_print_mostrecent();

#define PERFLOG_ADD(s)\
	perflog_append(__FILE__, (char *)__FUNCTION__, (s), __LINE__)
#define PERFLOG_PRINT(s)\
	printf("PerfLog - %s:%d [%s] %s\n", __FILE__, __LINE__, (char *)__FUNCTION__, (s))
#define PERFLOG_SPRINTF(b,s,...)\
	snprintf((b), sizeof((b)), (s), __VA_ARGS__);\
	PERFLOG_PRINT(b)
#define PERFLOG_APP_AND_PRINT(b,s,...)\
	snprintf((b), sizeof((b)), (s), __VA_ARGS__);\
	PERFLOG_PRINT(b);\
	PERFLOG_ADD(s)


#define LOG1D(p, d, m) hunklog_1(__FILE__, __LINE__, (m), (d), (p))
#define LOG2D(p, d, m) hunklog_2(__FILE__, __LINE__, (m), (d), (p))
#define LOG3D(p, d, m) hunklog_3(__FILE__, __LINE__, (m), (d), (p))

#define LOG3DORD(p, d, m, o) \
	hunklog_3ord(__FILE__, __LINE__, (m), (d), (p), (o))
	

#define LOG_NEWLINESEP 1
#define LOG_FLOATFMT "% 4.5e"
// #define LOG_FLOATFMT "%.15lf"

// we don't need a LOGMAKE_1 because that's the trivial case
#define LOGMAKE_2(v,a,b)   ((v)[0]=(a),(v)[1]=(b))
#define LOGMAKE_3(v,a,b,c) ((v)[0]=(a),(v)[1]=(b),(v)[2]=(c))

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
 * VectorMulAdd(v,s,b,o)
 *		Make b s units long, add to v, storing the result in o
 *
 * VectorSet(v,a,b,c)
 *		Sets v to the values a, b, c
 */

#define VectorDotProduct(a,b) ((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define Vec3Sub(a,b,c) \
	((c)[0]=(a)[0]-(b)[0],\
		(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,c) \
		((c)[0]=(a)[0]+(b)[0],\
		 (c)[1]=(a)[1]+(b)[1],\
		 (c)[2]=(a)[2]+(b)[2])
#define VectorCopy(a,b)  ((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define VectorScale(v,s,o) \
	((o)[0]=(v)[0]*(s), (o)[1]=(v)[1]*(s), (o)[2]=(v)[2]*(s))
#define VectorMulAdd(v,s,b,o) \
	((o)[0]=(v)[0]+(b)[0]*(s),\
	 (o)[1]=(v)[1]+(b)[1]*(s),\
	 (o)[2]=(v)[2]+(b)[2]*(s))
#define VectorCrossProduct(a,b,c) \
	((c)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1],\
	 (c)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2],\
	 (c)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0])
#define VectorClear(a) ((a)[0]=0,(a)[1]=0,(a)[2]=0)
#define VectorPrint(a) (printf("%lf, %lf, %lf\n", (a)[0], (a)[1], (a)[2]))

// now for some of my own
#define Vec2Set(v,a,b)     ((v)[0]=(a),(v)[1]=(b))
#define Vec3Set(v,a,b,c)   ((v)[0]=(a),(v)[1]=(b),(v)[2]=(c))
#define Vec4Set(v,a,b,c,d) ((v)[0]=(a),(v)[1]=(b),(v)[2]=(c),(v)[3]=(d))

#define Vec2Copy(a,b)      ((a)[0]=(b)[0],(a)[1]=(b)[1])
#define Vec3Copy(a,b)      ((a)[0]=(b)[0],(a)[1]=(b)[1],(a)[2]=(b)[2])
#define Vec4Copy(a,b)      ((a)[0]=(b)[0],(a)[1]=(b)[1],(a)[2]=(b)[2],(a)[3]=(b)[3])

#define Vec3Scale(o,i,s)   ((o)[0]=(i)[0]*(s),(o)[1]=(i)[1]*(s),(o)[2]=(i)[2]*(s))

#endif

