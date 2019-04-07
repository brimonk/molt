#ifndef MOLT
#define MOLT

#include "common.h"

/*
 * Brian Chrzanowski
 * Sat Apr 06, 2019 22:21
 *
 * CPU Implementation of MOLT's Timestepping Features
 */

/* molt_firststep : specific routines required for the "first" timestep */
void molt_firststep(lcfg_t *cfg, lrun_t *run, lnu_t *nu, lvweight_t *vw,
					lwweight_t *ww, lmesh_t *mesh);

/* molt_step : regular timestepping routine */
void molt_step(lcfg_t *cfg, lrun_t *run, lnu_t *nu, lvweight_t *vw,
			   lwweight_t *ww, lmesh_t *mesh);

#endif
