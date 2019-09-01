/*
 * Brian Chrzanowski
 * Fri Jul 19, 2019 22:48
 *
 * MOLT Viewer Implementation
 *
 * This viewer, for ease of transport, requires SDL2 to operate.
 *
 * TODO
 * 1. Single Particle
 * 2. Multiple Particle
 * 3. Array Instancing for Particle System
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <float.h>
#include <stdarg.h>

#include <SDL2/SDL.h> // TODO (brian) check include path for other operating systems

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

#include "common.h"
#include "molt.h"
#include "io.h"
#include "handmademath.h"
#include "viewer.h"

// get some other viewer flags out of the way
#define VIEWER_SDL_INITFLAGS \
	SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER
#define VIEWER_SDL_WINFLAGS SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE

#define ERRLOG(a,b) fprintf(stderr,"%s:%d %s %s\n",__FILE__,__LINE__,(a),(b))

#define VIEWER_MAXVEL 1.0f
#define VIEWER_FOV 45.0f

#define TARGET_FPS 144
#define TARGET_FRAMETIME 1000.0 / TARGET_FPS
#define WIDTH  1024
#define HEIGHT  768
#define MOUSE_SENSITIVITY 0.4f

#define MAX_PARTICLES 1000
#define VOL_BOUND (2 * M_PI)

// shader container
struct shader_cont_t {
	char *str;
	u32 shader;
	u32 vbo;
	u32 vao;
	u32 loc_view;
	u32 loc_proj;
	u32 loc_model;
};

struct intmap_t {
	int from;
	int to;
};

static struct intmap_t sdlkey_to_inputkey[] = {
	// numerics
	{ SDLK_0, INPUT_KEY_0 },
	{ SDLK_1, INPUT_KEY_1 },
	{ SDLK_2, INPUT_KEY_2 },
	{ SDLK_3, INPUT_KEY_3 },
	{ SDLK_4, INPUT_KEY_4 },
	{ SDLK_5, INPUT_KEY_5 },
	{ SDLK_6, INPUT_KEY_6 },
	{ SDLK_7, INPUT_KEY_7 },
	{ SDLK_8, INPUT_KEY_8 },
	{ SDLK_9, INPUT_KEY_9 },

	// a-z keys in qwerty layout
	{ SDLK_q, INPUT_KEY_Q },
	{ SDLK_w, INPUT_KEY_W },
	{ SDLK_e, INPUT_KEY_E },
	{ SDLK_r, INPUT_KEY_R },
	{ SDLK_t, INPUT_KEY_T },
	{ SDLK_y, INPUT_KEY_Y },
	{ SDLK_u, INPUT_KEY_U },
	{ SDLK_i, INPUT_KEY_I },
	{ SDLK_o, INPUT_KEY_O },
	{ SDLK_p, INPUT_KEY_P },
	{ SDLK_a, INPUT_KEY_A },
	{ SDLK_s, INPUT_KEY_S },
	{ SDLK_d, INPUT_KEY_D },
	{ SDLK_f, INPUT_KEY_F },
	{ SDLK_g, INPUT_KEY_G },
	{ SDLK_h, INPUT_KEY_H },
	{ SDLK_j, INPUT_KEY_J },
	{ SDLK_k, INPUT_KEY_K },
	{ SDLK_l, INPUT_KEY_L },
	{ SDLK_z, INPUT_KEY_Z },
	{ SDLK_x, INPUT_KEY_X },
	{ SDLK_c, INPUT_KEY_C },
	{ SDLK_v, INPUT_KEY_V },
	{ SDLK_b, INPUT_KEY_B },
	{ SDLK_n, INPUT_KEY_N },
	{ SDLK_m, INPUT_KEY_M },

	// modifier keys
	{ SDLK_LSHIFT, INPUT_KEY_SHIFT },
	{ SDLK_TAB, INPUT_KEY_TAB },

	{ SDLK_UP,    INPUT_KEY_UARROW },
	{ SDLK_RIGHT, INPUT_KEY_RARROW },
	{ SDLK_DOWN,  INPUT_KEY_DARROW },
	{ SDLK_LEFT,  INPUT_KEY_LARROW },

	// extras
	{ SDLK_ESCAPE, INPUT_KEY_ESC },
	{ SDLK_SPACE,  INPUT_KEY_SPACE },
	{ SDLK_HOME,   INPUT_KEY_HOME },
	{ SDLK_END,    INPUT_KEY_END },
	{ SDLK_LCTRL,  INPUT_KEY_CTRL }
};


struct input_t {
	s8 key[INPUT_KEY_TOTAL];
	ivec2_t mousepos;
	fvec2_t control_left, control_right, control_trig;
};

struct output_t {
	SDL_Window *window;
	s32 win_w, win_h;
};

struct simstate_t {
	struct input_t input;
	struct output_t output;

	// user state
	hmm_vec3 cam_pos, cam_front, cam_up, cam_look;
	f32 cam_x, cam_y, cam_sens;
	f32 cam_yaw, cam_pitch;

	f32 frame_curr, frame_last, frame_diff;
	// curr - current time in ms
	// last - last time we drew a frame
	// delta- difference between the two

	int run;
};

void viewer_bounds(f64 *p, u64 len, f64 *low, f64 *high);

void viewer_eventaxis(SDL_Event *event, struct simstate_t *state);
void viewer_eventbutton(SDL_Event *event, struct simstate_t *state);

void viewer_eventmotion(SDL_Event *event, struct simstate_t *state);
void viewer_eventmouse(SDL_Event *event, struct simstate_t *state);
void viewer_eventkey(SDL_Event *event, struct simstate_t *state);
void viewer_eventwindow(SDL_Event *event, struct simstate_t *state);
void v_keystatecycle(struct simstate_t *state);

void viewer_handleinput(struct simstate_t *state);

u32 viewer_mkshader(char *vertex, char *fragment);

int v_loadopenglfuncs();
int v_instatecheck(struct simstate_t *state, int kstate, int amt, ...);
int v_iteminlist(int v, int n, ...);

struct openglfuncs_t {
	char *str;
	void *ptr;
	void **funcptr;
	int status;
};

enum {
	VGLSTAT_NULL = -1,
	VGLSTAT_UNLOADED,
	VGLSTAT_LOADED,
	VGLSTAT_UNSUPPORTED
};

/*
 * NOTE (brian)
 * to reduce the amount of external project dependencies, I've made the
 * executive decision to avoid as many external dependencies as possible.
 * This means that for the next basket of lines, we're loading our own OpenGL
 * functions. No GLEW needed here.
 *
 * CONVENTION
 * Sat Aug 31, 2019 20:31
 * To avoid redefining the entire OpenGL spec, the functions that we use
 * are "Viewer GL *" (hence the v in front of all of the GL functions that
 * we have). There's probably a better way to do this :(
 *
 * HERE BE DRAGONS
 */

// the OpenGL functions that we need
typedef void (APIENTRY * GL_UseProgram_Func)(GLuint);
GL_UseProgram_Func vglUseProgram;
typedef void (APIENTRY * GL_GenBuffers_Func)(GLsizei n, GLuint *buffers);
GL_GenBuffers_Func vglGenBuffers;
typedef void (APIENTRY * GL_BindBuffer_Func)(GLenum target, GLuint buffer);
GL_BindBuffer_Func vglBindBuffer;
typedef void (APIENTRY * GL_BufferData_Func)(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
GL_BufferData_Func vglBufferData;
typedef GLint (APIENTRY * GL_GetUniformLocation_Func)(GLuint program, const GLchar *name);
GL_GetUniformLocation_Func vglGetUniformLocation;
typedef void (APIENTRY * GL_EnableVertexAttribArray_Func)(GLuint index);
GL_EnableVertexAttribArray_Func vglEnableVertexAttribArray;
typedef void (APIENTRY * GL_DisableVertexAttribArray_Func)(GLuint index);
GL_DisableVertexAttribArray_Func vglDisableVertexAttribArray;
typedef void (APIENTRY * GL_VertexAttribPointer_Func)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
GL_VertexAttribPointer_Func vglVertexAttribPointer;
typedef void (APIENTRY * GL_VertexAttribDivisor_Func)(GLuint index, GLuint divisor);
GL_VertexAttribDivisor_Func vglVertexAttribDivisor;
typedef void (APIENTRY * GL_DrawArraysInstanced_Func)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
GL_DrawArraysInstanced_Func vglDrawArraysInstanced;
typedef void (APIENTRY * GL_DeleteVertexArrays_Func)(GLsizei n, const GLuint *arrays);
GL_DeleteVertexArrays_Func vglDeleteVertexArrays;
typedef void (APIENTRY * GL_DeleteBuffers_Func)(GLsizei n, const GLuint *buffers);
GL_DeleteBuffers_Func vglDeleteBuffers;
typedef void (APIENTRY * GL_DeleteProgram_Func)(GLuint program);
GL_DeleteProgram_Func vglDeleteProgram;
typedef void (APIENTRY * GL_GenVertexArrays_Func)(GLsizei n, GLuint *arrays);
GL_GenVertexArrays_Func vglGenVertexArrays;
typedef void (APIENTRY * GL_BindVertexArray_Func)(GLuint array);
GL_BindVertexArray_Func vglBindVertexArray;
typedef GLint (APIENTRY * GL_GetAttribLocation_Func)(GLuint program, const GLchar *name);
GL_GetAttribLocation_Func vglGetAttribLocation;
typedef GLuint (APIENTRY * GL_CreateShader_Func)(GLenum shadertype);
GL_CreateShader_Func vglCreateShader;
typedef void (APIENTRY * GL_ShaderSource_Func)(GLuint shader, GLsizei count, const GLchar **string, const GLint *lenght);
GL_ShaderSource_Func vglShaderSource;
typedef void (APIENTRY * GL_CompileShader_Func)(GLuint shader);
GL_CompileShader_Func vglCompileShader;
typedef void (APIENTRY * GL_GetShaderiv_Func)(GLuint shader, GLenum name, GLint *params);
GL_GetShaderiv_Func vglGetShaderiv;
typedef void (APIENTRY * GL_GetShaderInfoLog_Func)(GLuint shader, GLsizei maxlen, GLsizei *length, GLchar *infolog);
GL_GetShaderInfoLog_Func vglGetShaderInfoLog;
typedef GLuint (APIENTRY * GL_CreateProgram_Func)();
GL_CreateProgram_Func vglCreateProgram;
typedef void (APIENTRY * GL_AttachShader_Func)(GLuint program, GLuint shader);
GL_AttachShader_Func vglAttachShader;
typedef void (APIENTRY * GL_LinkProgram_Func)(GLuint program);
GL_LinkProgram_Func vglLinkProgram;
typedef void (APIENTRY * GL_GetProgramiv_Func)(GLuint program, GLenum name, GLint *params);
GL_GetProgramiv_Func vglGetProgramiv;
typedef void (APIENTRY * GL_GetProgramInfoLog_Func)(GLuint program, GLsizei maxlen, GLsizei *len, GLchar *infolog);
GL_GetProgramInfoLog_Func vglGetProgramInfoLog;
typedef void (APIENTRY * GL_DeleteShader_Func)(GLuint shader);
GL_DeleteShader_Func vglDeleteShader;
typedef void (APIENTRY * GL_Viewport_Func)(GLint x, GLint y, GLsizei width, GLsizei height);
GL_Viewport_Func vglViewport;

// some uniform functions
typedef void (APIENTRY * GL_UniformMatrix4fv_Func)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GL_UniformMatrix4fv_Func vglUniformMatrix4fv;
typedef void (APIENTRY * GL_Uniform1f_Func)(GLint location, GLfloat v0);
GL_Uniform1f_Func vglUniform1f;
typedef void (APIENTRY * GL_Uniform2f_Func)(GLint location, GLfloat v0, GLfloat v1);
GL_Uniform2f_Func vglUniform2f;
typedef void (APIENTRY * GL_Uniform3f_Func)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
GL_Uniform3f_Func vglUniform3f;
typedef void (APIENTRY * GL_Uniform4f_Func)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GL_Uniform4f_Func vglUniform4f;

// "OpenGL Function Make"
#define PP_CONCAT(x,y)   x##y
#define OGLFUNCMK(x)     {#x, NULL, (void **)&PP_CONCAT(v,x), 0}

struct openglfuncs_t openglfunc_table[] = {
	OGLFUNCMK(glUseProgram),
	OGLFUNCMK(glGenBuffers),
	OGLFUNCMK(glBindBuffer),
	OGLFUNCMK(glBufferData),
	OGLFUNCMK(glGetUniformLocation),
	OGLFUNCMK(glEnableVertexAttribArray),
	OGLFUNCMK(glDisableVertexAttribArray),
	OGLFUNCMK(glVertexAttribPointer),
	OGLFUNCMK(glVertexAttribDivisor),
	OGLFUNCMK(glDrawArraysInstanced),
	OGLFUNCMK(glDeleteVertexArrays),
	OGLFUNCMK(glDeleteBuffers),
	OGLFUNCMK(glDeleteProgram),
	OGLFUNCMK(glGenVertexArrays),
	OGLFUNCMK(glBindVertexArray),
	OGLFUNCMK(glGetAttribLocation),
	OGLFUNCMK(glCreateShader),
	OGLFUNCMK(glShaderSource),
	OGLFUNCMK(glCompileShader),
	OGLFUNCMK(glGetShaderiv),
	OGLFUNCMK(glGetShaderInfoLog),
	OGLFUNCMK(glCreateProgram),
	OGLFUNCMK(glAttachShader),
	OGLFUNCMK(glLinkProgram),
	OGLFUNCMK(glGetProgramiv),
	OGLFUNCMK(glGetProgramInfoLog),
	OGLFUNCMK(glDeleteShader),
	OGLFUNCMK(glUniformMatrix4fv),
	OGLFUNCMK(glUniform1f),
	OGLFUNCMK(glUniform2f),
	OGLFUNCMK(glUniform3f),
	OGLFUNCMK(glUniform4f),
	OGLFUNCMK(glViewport)
};

// macro and functions to check for opengl errors
void gl_check_errors(const char *file, int line);

#ifdef DEBUG
#define GL_ERROR_CHECK() gl_check_errors(__FILE__, __LINE__)
#endif
#ifndef DEBUG
#define GL_ERROR_CHECK() 
#endif
void gl_check_errors(const char *file, int line)
{
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR) {
		// Process/log the error.
		// TODO: display the error in hex
		fprintf(stderr, "opengl error at %s:%i: %i\n", file, line, err);
	}
}

// particular_soln : computes the particular solution for a wave in 3d
// TODO remove and use real MOLT computations
f32 particular_soln(f32 x, f32 y, f32 z, f32 t)
{
	const f32 k = 0.4f;
	const f32 l = 0.4f;
	const f32 m = 0.4f;
	const f32 c = 1;

	return sin(k * M_PI * x) * sin(l * M_PI * y) * sin(m * M_PI * z) *
		cos(c * M_PI * sqrt(pow(k,2) + pow(l,2) + pow(m,2)) * t);
}

/* viewer_run : runs the molt graphical 3d simulation viewer */
s32 viewer_run(void *hunk, u64 hunksize, s32 fd, struct molt_cfg_t *cfg)
{
	SDL_GLContext glcontext;
	SDL_Event event;
	SDL_GameController *controller;

	struct simstate_t state;
	struct shader_cont_t shaders[2];
	s32 rc, i;

	float bbverts[] = {
		-0.5f, -0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f,
		 0.5f,  0.5f, 0.0f
	};

	hmm_mat4 model, view, proj, orig;
	hmm_mat4 part_model;

	memset(&state, 0, sizeof state);

	// initialize the camera
	state.cam_pos = HMM_Vec3(-12, 12, 12);//0, 3, 9);
	state.cam_front = HMM_MultiplyVec3f(state.cam_pos, -1);
	state.cam_up = HMM_Vec3(0, 1, 0);

	rc = 0, state.run = 1;

	if (SDL_Init(VIEWER_SDL_INITFLAGS) != 0) {
		ERRLOG("Couldn't Init SDL", SDL_GetError());
		return -1;
	}

	state.output.window =
		SDL_CreateWindow("MOLT Viewer", 64, 64, WIDTH, HEIGHT, VIEWER_SDL_WINFLAGS);
	if (state.output.window == NULL) {
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

	glcontext = SDL_GL_CreateContext(state.output.window);

	v_loadopenglfuncs();

	// set no vsync (??)
	if (SDL_GL_SetSwapInterval(0) != 0) {
		ERRLOG("Couldn't set the swap interval", SDL_GetError());
	}

	glEnable(GL_DEPTH_TEST);

	// setup our particle shader
	shaders[0].shader = viewer_mkshader("src/particle.vert", "src/particle.frag");
	shaders[0].str = "particle_shader";

	vglGenVertexArrays(1, &shaders[0].vao);
	vglGenBuffers(1, &shaders[0].vbo);

	vglBindBuffer(GL_ARRAY_BUFFER, shaders[0].vbo);
	vglBufferData(GL_ARRAY_BUFFER, sizeof(bbverts), bbverts, GL_STATIC_DRAW);

	vglBindVertexArray(shaders[0].vao);
	vglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	vglEnableVertexAttribArray(0);

	shaders[0].loc_model = vglGetUniformLocation(shaders[0].shader, "uModel");
	shaders[0].loc_view  = vglGetUniformLocation(shaders[0].shader, "uView");
	shaders[0].loc_proj  = vglGetUniformLocation(shaders[0].shader, "uProj");

	// setup original cube state
	orig = HMM_Mat4d(1.0f);
	part_model = HMM_MultiplyMat4(orig, HMM_Translate(HMM_Vec3(0, 5, 0)));

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
			case SDL_WINDOWEVENT:
				viewer_eventwindow(&event, &state);
				break;
			case SDL_QUIT:
				state.run = 0;
				break;
			default:
				break;
			}
		}

		if (state.input.key[INPUT_KEY_F]) {
			// TODO move the fullscreen call
			// TODO change gl settings and resolution when this happens
			SDL_SetWindowFullscreen(state.output.window, SDL_WINDOW_FULLSCREEN);
		}

		viewer_handleinput(&state);

		// render
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		state.cam_look = HMM_AddVec3(state.cam_pos, state.cam_front);
		view = HMM_LookAt(state.cam_pos, state.cam_look, state.cam_up);

		proj = HMM_Perspective(90.0f, WIDTH / HEIGHT, 0.1f, 100.0f);

		// draw the particles
		vglUseProgram(shaders[0].shader);

		vglUniformMatrix4fv(shaders[0].loc_model, 1, GL_FALSE, (f32 *)&part_model);
		vglUniformMatrix4fv(shaders[0].loc_view,  1, GL_FALSE, (f32 *)&view);
		vglUniformMatrix4fv(shaders[0].loc_proj,  1, GL_FALSE, (f32 *)&proj);

		vglBindVertexArray(shaders[0].vao);

		u32 locPos, locMag, locRes;
		locPos = vglGetUniformLocation(shaders[0].shader, "uPos");
		locMag = vglGetUniformLocation(shaders[0].shader, "uMag");
		locRes = vglGetUniformLocation(shaders[0].shader, "uRes");
		vglUniform2f(locRes, state.output.win_w, state.output.win_h);

		f32 lx, ly, lz, lw;
		for (lx = -VOL_BOUND; lx < VOL_BOUND; lx += 0.5)
		for (ly = -VOL_BOUND; ly < VOL_BOUND; ly += 0.5)
		for (lz = -VOL_BOUND; lz < VOL_BOUND; lz += 0.5) {
			lw = particular_soln(lx, ly, lz, state.frame_curr);
			vglUniform4f(locPos, lx, ly, lz, lw);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
		}

		// redraw the screen
		SDL_GL_SwapWindow(state.output.window);

		vglUseProgram(0);

		v_keystatecycle(&state);

		// get time to draw frame, wait if needed
		state.frame_curr = ((f32)SDL_GetTicks()) / 1000.0f;
		state.frame_diff = state.frame_curr - state.frame_last;

		printf("Frame : %4.3fms\n", state.frame_diff);

		if (state.frame_diff < TARGET_FRAMETIME) {
			SDL_Delay(((f32)SDL_GetTicks()) / 1000 - state.frame_curr);
		}

		state.frame_last = state.frame_curr;
	}

	vglDeleteVertexArrays(1, &shaders[0].vao);
	vglDeleteBuffers(1, &shaders[0].vbo);

	// clean up from our gl shenanigans
	vglDeleteProgram(shaders[0].shader);

	SDL_GameControllerClose(controller);
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(state.output.window);
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

	mapval = event->caxis.value / ((f32)SHRT_MAX);

	if (-0.2 <= mapval && mapval < 0.2) // we don't want weird controller input
		return;

	switch ((s16)event->caxis.axis) {
	case SDL_CONTROLLER_AXIS_INVALID:
		ERRLOG("Invalid Axis!", SDL_GetError());
		break;
	case SDL_CONTROLLER_AXIS_LEFTX:
		state->input.control_left[0] = mapval;
		break;
	case SDL_CONTROLLER_AXIS_LEFTY:
		state->input.control_left[1] = mapval;
		break;
	case SDL_CONTROLLER_AXIS_RIGHTX:
		state->input.control_right[0] = mapval;
		break;
	case SDL_CONTROLLER_AXIS_RIGHTY:
		state->input.control_right[1] = mapval;
		break;
	case SDL_CONTROLLER_AXIS_TRIGGERLEFT: // X
		state->input.control_trig[1] = mapval;
		break;
	case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: // Y
		state->input.control_trig[0] = mapval;
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
		ptr = &state->input.key[INPUT_KEY_MLEFT];
		break;
	case SDL_BUTTON_RIGHT:
		ptr = &state->input.key[INPUT_KEY_MRIGHT];
		break;
	case SDL_BUTTON_MIDDLE:
		ptr = &state->input.key[INPUT_KEY_MCENTER];
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
	int items, i;

	items = sizeof(sdlkey_to_inputkey) / sizeof(sdlkey_to_inputkey[0]);

	for (i = 0; i < items; i++) {
		if (event->key.keysym.sym == sdlkey_to_inputkey[i].from) {
			if (event->key.state == SDL_PRESSED) {
				state->input.key[sdlkey_to_inputkey[i].to] = 1;
			} else {
				state->input.key[sdlkey_to_inputkey[i].to] = 0;
			}
			break;
		}
	}
}

/* v_keystatecycle : cycles the keystate array */
void v_keystatecycle(struct simstate_t *state)
{
	// PRESSED -> HELD
	// RELEASED -> NOTHING
	int i;

	for (i = 0; i < INPUT_KEY_TOTAL; i++) {
		switch (state->input.key[i]) {
			case INSTATE_PRESSED:
				state->input.key[i] = INSTATE_HELD;
				break;
			case INSTATE_RELEASED:
				state->input.key[i] = INSTATE_NOTHING;
				break;
			default:
				continue;
		}
	}
}

/* viewer_eventwindow : handles SDL window events */
void viewer_eventwindow(SDL_Event *event, struct simstate_t *state)
{
	switch (event->window.event) {
	case SDL_WINDOWEVENT_SHOWN:
	case SDL_WINDOWEVENT_HIDDEN:
	case SDL_WINDOWEVENT_EXPOSED:
	case SDL_WINDOWEVENT_MOVED:
	case SDL_WINDOWEVENT_MINIMIZED:
	case SDL_WINDOWEVENT_MAXIMIZED:
	case SDL_WINDOWEVENT_RESTORED:
	case SDL_WINDOWEVENT_ENTER:
	case SDL_WINDOWEVENT_LEAVE:
	case SDL_WINDOWEVENT_FOCUS_GAINED:
	case SDL_WINDOWEVENT_FOCUS_LOST:
		break;
	case SDL_WINDOWEVENT_RESIZED:
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		state->output.win_w = event->window.data1;
		state->output.win_h = event->window.data2;
		vglViewport(0, 0, state->output.win_w, state->output.win_h);
		break;
	case SDL_WINDOWEVENT_CLOSE:
		state->run = 0;
		break;
	default:
		break;
	}
}

/* viewer_handleinput : updates the gamestate from the */
void viewer_handleinput(struct simstate_t *state)
{
	f32 camera_speed;
	hmm_vec3 tmp;

#define IN_MOVEF     INPUT_KEY_W
#define IN_MOVEB     INPUT_KEY_S
#define IN_MOVEL     INPUT_KEY_A
#define IN_MOVER     INPUT_KEY_D
#define IN_MOVEU     INPUT_KEY_SPACE
#define IN_MOVED     INPUT_KEY_SHIFT
// #define IN_FAST      INPUT_KEY_SHIFT
#define IN_QUIT      INPUT_KEY_ESC, INPUT_KEY_Q

	// handle the "quit" sequence
	if (v_instatecheck(state, INSTATE_PRESSED, PP_NARG(IN_QUIT), IN_QUIT)) {
		state->run = 0;
		return;
	}

	camera_speed = 3 * state->frame_diff;

	if (v_instatecheck(state, INSTATE_DOWN, PP_NARG(IN_MOVEF), IN_MOVEF)) {
		tmp = HMM_MultiplyVec3f(state->cam_front, camera_speed);
		state->cam_pos = HMM_AddVec3(state->cam_pos, tmp);
	}

	if (v_instatecheck(state, INSTATE_DOWN, PP_NARG(IN_MOVEB), IN_MOVEB)) {
		tmp = HMM_MultiplyVec3f(state->cam_front, camera_speed);
		state->cam_pos = HMM_SubtractVec3(state->cam_pos, tmp);
	}

	if (v_instatecheck(state, INSTATE_DOWN, PP_NARG(IN_MOVEL), IN_MOVEL)) {
		tmp = HMM_Cross(state->cam_front, state->cam_up);
		tmp = HMM_NormalizeVec3(tmp);
		tmp = HMM_MultiplyVec3f(tmp, camera_speed);
		state->cam_pos = HMM_SubtractVec3(state->cam_pos, tmp);
	}

	if (v_instatecheck(state, INSTATE_DOWN, PP_NARG(IN_MOVER), IN_MOVER)) {
		tmp = HMM_Cross(state->cam_front, state->cam_up);
		tmp = HMM_NormalizeVec3(tmp);
		tmp = HMM_MultiplyVec3f(tmp, camera_speed);
		state->cam_pos = HMM_AddVec3(state->cam_pos, tmp);
	}

	if (v_instatecheck(state, INSTATE_DOWN, PP_NARG(IN_MOVEU), IN_MOVEU)) {
		tmp = HMM_Vec3(0, camera_speed, 0);
		state->cam_pos = HMM_AddVec3(state->cam_pos, tmp);
	}

	if (v_instatecheck(state, INSTATE_DOWN, PP_NARG(IN_MOVED), IN_MOVED)) {
		tmp = HMM_Vec3(0, -camera_speed, 0);
		state->cam_pos = HMM_AddVec3(state->cam_pos, tmp);
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

	if (!vertex) {
		fprintf(stderr, "Couldn't load [%s], doesn't exist\n", vertex_file);
		exit(0);
	}

	if (!fragment) {
		fprintf(stderr, "Couldn't load [%s], doesn't exist\n", fragment_file);
		exit(0);
	}

	// compile the desired vertex shader
	vshader_id = vglCreateShader(GL_VERTEX_SHADER);
	vglShaderSource(vshader_id, 1, (void *)&vertex, NULL);
	vglCompileShader(vshader_id);

	// vertex shader error checking
	vglGetShaderiv(vshader_id, GL_COMPILE_STATUS, &success);
	if (!success) {
		vglGetShaderInfoLog(vshader_id, sizeof err_buf, NULL, err_buf);
		fprintf(stderr, "vshader error [%s] : %s\n", vertex_file, err_buf);
		assert(0);
	}

	// compile the desired fragment shader
	fshader_id = vglCreateShader(GL_FRAGMENT_SHADER);
	vglShaderSource(fshader_id, 1, (void *)&fragment, NULL);
	vglCompileShader(fshader_id);

	// fragment shader error checking
	vglGetShaderiv(fshader_id, GL_COMPILE_STATUS, &success);
	if (!success) {
		vglGetShaderInfoLog(fshader_id, sizeof err_buf, NULL, err_buf);
		fprintf(stderr, "fshader error [%s] : %s\n", fragment_file, err_buf);
		assert(0);
	}

	// attach and link the program
	program_id = vglCreateProgram();
	vglAttachShader(program_id, vshader_id);
	vglAttachShader(program_id, fshader_id);
	vglLinkProgram(program_id);

	// now do some error checking
	// glValidateProgram(program_id);
	vglGetProgramiv(program_id, GL_LINK_STATUS, &success);

	if (!success) {
		vglGetProgramInfoLog(program_id, sizeof err_buf, &sizei, err_buf);
		ERRLOG("shader link error:\n", err_buf);
		assert(0);
	}

	// clean up from our shader shenanigans
	vglDeleteShader(vshader_id);
	vglDeleteShader(fshader_id);

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

/* viewer_loadopenglfuncs : load the opengl functions that we need */
int v_loadopenglfuncs()
{
	struct openglfuncs_t *tab;
	int rc, i;
	
	rc = 0;
	tab = openglfunc_table;
	
	// TODO (brian) check for extensions here
	
	for (i = 0; i < ARRSIZE(openglfunc_table); i++) {
		tab[i].ptr = SDL_GL_GetProcAddress(tab[i].str);
		*tab[i].funcptr = tab[i].ptr;
		tab[i].status = tab[i].ptr ? VGLSTAT_LOADED : VGLSTAT_UNLOADED;
	}

	for (i = 0; i < ARRSIZE(openglfunc_table); i++) {
		// check for any issues in our UNLOADED functions here
		if (tab[i].status == VGLSTAT_UNLOADED) {
			fprintf(stderr, "ERROR : [%s] Couldn't be found!\n", tab[i].str);
			exit(0);
		}
	}
	
	return rc;
}

/* v_instatecheck : Check variable amounts of keys for a specific state */
int v_instatecheck(struct simstate_t *state, int kstate, int amt, ...)
{
	// NOTE	(brian) returns true if any key is in the state
	// returns 1 on "some keys are in teh state", 0 on others
	va_list list;
	int keyval, val, i, rc;

	rc = 0;
	va_start(list, amt);

	for (i = 0; i < amt && rc == 0; i++) {
		val = va_arg(list, int);
		keyval = state->input.key[val];
		switch (kstate) {
			case INSTATE_DOWN:
				if (v_iteminlist(keyval, 2, INSTATE_PRESSED, INSTATE_HELD)) {
					rc = 1;
				}
				break;
			case INSTATE_UP:
				if (v_iteminlist(keyval, 2, INSTATE_NOTHING, INSTATE_RELEASED)) {
					rc = 1;
				}
				break;

			case INSTATE_NOTHING:
			case INSTATE_RELEASED:
			case INSTATE_PRESSED:
			case INSTATE_HELD:
				if (keyval == kstate) {
					rc = 1;
				}
				break;
		}
	}

	va_end(list);

	return rc;
}

/* v_iteminlist : checks if v appears in the n item list of args */
int v_iteminlist(int v, int n, ...)
{
	va_list list;
	int i, val, rc;
	rc = 0;

	va_start(list, n);

	for (i = 0; i < n; i++) {
		val = va_arg(list, int);
		if (val == v) {
			rc = 1;
			break;
		}
	}

	va_end(list);

	return rc;
}

