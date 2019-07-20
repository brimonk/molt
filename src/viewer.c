/*
 * Brian Chrzanowski
 * Fri Jul 19, 2019 22:48
 *
 * MOLT Viewer Implementation
 *
 * This viewer, for ease of transport, requires SDL2 to operate.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <SDL2/SDL.h> // TODO (brian) check include path for other operating systems
#include <GL/gl.h>

// get some other viewer flags out of the way
#define VIEWER_SDL_WINFLGS SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE

#include "molt.h"

/* viewer_run : runs the molt graphical 3d simulation viewer */
s32 viewer_run(void *hunk, u64 hunksize, s32 fd, struct molt_cfg_t *cfg)
{
	SDL_Window *window;
	SDL_GLContext glcontext;
	s32 rc, run;

	run = 1, rc = 0;

	window = SDL_CreateWindow("MOLT Viewer",0, 0, 640, 480, VIEWER_SDL_WINFLGS);

	// TODO (brian) check for window creation errors

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	glcontext = SDL_GL_CreateContext(window);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(window);

	SDL_Delay(2 * 1000);

#if 0
	while (run) {
	}
#endif

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);

	return rc;
}

