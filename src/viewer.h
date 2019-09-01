#ifndef VIEWER_H
#define VIEWER_H

#include <SDL2/SDL.h>

#include "common.h"
#include "molt.h"

/* viewer_run : runs the molt graphical 3d simulation viewer */
s32 viewer_run(void *hunk, u64 hunksize, s32 fd, struct molt_cfg_t *cfg);

#endif // VIEWER_H

