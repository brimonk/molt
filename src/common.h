#if !defined(COMMON_H)
#define COMMON_H

/*
 * Brian Chrzanowski
 * Thu Jan 16, 2020 16:01
 *
 * Brian's Common Module
 *
 * USAGE
 *
 * In at least one source file, do this:
 *    #define COMMON_IMPLEMENTATION
 *    #include "common.h"
 *
 * and compile regularly.
 *
 * TODO (cleanup)
 * 1. Make a common module prefix (c_), and prepend it to common functions
 * 2. Reorganize the #defines in this header file
 */

#include <stdio.h>

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <assert.h>

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
typedef uint8_t      u8;
typedef uint16_t     u16;
typedef uint32_t     u32;
typedef uint64_t     u64;
typedef int8_t       s8;
typedef int16_t      s16;
typedef int32_t      s32;
typedef int64_t      s64;
typedef float        f32;
typedef double       f64;

#define BUFSMALL (256)
#define BUFLARGE (4096)
#define BUFGIANT (1 << 20 << 1)

// MOLT VECTOR DEFINITION
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



/* macros to help index into our 2d and 3d arrays */
#define IDX2D(x, y, ylen)          ((x) + (y) * (ylen))
#define IDX3D(x, y, z, ylen, zlen) ((x) + (y) * (ylen) + (z) * (zlen) * (ylen))



/* c_resize : resizes the ptr should length and capacity be the same */
void c_resize(void *ptr, size_t *len, size_t *cap, size_t bytes);

#define C_RESIZE(x,y,z) (c_resize((x),y##_len,y##_cap,z))

// NOTE (brian) the regular expression functions were stolen from Rob Pike
/* regex : function to help save some typing */
int regex(char *text, char *regexp);
/* regex_match : search for regexp anywhere in text */
int regex_match (char *regexp, char *text);
/* regex_matchhere: search for regexp at beginning of text */
int regex_matchhere(char *regexp, char *text);
/* regex_matchstar: search for c*regexp at beginning of text */
int regex_matchstar(int c, char *regexp, char *text);

/* strsplit : string split */
size_t strsplit(char **arr, size_t len, char *buf, char sep);
/* strlen_char : returns strlen, but as if 'c' were the string terminator */
size_t strlen_char(char *s, char c);
/* bstrtok : Brian's (Better) strtok */
char *bstrtok(char **str, char *delim);
/* strnullcmp : compare strings, sorting null values as "first" */
int strnullcmp(const void *a, const void *b);
/* strornull : returns the string representation of NULL if the string is */
char *strornull(char *s);

/* ltrim : removes whitespace on the "left" (start) of the string */
char *ltrim(char *s);

/* rtrim : removes whitespace on the "right" (end) of the string */
char *rtrim(char *s);

/* mklower : makes the string lower cased */
int mklower(char *s);

/* mkupper : makes the string lower cased */
int mkupper(char *s);

/* streq : return true if the strings are equal */
int streq(char *s, char *t);

/* is_num : returns true if the string is numeric */
int is_num(char *s);

/* c_atoi : stdlib's atoi, but returns 0 if the pointer is NULL */
s32 c_atoi(char *s);

/* c_cmp_strstr : common comparator for two strings */
int c_cmp_strstr(const void *a, const void *b);

/* strdup_null : duplicates the string if non-null, returns NULL otherwise */
char *strdup_null(char *s);

/* c_fprintf : common printf logging routine, with some extra pizzaz */
int c_fprintf(char *file, int line, int level, FILE *fp, char *fmt, ...);

enum {
	  LOG_NON
	, LOG_ERR
	, LOG_WRN
	, LOG_MSG
	, LOG_VER
	, LOG_TOTAL
};

#define MSG(fmt, ...) (c_fprintf(__FILE__, __LINE__, LOG_MSG, stderr, fmt, ##__VA_ARGS__)) // basic log message
#define WRN(fmt, ...) (c_fprintf(__FILE__, __LINE__, LOG_WRN, stderr, fmt, ##__VA_ARGS__)) // warning message
#define ERR(fmt, ...) (c_fprintf(__FILE__, __LINE__, LOG_ERR, stderr, fmt, ##__VA_ARGS__)) // error message
#define VER(fmt, ...) (c_fprintf(__FILE__, __LINE__, LOG_VER, stderr, fmt, ##__VA_ARGS__)) // verbose message

#if defined(COMMON_IMPLEMENTATION)

/* c_resize : resizes the ptr should length and capacity be the same */
void c_resize(void *ptr, size_t *len, size_t *cap, size_t bytes)
{
	void **p;

	if (*len == *cap) {
		p = (void **)ptr;

		if (*cap) {
			if (BUFLARGE < *cap) {
				*cap += BUFLARGE;
			} else {
				*cap *= 2;
			}
		} else {
			*cap = BUFSMALL;
		}

		*p = realloc(*p, bytes * *cap);

		// set the rest of the elements to zero
		memset(((u8 *)*p) + *len * bytes, 0, (*cap - *len) * bytes);
	}
}

/* ltrim : removes whitespace on the "left" (start) of the string */
char *ltrim(char *s)
{
	while (isspace(*s))
		s++;

	return s;
}

/* rtrim : removes whitespace on the "right" (end) of the string */
char *rtrim(char *s)
{
	char *e;

	for (e = s + strlen(s) - 1; isspace(*e); e--)
		*e = 0;

	return s;
}

/* strnullcmp : compare strings, sorting null values as "first" */
int strnullcmp(const void *a, const void *b)
{
	char **stra, **strb;

	stra = (char **)a;
	strb = (char **)b;

	if (*stra == NULL && *strb == NULL) {
		return 0;
	}

	if (*stra == NULL && *strb != NULL) {
		return 1;
	}

	if (*stra != NULL && *strb == NULL) {
		return -1;
	}

	return strcmp(*stra, *strb);
}

/* strornull : returns the string representation of NULL if the string is */
char *strornull(char *s)
{
	// NOTE also returns NULL on empty string
	return (s && strlen(s) != 0) ? s : "NULL";
}

/* strcmpv : a qsort wrapper for strcmp */
int strcmpv(const void *a, const void *b)
{
	// remember, we get char **s
	return (int)strcmp(*(char **)a, *(char **)b);
}

/* regex : function to help save some typing */
int regex(char *text, char *regexp)
{
	return regex_match(regexp, text);
}

/* regex_match : search for regexp anywhere in text */
int regex_match(char *regexp, char *text)
{
	if (regexp[0] == '^')
		return regex_matchhere(regexp+1, text);
	do {    /* must look even if string is empty */
		if (regex_matchhere(regexp, text))
			return 1;
	} while (*text++ != '\0');
	return 0;
}

/* regex_matchhere: search for regexp at beginning of text */
int regex_matchhere(char *regexp, char *text)
{
	if (regexp[0] == '\0')
		return 1;
	if (regexp[1] == '*')
		return regex_matchstar(regexp[0], regexp+2, text);
	if (regexp[0] == '$' && regexp[1] == '\0')
		return *text == '\0';
	if (*text!='\0' && (regexp[0]=='.' || regexp[0]==*text))
		return regex_matchhere(regexp+1, text+1);
	return 0;
}

/* regex_matchstar: search for c*regexp at beginning of text */
int regex_matchstar(int c, char *regexp, char *text)
{
	do {    /* a * matches zero or more instances */
		if (regex_matchhere(regexp, text))
			return 1;
	} while (*text != '\0' && (*text++ == c || c == '.'));
	return 0;
}

/* strsplit : string split */
size_t strsplit(char **arr, size_t len, char *buf, char sep)
{
	size_t num, i;
	char *s;


	// first, we count how many instances of sep there are
	// then we split it into that many pieces

	for (num = 0, s = buf; *s; s++) {
		if (*s == sep) {
			num++;
		}
	}

	if (arr) { // only if we have a valid array, do we actually split the str
		memset(arr, 0, len * sizeof(*arr));
		for (i = 0, s = buf; i < len; i++) {
			if (0 < strlen_char(s, sep)) {
				arr[i] = s;

				s = strchr(s, sep);
				if (s == NULL)
					break;
			} else {
				arr[i] = s + strlen(s); // empty string, point to NULL byte
			}

			*s = 0;
			s++;
		}
	}

	return num;
}

/* strlen_char : returns strlen, but as if 'c' were the string terminator */
size_t strlen_char(char *s, char c)
{
	size_t i;

	// NOTE this will stop at NULLs

	for (i = 0; s[i]; i++)
		if (s[i] == c)
			break;

	return i;
}

/* bstrtok : Brian's (Better) strtok */
char *bstrtok(char **str, char *delim)
{
	/*
	 * strtok is super gross. let's make it better (and worse)
	 *
	 * few things to note
	 *
	 * To use this properly, pass a pointer to your buffer as well as a string
	 * you'd like to delimit your text by. When you've done that, you
	 * effectively have two return values. The NULL terminated C string
	 * explicitly returned from the function, and the **str argument will point
	 * to the next section of the buffer to parse. If str is ever NULL after a
	 * call to this, there is no new delimiting string, and you've reached the
	 * end
	 */

	char *ret, *work;

	ret = *str; /* setup our clean return value */
	work = strstr(*str, delim); /* now do the heavy lifting */

	if (work) {
		/* we ASSUME that str was originally NULL terminated */
		memset(work, 0, strlen(delim));
		work += strlen(delim);
	}

	*str = work; /* setting this will make *str NULL if a delimiter wasn't found */

	return ret;
}

/* streq : return true if the strings are equal */
int streq(char *s, char *t)
{
	return strcmp(s, t) == 0;
}

/* is_num : returns true if the string is numeric */
int is_num(char *s)
{
	while (s && *s) {
		if (!isdigit(*s))
			return 0;
	}

	return 1;
}

/* c_atoi : stdlib's atoi, but returns 0 if the pointer is NULL */
s32 c_atoi(char *s)
{
	return s == NULL ? 0 : atoi(s);
}

/* mklower : makes the string lower cased */
int mklower(char *s)
{
	char *t;
	for (t = s; *t; t++) {
		*t = tolower(*t);
	}
	return 0;
}

/* mkupper : makes the string lower cased */
int mkupper(char *s)
{
	char *t;
	for (t = s; *t; t++) {
		*t = toupper(*t);
	}
	return 0;
}

/* c_cmp_strstr : common comparator for two strings */
int c_cmp_strstr(const void *a, const void *b)
{
	char *stra, *strb;

	// generic strcmp, with NULL checks

	stra = *(char **)a;
	strb = *(char **)b;

	if (stra == NULL && strb != NULL)
		return 1;

	if (stra != NULL && strb == NULL)
		return -1;

	if (stra == NULL && strb == NULL)
		return 0;

	return strcmp(stra, strb);
}

/* strdup_null : duplicates the string if non-null, returns NULL otherwise */
char *strdup_null(char *s)
{
	return s ? strdup(s) : NULL;
}

/* c_fprintf : common printf logging routine, with some extra pizzaz */
int c_fprintf(char *file, int line, int level, FILE *fp, char *fmt, ...)
{
	va_list args;
	int rc;
	char bigbuf[BUFLARGE];

	char *logstr[] = {
		  "   " // LOG_NON = 0 (this is a special case, to make the array work)
		, "ERR" // LOG_ERR = 1
		, "WRN" // LOG_WRN = 2
		, "MSG" // LOG_MSG = 3
		, "VER" // LOG_VER = 4
	};

	// NOTE (brian)
	//
	// This writes a formatted message:
	//   __FILE__:__LINE__ LEVELSTR MESSAGE

	assert(LOG_TOTAL == ARRSIZE(logstr));

	if (!(LOG_NON <= level && level <= LOG_VER)) {
		WRN("Invalid LOG Level from %s:%d\n", file, line);
		level = LOG_NON;
	}

	rc = 0;

	if (strlen(fmt) == 0)
		return rc;

	memset(bigbuf, 0, sizeof(bigbuf));

	va_start(args, fmt); /* get the arguments from the stack */

	rc += fprintf(fp, "%s:%04d %s ", file, line, logstr[level]);
	rc += vfprintf(fp, fmt, args);

	va_end(args); /* cleanup stack arguments */

	if (fmt[strlen(fmt) - 1] != '\n' && fmt[strlen(fmt) - 1] != '\r') {
		rc += fprintf(fp, "\n");
	}

	return rc;
}

#endif // COMMON_IMPLEMENTATION

#endif // COMMON_H

