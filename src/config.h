/*
 * Brian Chrzanowski
 * Sun Feb 03, 2019 02:42 
 * 
 * MoLT Static Configuration Header
 *
 * NOTE:
 * Some units in this config file are SCALED! Namely:
 *
 * - MOLT_STEPIN*
 * - MOLT_DOMAIN*
 * and by association
 * - MOLT_TOTAL*
 *
 * Having a standardized integer "unit" makes size and indexing calculations
 * effortless, and getting the real value is just a single multiplication away.
 */

#ifndef MOLT_CONFIG
#define MOLT_CONFIG

#define CEILING(X, Y)   (((X) + (Y) - 1) / (Y))

/* mathematical constants first */
#define PI 3.14159265

/* freespace parameters */
#define MOLT_LIGHTSPEED      299792458
#define MOLT_HENRYPERMETER   4 * PI * 1E-7
#define MOLT_FARADSPERMETER \
	1 / (((long)MOLT_LIGHTSPEED * (long)MOLT_LIGHTSPEED) * MOLT_HENRYPERMETER)

/* tissue parameters */
#define MOLT_STATICPERM      80
#define MOLT_INFPERM         4
#define MOLT_TAU             1
#define MOLT_DISTRIBTAIL     0.7
#define MOLT_DISTRIBASYM     0.25
#define MOLT_TISSUESPEED     (MOLT_LIGHTSPEED / (sqrt(MOLT_INFPERM)))

/* 
 * mesh parameters 
 */

/* 
 * Values are normalized to integers (u64s) for computation. Their values
 * will, when needed, be scaled according to the MOLT_INTSCALE value.
 */

#define MOLT_INTSCALE      0.01

/*
 * There are 3 pieces of information to know about the dimensionality of the
 * problem.
 *
 * * start
 * * stop
 * * length
 *
 * That is, to define a dimension, we need to know what (remember the normalized
 * units from up above) integer it starts at, stops at, and how big of a step we
 * can take within the simulation grid.
 */

#define MOLT_T_START         0
#define MOLT_T_STOP          100
#define MOLT_T_STEP          1
#define MOLT_T_POINTS        ((MOLT_T_STOP - MOLT_T_START) / MOLT_T_STEP)
#define MOLT_T_POINTS_INC    (MOLT_T_POINTS + 1)

#define MOLT_X_START         0
#define MOLT_X_STOP          100
#define MOLT_X_STEP          1
#define MOLT_X_POINTS        ((MOLT_X_STOP - MOLT_X_START) / MOLT_X_STEP)
#define MOLT_X_POINTS_INC    (MOLT_X_POINTS + 1)

#define MOLT_Y_START         0
#define MOLT_Y_STOP          100
#define MOLT_Y_STEP          1
#define MOLT_Y_POINTS        ((MOLT_Y_STOP - MOLT_Y_START) / MOLT_Y_STEP)
#define MOLT_Y_POINTS_INC    (MOLT_Y_POINTS + 1)

#define MOLT_Z_START         0
#define MOLT_Z_STOP          100
#define MOLT_Z_STEP          1
#define MOLT_Z_POINTS        ((MOLT_Z_STOP - MOLT_Z_START) / MOLT_Z_STEP)
#define MOLT_Z_POINTS_INC    (MOLT_Z_POINTS + 1)

#define MOLT_CFL \
	(MOLT_TISSUESPEED * (MOLT_T_STEP * MOLT_INTSCALE) *\
	 sqrt(\
		 1 / pow((MOLT_X_STEP * MOLT_INTSCALE), 2) + \
		 1 / pow((MOLT_Y_STEP * MOLT_INTSCALE), 2) + \
		 1 / pow((MOLT_Z_STEP * MOLT_INTSCALE), 2)) * 1e-10)

#define MOLT_SPACEACC     6
#define MOLT_TIMEACC      3

#if     MOLT_TIMEACC == 3
#define MOLT_BETA 1.23429074525305
#elif   MOLT_TIMEACC == 2
#define MOLT_BETA 1.48392756860545
#elif   MOLT_TIMEACC == 1
#define MOLT_BETA 2
#endif

#define MOLT_ALPHA MOLT_BETA / (MOLT_TISSUESPEED * MOLT_T_STEP * MOLT_INTSCALE)

#endif
