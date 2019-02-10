/*
 * this is the default config header for tissue simulation settings
 */

#ifndef MOLT_CONFIG
#define MOLT_CONFIG

#if     MOLT_P == 1
#define MOLT_BETA 2
#elif   MOLT_P == 2
#define MOLT_BETA 1.48392756860545
#elif   MOLT_P == 3
#define MOLT_BETA 1.23429074525306
#endif

#endif
