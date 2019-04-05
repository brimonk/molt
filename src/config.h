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
#define MOLT_FARADSPERMETER  \
	1 / (((long)MOLT_LIGHTSPEED * (long)MOLT_LIGHTSPEED) * MOLT_HENRYPERMETER)

/* tissue parameters */
#define MOLT_STATICPERM      80
#define MOLT_INFPERM         4
#define MOLT_TAU             1
#define MOLT_DISTRIBTAIL     0.7
#define MOLT_DISTRIBASYM     0.25
#define MOLT_TISSUESPEED     (MOLT_LIGHTSPEED / (sqrt(MOLT_INFPERM)))

/* mesh parameters */
#define MOLT_INTSCALE      0.01
#define MOLT_STEPINT         1 /* in units */
#define MOLT_STEPINX         2 /* in units */
#define MOLT_STEPINY         2 /* in units */
#define MOLT_STEPINZ         2 /* in units */

#define MOLT_CFL \
	(MOLT_TISSUESPEED * MOLT_STEPINT * \
	 sqrt(1/pow(MOLT_STEPINX,2) + 1/pow(MOLT_STEPINY,2) \
		 + 1/pow(MOLT_STEPINZ,2)) * 1E-10)

/* dimension parameters */
#define MOLT_SIMTIME      10 /* in units */
#define MOLT_DOMAINWIDTH  200 /* in units */
#define MOLT_DOMAINDEPTH  200 /* in units */
#define MOLT_DOMAINHEIGHT 200 /* in units */

#define MOLT_TOTALSTEPS   (MOLT_SIMTIME / MOLT_STEPINT) + 1
#define MOLT_TOTALWIDTH   (MOLT_DOMAINWIDTH / MOLT_STEPINX)
#define MOLT_TOTALDEEP    (MOLT_DOMAINDEPTH / MOLT_STEPINY)
#define MOLT_TOTALHEIGHT  (MOLT_DOMAINHEIGHT / MOLT_STEPINZ)

#if 0
#define MOLT_XPOINTS      ((long)((0 + MOLT_TOTALWIDTH) / MOLT_DOMAINWIDTH))
#define MOLT_YPOINTS      ((long)((0 + MOLT_TOTALDEEP) / MOLT_DOMAINDEPTH))
#define MOLT_ZPOINTS      ((long)((0 + MOLT_TOTALHEIGHT) / MOLT_DOMAINHEIGHT))
#else
#define MOLT_XPOINTS      (MOLT_TOTALWIDTH + 1)
#define MOLT_YPOINTS      (MOLT_TOTALDEEP + 1)
#define MOLT_ZPOINTS      (MOLT_TOTALHEIGHT + 1)
#endif

#define MOLT_SPACEACC     6
#define MOLT_TIMEACC      3

#if     MOLT_TIMEACC == 3
#define MOLT_BETA 1.23429074525305
#elif   MOLT_TIMEACC == 2
#define MOLT_BETA 1.48392756860545
#elif   MOLT_TIMEACC == 1
#define MOLT_BETA 2
#endif

#define MOLT_ALPHA MOLT_BETA / \
	(MOLT_TISSUESPEED * MOLT_STEPINT * MOLT_INTSCALE)

#endif
