/*
 * Brian Chrzanowski
 * Sun Feb 03, 2019 02:42 
 * 
 * MoLT Static Configuration Header
 */

#ifndef MOLT_CONFIG
#define MOLT_CONFIG

/* mathematical constants first */
#define PI 3.14159265

/* freespace parameters */
#define MOLT_LIGHTSPEED      299792458
#define MOLT_HENRYPERMETER   4 * PI * 1E-7
#define MOLT_FARADSPERMETER  \
	1 / ((MOLT_LIGHTSPEED * MOLT_LIGHTSPEED) * MOLT_HENRYPERMETER)

/* tissue parameters */
#define MOLT_STATICPERM      80
#define MOLT_INFPERM         4
#define MOLT_TAU             1
#define MOLT_DISTRIBTAIL     0.7
#define MOLT_DISTRIBASYM     0.25
#define MOLT_TISSUESPEED     (MOLT_LIGHTSPEED / sqrt(MOLT_INFPERM))

/* mesh parameters */
#define MOLT_STEPINSEC       1E-2
#define MOLT_STEPINX         2E-2 /* in meters */
#define MOLT_STEPINY         2E-2 /* in meters */
#define MOLT_STEPINZ         2E-2 /* in meters */

#define MOLT_CFL \
	(MOLT_TISSUESPEED * MOLT_STEPINSEC * \
	 sqrt(1/pow(MOLT_STEPINX,2) + 1/pow(MOLT_STEPINY,2) + 1/pow(MOLT_STEPINZ)) \
	 * 1E-10)


/* dimension parameters */
#define MOLT_SIMTIME         0.1 /* in seconds (steps are in picoseconds) */
#define MOLT_DOMAINWIDTH     2 /* in meters */
#define MOLT_DOMAINDEPTH     2 /* in meters */
#define MOLT_DOMAINHEIGHT    2 /* in meters */

#define MOLT_TOTALSTEPS      ((long)(ceil(MOLT_SIMTIME/MOLT_STEPINSEC)))
#define MOLT_TOTALWIDTH      ((long)(ceil(MOLT_DOMAINWIDTH/MOLT_STEPINX)))
#define MOLT_TOTALDEEP       ((long)(ceil(MOLT_DOMAINDEPTH/MOLT_STEPINY)))
#define MOLT_TOTALHEIGHT     ((long)(ceil(MOLT_DOMAINHEIGHT/MOLT_STEPINZ)))

#define MOLT_SPACEACC        6
#define MOLT_TIMEACC         3

#if     MOLT_TIMEACC == 3
#define MOLT_BETA 1.23429074525305
#elif   MOLT_TIMEACC == 2
#define MOLT_BETA 1.48392756860545
#elif   MOLT_TIMEACC == 1
#define MOLT_BETA 2
#endif

#define MOLT_ALPHA MOLT_BETA / (MOLT_TISSUESPEED * MOLT_STEPINSEC)

#endif
