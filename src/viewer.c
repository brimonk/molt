/*
 * Brian Chrzanowski
 * Fri Jul 19, 2019 22:48
 *
 * MOLT Viewer Implementation
 *
 * This viewer, for ease of transport, requires SDL2 to operate.
 *
 * TODO
 * 1. Error Handling
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

#define ERRLOG(a,b)\
	fprintf(stderr, "%s:%d %s %s\n", __FILE__, __LINE__, (a), (b))

#include "common.h"
#include "molt.h"

struct simstate_t {
	// input goes first
	ivec2_t mousepos;
	ivec3_t mousebutton; // 0-left, 1-right, 2-middle

	ivec4_t wasd;  // w-0, a-1, s-2, d-3
	ivec2_t r_esc; // r-0, esc-1

	// viewer state last
	fvec3_t userpos, usercam;
	int run;
};

void viewer_getinputs(struct simstate_t *state);

/* viewer_run : runs the molt graphical 3d simulation viewer */
s32 viewer_run(void *hunk, u64 hunksize, s32 fd, struct molt_cfg_t *cfg)
{
	SDL_Window *window;
	SDL_GLContext glcontext;
	struct simstate_t state;
	s32 rc;

	rc = 0, state.run = 1;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		ERRLOG("Couldn't Init SDL", SDL_GetError());
		return 1;
	}

	window = SDL_CreateWindow("MOLT Viewer",0, 0, 640, 480, VIEWER_SDL_WINFLGS);

	// TODO (brian) check for window creation errors

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	glcontext = SDL_GL_CreateContext(window);

	glClearColor(1, 0, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(window);

	SDL_Delay(2 * 1000);

	while (state.run) {
		viewer_getinputs(&state);
		state.run = !state.r_esc[1];
	}

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return rc;
}

/* viewer_getinputs : retrieve the SDL inputs when we're ready to shine */
void viewer_getinputs(struct simstate_t *state)
{
	u32 mouse_mask;
	const u8 *keystate;
	int numkeys;

	SDL_PumpEvents(); // force the input polling

	// retrieve the mouse info
	mouse_mask = SDL_GetMouseState(&state->mousepos[0], &state->mousepos[1]);
	if (mouse_mask & SDL_BUTTON(SDL_BUTTON_LEFT))
		state->mousebutton[0] = 1;
	else
		state->mousebutton[0] = 0;

	if (mouse_mask & SDL_BUTTON(SDL_BUTTON_RIGHT))
		state->mousebutton[1] = 1;
	else
		state->mousebutton[1] = 0;

	if (mouse_mask & SDL_BUTTON(SDL_BUTTON_MIDDLE))
		state->mousebutton[2] = 1;
	else
		state->mousebutton[2] = 0;

	keystate = SDL_GetKeyboardState(&numkeys);

	state->wasd[0] = keystate[SDL_SCANCODE_W];
	state->wasd[1] = keystate[SDL_SCANCODE_A];
	state->wasd[2] = keystate[SDL_SCANCODE_S];
	state->wasd[3] = keystate[SDL_SCANCODE_D];

	state->r_esc[0] = keystate[SDL_SCANCODE_R];
	state->r_esc[1] = keystate[SDL_SCANCODE_ESCAPE];
}

