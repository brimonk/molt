/*
 * Brian Chrzanowski
 * Fri Jul 19, 2019 22:48
 *
 * MOLT Viewer Implementation
 *
 * This viewer, for ease of transport, requires SDL2 to operate.
 *
 * TODO General (brian)
 * 1. Debug Info In Lower Left of Screen
 *    Info to Display (through text)
 *    * FPS, FrameTiming
 *    * (X,Y,Z,T) Player Position
 *    * (X,Y,Z,T,Magnitude) Selected Gridpoint
 *    * Data Displayed
 *
 * 2. Implement a Loose Particle System, Like the one found here:
 *    http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/
 *
 * 3. Texture-Based 3d Volume Rendering
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
 * TODO MoLT Volume (brian)
 * 1. Data Structure for prev, curr, next volumes
 * 2. Setup Stride info based on 
 *
 * TODO MoLT Shader (brian)
 * 1. Uniform with LowFloatingBound, HighFloatingBound
 *
 * TODO Oculus SDK
 * 1. Get XYZ Player Movement
 * 2. Get XYZ Controller Movement
 *
 * TODO Simulation Rotation
 * 1. Read from XBOX Controller
 * 2. Triggers Pull/Push the Mesh Apart/Together
 *
 * TODO Cleanup (brian)
 * 1. SDL / OpenGL Error Handling
 * 2. Cross Platform Include Headers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
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
	SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER
#define VIEWER_SDL_WINFLAGS SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE

#define ERRLOG(a,b) fprintf(stderr,"%s:%d %s %s\n",__FILE__,__LINE__,(a),(b))

#define VERTEX_SHADER_PATH   "src/vertex.glsl"
#define FRAGMENT_SHADER_PATH "src/fragment.glsl"

#define VIEWER_MAXVEL 1.0f
#define VIEWER_FOV 45.0f

// #define TARGET_FPS 144
#define TARGET_FPS 144
#define TARGET_FRAMETIME 1000.0 / TARGET_FPS
#define WIDTH  1024
#define HEIGHT  768
#define MOUSE_SENSITIVITY 0.5f

enum {
	AXIS_CAMERA,
	AXIS_X,
	AXIS_Y,
	AXIS_Z
} SIM_CURRAXIS;

struct simstate_t {
	// input goes first
	ivec2_t mousepos;
	ivec3_t mousebutton; // 0-left, 1-right, 2-middle

	s8 key_w, key_a, key_s, key_d;
	s8 mouse_l, mouse_r, mouse_m;
	s8 key_pgup, key_pgdown;
	s8 key_esc;

	// curr - current time in ms
	// last - last time we drew a frame
	// delta- difference between the two

	// user state
	hmm_vec3 cam_pos, cam_front, cam_up, cam_look;
	f32 cam_x, cam_y, cam_sens;
	f32 cam_yaw, cam_pitch;
	s32 first_mouse;

	f32 frame_curr, frame_last, frame_diff;

	// volume state
	// we update the uniforms for the volume every frame
	hmm_vec3 sim_pos, sim_front;
	fvec2_t control_left, control_right, control_trig; // 0 - x, 1 - y
	fvec3_t sim_rotp;
	f32 sim_stretch, sim_rotv;
	s32 sim_curraxis, sim_rotatevol;

	s32 sim_timeidx;

	int run;

	f64 lbound, hbound; // low and high range of values on ALL the meshes
};

// NOTE (brian) all of the other geometry, that ISN'T the simulation
// is defined by a series of 
struct block_t {
};

void viewer_bounds(f64 *p, u64 len, f64 *low, f64 *high);

void viewer_eventaxis(SDL_Event *event, struct simstate_t *state);
void viewer_eventbutton(SDL_Event *event, struct simstate_t *state);

void viewer_eventmotion(SDL_Event *event, struct simstate_t *state);
void viewer_eventmouse(SDL_Event *event, struct simstate_t *state);
void viewer_eventkey(SDL_Event *event, struct simstate_t *state);

void viewer_handleinput(struct simstate_t *state);

u32 viewer_mkshader(char *vertex, char *fragment);

/* viewer_run : runs the molt graphical 3d simulation viewer */
s32 viewer_run(void *hunk, u64 hunksize, s32 fd, struct molt_cfg_t *cfg)
{
	SDL_Window *window;
	SDL_GLContext glcontext;
	SDL_Event event;
	SDL_GameController *controller;

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

	memset(&state, 0, sizeof state);

	// initialize the camera
	state.first_mouse = 1;
	state.cam_pos = HMM_Vec3(0, 3, 3);
	state.cam_front = HMM_Vec3(0, 0, -1);
	state.cam_up = HMM_Vec3(0, 1, 0);

	// init the simulation rectangular prism in the same way
	state.sim_pos = HMM_Vec3(0, 3, 0);
	state.sim_front = HMM_Vec3(0, 0, -1);

	rc = 0, state.run = 1;
	mesh = io_lumpgetbase(hunk, MOLTLUMP_UMESH);
	molt_cfg_parampull_xyz(cfg, meshdim, MOLT_PARAM_POINTS);
	meshlen = meshdim[0] * meshdim[1] * meshdim[2];
	timesteps = cfg->t_params[MOLT_PARAM_POINTS];

	for (i = 0; i < timesteps; i++) {
		viewer_bounds(mesh[i].data, meshlen, &state.lbound, &state.hbound);
	}

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

	controller = SDL_GameControllerOpen(0);
	if (!controller) {
		ERRLOG("Couldn't open Controller 0", SDL_GetError());
		return -1;
	}

	// TODO (brian) check for window creation errors

	state.cam_x = WIDTH;
	state.cam_y = HEIGHT;
	state.cam_sens = MOUSE_SENSITIVITY;
	state.cam_yaw = -90;

	// setup controller information
	if (SDL_SetRelativeMouseMode(SDL_TRUE) == -1) {
		ERRLOG("Couldn't Set Relative Motion Mode", SDL_GetError());
		return -1;
	}

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

	// setup original cube state
	orig = HMM_Mat4d(1.0f);
	model = HMM_MultiplyMat4(orig, HMM_Translate(state.sim_pos));

	while (state.run) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_CONTROLLERAXISMOTION:
				viewer_eventaxis(&event, &state);
				break;
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
				viewer_eventbutton(&event, &state);
				break;
			case SDL_MOUSEMOTION:
				viewer_eventmotion(&event, &state);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				viewer_eventmouse(&event, &state);
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				viewer_eventkey(&event, &state);
				break;
			case SDL_QUIT:
				state.run = 0;
				break;
			default:
				break;
			}
		}

		viewer_handleinput(&state);

		// render
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(program_shader);

		// rotate our volume
		if (state.sim_rotatevol) {
			model = HMM_MultiplyMat4(model, HMM_Rotate(state.sim_rotp[0], HMM_Vec3(1, 0, 0)));
			model = HMM_MultiplyMat4(model, HMM_Rotate(state.sim_rotp[1], HMM_Vec3(0, 1, 0)));
			model = HMM_MultiplyMat4(model, HMM_Rotate(state.sim_rotp[2], HMM_Vec3(0, 0, 1)));
		}

		state.cam_look = HMM_AddVec3(state.cam_pos, state.cam_front);
		view = HMM_LookAt(state.cam_pos, state.cam_look, state.cam_up);

		proj = HMM_Perspective(90.0f, WIDTH / HEIGHT, 0.1f, 100.0f);

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

		state.frame_curr = ((f32)SDL_GetTicks()) / 1000.0f;
		state.frame_diff = state.frame_curr - state.frame_last;
		state.frame_last = state.frame_curr;

		if (state.frame_diff < TARGET_FRAMETIME) {
			SDL_Delay(((f32)SDL_GetTicks() / 1000) - state.frame_diff);
		}
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);

	// clean up from our gl shenanigans
	glDeleteProgram(program_shader);

	SDL_GameControllerClose(controller);
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return rc;
}

/* viewer_eventaxis : event handler for a controller axis input */
void viewer_eventaxis(SDL_Event *event, struct simstate_t *state)
{
	// NOTE https://wiki.libsdl.org/SDL_ControllerAxisEvent
	// the values that we'll read from the axis is between (-2^16)-(2^16 - 1)
	// so we have to map that into a -1 - 1 floating point range, to make the
	// other f32 math nice

	f32 mapval;

	if (!state->sim_rotatevol)
		return;

	mapval = event->caxis.value / ((f32)SHRT_MAX);

	if (-0.2 <= mapval && mapval < 0.2) // we don't want weird controller input
		return;

	switch ((s16)event->caxis.axis) {
	case SDL_CONTROLLER_AXIS_INVALID:
		ERRLOG("Invalid Axis!", SDL_GetError());
		break;
	case SDL_CONTROLLER_AXIS_LEFTX:
		state->control_left[0] = mapval;
		break;
	case SDL_CONTROLLER_AXIS_LEFTY:
		state->control_left[1] = mapval;
		break;
	case SDL_CONTROLLER_AXIS_RIGHTX:
		state->control_right[0] = mapval;
		break;
	case SDL_CONTROLLER_AXIS_RIGHTY:
		state->control_right[1] = mapval;
		break;
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT: // X
		state->control_trig[1] = mapval;
		break;
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: // Y
		state->control_trig[0] = mapval;
		break;
	default:
		assert(0);
		break;
	}
}

/* viewer_eventbutton : event handler for a controller button input */
void viewer_eventbutton(SDL_Event *event, struct simstate_t *state)
{
	// NOTE https://wiki.libsdl.org/SDL_ControllerButtonEvent
	// we mangle the boolean states here :)

	switch (event->cbutton.button) {
	case SDL_CONTROLLER_BUTTON_Y:
		if (event->cbutton.state == SDL_PRESSED) {
			// we need to set all of the rotations to zero to "pause"
			state->control_left[1] = 0;
			state->control_left[0] = 0;
			state->sim_rotp[0] = 0;
			state->sim_rotp[1] = 0;
			state->sim_rotp[2] = 0;
			state->sim_curraxis = (state->sim_curraxis+1) % (AXIS_Z+1);
			state->sim_rotatevol = state->sim_curraxis != AXIS_CAMERA;
			printf("state->sim_curraxis %d\n", state->sim_curraxis);
		}
		break;
	}
}

/* viewer_eventdevice : event handler for a controller device updates */
void viewer_eventdevice(SDL_Event *event, struct simstate_t *state)
{
	// TODO (brian) fill out if we actually need this
	// (really, just don't unplug the controller, and we're fine)
}

/* viewer_eventmotion : interprets and updates simstate from mouse motion */
void viewer_eventmotion(SDL_Event *event, struct simstate_t *state)
{
	f32 xoff, yoff;
	hmm_vec3 front;

	xoff = event->motion.xrel;
	yoff = event->motion.yrel;

	state->cam_x += xoff;
	state->cam_y += yoff;

	// mul for sensitivity
	xoff *= state->cam_sens;
	yoff *= state->cam_sens;

	state->cam_yaw   += xoff;
	state->cam_pitch -= yoff;

	// constrain the view so the user can't look 360 deg vertically
	state->cam_pitch = HMM_Clamp(-89.0f, state->cam_pitch, 89.0f);

	front.x = cos(HMM_ToRadians(state->cam_yaw)) * cos(HMM_ToRadians(state->cam_pitch));
	front.y = sin(HMM_ToRadians(state->cam_pitch));
	front.z = sin(HMM_ToRadians(state->cam_yaw)) * cos(HMM_ToRadians(state->cam_pitch));

	state->cam_front = HMM_NormalizeVec3(front);
}

/* viewer_eventmouse : mouse button event handler */
void viewer_eventmouse(SDL_Event *event, struct simstate_t *state)
{
	s8 *ptr;

	switch (event->button.button) {
	case SDL_BUTTON_LEFT:
		ptr = &state->mouse_l;
		break;
	case SDL_BUTTON_RIGHT:
		ptr = &state->mouse_r;
		break;
	case SDL_BUTTON_MIDDLE:
		ptr = &state->mouse_m;
		break;
	default:
		return;
	}

	if (event->button.state == SDL_PRESSED) {
		*ptr = 1;
	} else {
		*ptr = 0;
	}
}

/* viewer_getinputs : reads and toggles the internal keystate */
void viewer_eventkey(SDL_Event *event, struct simstate_t *state)
{
	s8 *ptr;
	switch (event->key.keysym.sym) {
		case SDLK_w:
			ptr = &state->key_w;
			break;
		case SDLK_a:
			ptr = &state->key_a;
			break;
		case SDLK_s:
			ptr = &state->key_s;
			break;
		case SDLK_d:
			ptr = &state->key_d;
			break;

#if 0
		case SDLK_PAGEUP:
			state->key_pgup = !state->key_pgup;
			break;
		case SDLK_PAGEDOWN:
			state->key_pgdown = !state->key_pgdown;
			break;
#endif

		case SDLK_ESCAPE:
			ptr = &state->key_esc;
			break;

		default:
			// if we don't have a key, we can yeet outta here
			return;
	}

	if (event->key.state == SDL_PRESSED) {
		*ptr = 1;
	} else {
		*ptr = 0;
	}
}

/* viewer_handleinput : updates the gamestate from the */
void viewer_handleinput(struct simstate_t *state)
{
	f32 camera_speed;
	hmm_vec3 tmp;

	// handle the "quit" sequence
	if (state->key_esc) {
		state->run = 0;
		return;
	}

	camera_speed = 2.5 * state->frame_diff;

	if (state->key_w) { // camera move forward
		tmp = HMM_MultiplyVec3f(state->cam_front, camera_speed);
		state->cam_pos = HMM_AddVec3(state->cam_pos, tmp);
	}

	if (state->key_s) { // camera move backward
		tmp = HMM_MultiplyVec3f(state->cam_front, camera_speed);
		state->cam_pos = HMM_SubtractVec3(state->cam_pos, tmp);
	}

	if (state->key_a) { // camera move left
		tmp = HMM_Cross(state->cam_front, state->cam_up);
		tmp = HMM_NormalizeVec3(tmp);
		tmp = HMM_MultiplyVec3f(tmp, camera_speed);
		state->cam_pos = HMM_SubtractVec3(state->cam_pos, tmp);
	}

	if (state->key_d) { // camera move right
		tmp = HMM_Cross(state->cam_front, state->cam_up);
		tmp = HMM_NormalizeVec3(tmp);
		tmp = HMM_MultiplyVec3f(tmp, camera_speed);
		state->cam_pos = HMM_AddVec3(state->cam_pos, tmp);
	}

	// update the cube position in 3d space
	if (state->sim_rotatevol) {
		state->sim_rotv = camera_speed * (state->control_left[1] - state->control_left[0]);
		switch (state->sim_curraxis) {
		case AXIS_X:
			state->sim_rotp[0] += state->sim_rotv;
			break;
		case AXIS_Y:
			state->sim_rotp[1] += state->sim_rotv;
			break;
		case AXIS_Z:
			state->sim_rotp[2] += state->sim_rotv;
			break;
		}
	}
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
	// hopefully they come in as zero
	if (*low > llow)
		*low  = llow;
	if (*high < lhigh)
		*high = lhigh;
}

