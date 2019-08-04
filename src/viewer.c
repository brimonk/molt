/*
 * Brian Chrzanowski
 * Fri Jul 19, 2019 22:48
 *
 * MOLT Viewer Implementation
 *
 * This viewer, for ease of transport, requires SDL2 to operate.
 *
 * TODO (brian)
 * 1. Debug Info In Lower Left of Screen
 *
 *    Info to Display (through text)
 *
 *    * FPS, FrameTiming
 *    * (X,Y,Z,T) Player Position
 *    * (X,Y,Z,T,Magnitude) Selected Gridpoint
 *    * Data Displayed
 *
 * 2. Refactor Run Loop
 *
 * 3. Implement a Loose Particle System, Like the one found here:
 *    http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/
 *
 * 4. Texture-Based 3d Volume Rendering
 *
 *    Should the particle system become too slow, I could probably use a
 *    texture-based 3d volume rendering. Reason being that GPU fill-rate is
 *    the main thing that limits engineering data with lots of data.
 *
 *    https://developer.download.nvidia.com/books/HTML/gpugems/gpugems_ch39.html
 *
 *    While complex and potentially very slow, this is effectively the real way
 *    to do this. From that Nvidia book chapter, effectively you create textures
 *    at various slices of the volume perpendicular to the 3d camera's line of
 *    focus (?). The considerations, however are performance concerns a
 *    programmer needs to be aware of:
 *
 *    * Procedural Rendering Might Be Neccessary
 *    * Rasterization Bottlenecks
 *    * Limitations of Fragment Programs (Shaders)
 *    * Texture Memory Limitations
 *
 * 5. Change how we get input, convert it to use SDL Events
 *
 * TODO (brian, Eventually)
 * 1. SDL / OpenGL Error Handling
 * 2. Cross Platform Include Headers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <float.h>

#include <SDL2/SDL.h> // TODO (brian) check include path for other operating systems

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "common.h"
#include "molt.h"
#include "io.h"
#include "handmademath.h"

// get some other viewer flags out of the way
#define VIEWER_SDL_INITFLAGS \
	SDL_INIT_VIDEO | SDL_INIT_TIMER
#define VIEWER_SDL_WINFLAGS SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE

#define ERRLOG(a,b)\
	fprintf(stderr, "%s:%d %s %s\n", __FILE__, __LINE__, (a), (b))

#define VIEWER_MAXVEL 1.0f

#define VERTEX_SHADER_PATH   "src/vertex.glsl"
#define FRAGMENT_SHADER_PATH "src/fragment.glsl"

#define TARGET_FPS 90
#define TARGET_FRAMETIME 1000 / TARGET_FPS
#define WIDTH  1024
#define HEIGHT  768

struct simstate_t {
	// input goes first
	ivec2_t mousepos;
	ivec3_t mousebutton; // 0-left, 1-right, 2-middle

	ivec4_t wasd;  // w-0, a-1, s-2, d-3
	ivec2_t r_esc; // r-0, esc-1
	ivec2_t pgup_pgdown; // pgup-0, pgdown-1, controls timestepping

	// 0 - time we last drew a frame
	// 1 - time at the start of the frame
	// 2 - time at the end of the frame
	uivec3_t frametime;

	// viewer state last
	fvec3_t userpos, usercam, uservel;
	int run;

	f64 lbound, hbound; // low and high range of values on ALL the meshes
};

void viewer_bounds(f64 *p, u64 len, f64 *low, f64 *high);

void viewer_getinputs(struct simstate_t *state);
void viewer_handleinput(struct simstate_t *state);
u32 viewer_mkshader(char *vertex, char *fragment);

/* viewer_run : runs the molt graphical 3d simulation viewer */
s32 viewer_run(void *hunk, u64 hunksize, s32 fd, struct molt_cfg_t *cfg)
{
	SDL_Window *window;
	SDL_GLContext glcontext;
	struct simstate_t state;
	u32 program_shader;
	struct lump_umesh_t *mesh;

	u32 model_location, view_location, persp_loc;
	s32 rc, i, timesteps;
	s64 meshlen;

	ivec3_t meshdim;

	u32 vbo, vao;

	float vertices[] = {
		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,

		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,

		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,

		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f,

		-0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,
	};

	hmm_mat4 model, view, proj, orig;

	rc = 0, state.run = 1;
	mesh = io_lumpgetbase(hunk, MOLTLUMP_UMESH);
	molt_cfg_parampull_xyz(cfg, meshdim, MOLT_PARAM_POINTS);
	meshlen = meshdim[0] * meshdim[1] * meshdim[2];
	timesteps = cfg->t_params[MOLT_PARAM_POINTS];

	for (i = 0; i < timesteps; i++) {
		viewer_bounds(mesh[i].data, meshlen, &state.lbound, &state.hbound);
	}

	printf("low : %lf, high : %lf\n", state.lbound, state.hbound);

	// determine what the high and low values in ALL the meshes are
	// this is for coloring the quads later in the program

	if (SDL_Init(VIEWER_SDL_INITFLAGS) != 0) {
		ERRLOG("Couldn't Init SDL", SDL_GetError());
		return -1;
	}

	window = SDL_CreateWindow("MOLT Viewer",0, 0, WIDTH, HEIGHT, VIEWER_SDL_WINFLAGS);
	if (window == NULL) {
		ERRLOG("Couldn't Create Window", SDL_GetError());
		return -1;
	}

	// TODO (brian) check for window creation errors

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	glcontext = SDL_GL_CreateContext(window);

	// set no vsync (??)
	if (SDL_GL_SetSwapInterval(0) != 0) {
		ERRLOG("Couldn't set the swap interval", SDL_GetError());
	}

	glEnable(GL_DEPTH_TEST);

	program_shader = viewer_mkshader(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	glUseProgram(program_shader);

	f32 r_val;
	r_val = 0;

	// setup original cube state
	orig = HMM_Mat4d(1.0f);
	orig = HMM_MultiplyMat4(orig, HMM_Rotate(90, HMM_Vec3(0.0f, 1.0f, 1.0f)));

	while (state.run) {
		state.frametime[1] = SDL_GetTicks();

		viewer_getinputs(&state);
		viewer_handleinput(&state);

		state.run = !state.r_esc[1];

		if (state.r_esc[0])
			r_val += 1.0f;

		if (r_val > 360.0f)
			r_val = 0.0f;

		// render
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(program_shader);

		// create our transformation
#if 1
		model = HMM_Rotate(SDL_GetTicks(), HMM_Vec3(1.0f, 0.0f, 0.0f));
		model = HMM_MultiplyMat4(model, orig);
#else
		model = HMM_Mat4d(1.0f);
#endif
		view = HMM_Translate(HMM_Vec3(0.0f, 0.0f, -3.0f));
		proj = HMM_Perspective(45.0f, WIDTH / HEIGHT, 0.1f, 100.0f);

		model_location = glGetUniformLocation(program_shader, "model");
		view_location = glGetUniformLocation(program_shader, "view");
		persp_loc = glGetUniformLocation(program_shader, "proj");

		// send them to the shader
		glUniformMatrix4fv(model_location, 1, GL_FALSE, (f32 *)&model);
		glUniformMatrix4fv(view_location, 1, GL_FALSE, (f32 *)&view);
		glUniformMatrix4fv(persp_loc, 1, GL_FALSE, (f32 *)&proj);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		SDL_GL_SwapWindow(window);

		state.frametime[2] = SDL_GetTicks();

		// delay if diff between curr time and last drawn time is more
		// than our target frame time
		// NOTE (brian)
		// time to draw frame, frametime[2] - frametime[1]
		// rendering the scene actually needs to happen right in here
		if (state.frametime[0] + TARGET_FRAMETIME < state.frametime[2]) {
			SDL_Delay(state.frametime[2] - state.frametime[0]);
			state.frametime[0] = SDL_GetTicks();
		}

		// printf("frame time : %d ms\n", state.frametime[2] - state.frametime[1]);
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);

	// clean up from our gl shenanigans
	glDeleteProgram(program_shader);

	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return rc;
}

/* viewer_getinputs : retrieve the SDL inputs when we're ready to shine */
void viewer_getinputs(struct simstate_t *state)
{
	// DOC Scancodes - https://wiki.libsdl.org/SDL_Scancode
	// DOC Keyboard modifiers - https://wiki.libsdl.org/SDL_Keymod
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

	state->pgup_pgdown[0] = keystate[SDL_SCANCODE_PAGEUP];
	state->pgup_pgdown[1] = keystate[SDL_SCANCODE_PAGEDOWN];
}

/* viewer_handleinput : updates the gamestate from the */
void viewer_handleinput(struct simstate_t *state)
{
	// games are hard
}

u32 viewer_mkshader(char *vertex_file, char *fragment_file)
{
	s32 success;
	u32 vshader_id, fshader_id, program_id;
	s32 sizei;
	char err_buf[BUFLARGE];
	char *vertex, *fragment;

	// load shaders from disk
	vertex = io_readfile(vertex_file);
	fragment = io_readfile(fragment_file);

	// compile the desired vertex shader
	vshader_id = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader_id, 1, (void *)&vertex, NULL);
	glCompileShader(vshader_id);

	// vertex shader error checking
	glGetShaderiv(vshader_id, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vshader_id, sizeof err_buf, NULL, err_buf);
		ERRLOG("vertex shader error:\n", err_buf);
		assert(0);
	}

	// compile the desired fragment shader
	fshader_id = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader_id, 1, (void *)&fragment, NULL);
	glCompileShader(fshader_id);

	// fragment shader error checking
	glGetShaderiv(fshader_id, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fshader_id, sizeof err_buf, NULL, err_buf);
		ERRLOG("fragment shader error:\n", err_buf);
		assert(0);
	}

	// attach and link the program
	program_id = glCreateProgram();
	glAttachShader(program_id, vshader_id);
	glAttachShader(program_id, fshader_id);
	glLinkProgram(program_id);

	// now do some error checking
	// glValidateProgram(program_id);
	glGetProgramiv(program_id, GL_LINK_STATUS, &success);

	if (!success) {
		glGetProgramInfoLog(program_id, sizeof err_buf, &sizei, err_buf);
		ERRLOG("shader link error:\n", err_buf);
		assert(0);
	}

	// clean up from our shader shenanigans
	glDeleteShader(vshader_id);
	glDeleteShader(fshader_id);

	free(vertex);
	free(fragment);

	return program_id;
}

/* viewer_bounds : gets the lowest and highest values of all values in the mesh */
void viewer_bounds(f64 *p, u64 len, f64 *low, f64 *high)
{
	// WARN (Brian)
	// this probably doesn't play nice if the algorithm isn't stable, meaning
	// when you accidentally get -nans in your doubles, you're going to have
	// issues (probably)

	f64 llow, lhigh;
	u64 i;

	llow  = DBL_MAX;
	lhigh = DBL_MIN;

	for (i = 0; i < len; i++) {
		if (p[i] < llow) {
			llow = p[i];
		}
		if (p[i] > lhigh) {
			lhigh = p[i];
		}
	}

	// we only update them if the values REALLY ARE lower
	if (*low > llow)
		*low  = llow;
	if (*high < lhigh)
		*high = lhigh;
}

