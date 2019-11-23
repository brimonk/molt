/*
 * Brian Chrzanowski
 * Fri Jul 19, 2019 22:48
 *
 * MOLT Viewer Implementation
 *
 * TODO
 * 1. Get Fonts Working
 * 2. Print out the following at the top of the screen (for demo purposes)
 *    Position (x,y,z)
 *    Time     (start,curr,stop)
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

#define HANDMADE_MATH_IMPLEMENTATION
#include "handmademath.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "common.h"
#include "molt.h"
#include "lump.h"
#include "sys.h"
#include "viewer.h"

// get some other viewer flags out of the way
#define VIEWER_SDL_INITFLAGS SDL_INIT_EVERYTHING // SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER
#define VIEWER_SDL_WINFLAGS SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE

#define ERRLOG(a,b) fprintf(stderr,"%s:%d %s %s\n",__FILE__,__LINE__,(a),(b))

#define VIEWER_MAXVEL 1.0f
#define VIEWER_FOV 45.0f

#define TARGET_FPS 144
#define TARGET_FRAMETIME 1000.0 / TARGET_FPS
#define WIDTH  1024
#define HEIGHT  768
#define MOUSE_SENSITIVITY 0.4f

#define FONT_PATH "assets/fonts/LiberationMono-Regular.ttf"

// shader container
struct shader_cont_t {
	char *str;
	u32 shader;
	u32 vbo;
	u32 vao;
	u32 loc_view;
	u32 loc_proj;
};

struct fchar_t {
	u32 texture;
	s32 f_x; // font size
	s32 f_y;
	s32 b_x; // bearing information
	s32 b_y;
	u32 advance;
};

// font container
struct frender_t {
	// shader and OpenGL information
	u32 shader, vao, vbo, eab;

	u32 loc_pers;
	u32 loc_view;

	s32 vertadvance;

	// text printing information
	fvec2_t pos;
	fvec4_t color;
};

struct intmap_t {
	int from;
	int to;
};

enum {
	INSTATE_NOTHING,
	INSTATE_PRESSED,
	INSTATE_HELD,
	INSTATE_RELEASED,
	// down -> PRESSED & HELD
	// up   -> NOTHING & RELEASED
	INSTATE_DOWN,
	INSTATE_UP,
	INSTATE_TOTAL
};

enum {
	INPUT_KEY__START,
	// numerics
	INPUT_KEY_0,
	INPUT_KEY_1,
	INPUT_KEY_2,
	INPUT_KEY_3,
	INPUT_KEY_4,
	INPUT_KEY_5,
	INPUT_KEY_6,
	INPUT_KEY_7,
	INPUT_KEY_8,
	INPUT_KEY_9,

	// a-z keys in qwerty layout
	INPUT_KEY_Q,
	INPUT_KEY_W,
	INPUT_KEY_E,
	INPUT_KEY_R,
	INPUT_KEY_T,
	INPUT_KEY_Y,
	INPUT_KEY_U,
	INPUT_KEY_I,
	INPUT_KEY_O,
	INPUT_KEY_P,
	INPUT_KEY_A,
	INPUT_KEY_S,
	INPUT_KEY_D,
	INPUT_KEY_F,
	INPUT_KEY_G,
	INPUT_KEY_H,
	INPUT_KEY_J,
	INPUT_KEY_K,
	INPUT_KEY_L,
	INPUT_KEY_Z,
	INPUT_KEY_X,
	INPUT_KEY_C,
	INPUT_KEY_V,
	INPUT_KEY_B,
	INPUT_KEY_N,
	INPUT_KEY_M,

	INPUT_KEY_UARROW,
	INPUT_KEY_RARROW,
	INPUT_KEY_DARROW,
	INPUT_KEY_LARROW,

	// modifier keys
	INPUT_KEY_SHIFT,
	INPUT_KEY_TAB,

	// extras
	INPUT_KEY_ESC,
	INPUT_KEY_SPACE,
	INPUT_KEY_HOME,
	INPUT_KEY_END,
	INPUT_KEY_CTRL,
	INPUT_KEY__DONE,

	// mouse "keys"
	INPUT_MOUSE__START,
	INPUT_MOUSE_LEFT,
	INPUT_MOUSE_RIGHT,
	INPUT_MOUSE_CENTER,
	INPUT_MOUSE__DONE,

	// controller "keys"
	INPUT_CTRLLR__START,
	INPUT_CTRLLR_A,
	INPUT_CTRLLR_B,
	INPUT_CTRLLR_X,
	INPUT_CTRLLR_Y,
	INPUT_CTRLLR_BACK,
	INPUT_CTRLLR_START,
	INPUT_CTRLLR_LSTICK,
	INPUT_CTRLLR_RSTICK,
	INPUT_CTRLLR_LSHOULDER,
	INPUT_CTRLLR_RSHOULDER,
	INPUT_CTRLLR_DPAD_U,
	INPUT_CTRLLR_DPAD_D,
	INPUT_CTRLLR_DPAD_L,
	INPUT_CTRLLR_DPAD_R,
	INPUT_CTRLLR__DONE,

	INPUT_KEY_TOTAL
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
	{ SDLK_LCTRL,  INPUT_KEY_CTRL },

	// mouse
	{ SDL_BUTTON_LEFT, INPUT_MOUSE_LEFT },
	{ SDL_BUTTON_MIDDLE, INPUT_MOUSE_CENTER },
	{ SDL_BUTTON_RIGHT, INPUT_MOUSE_RIGHT },

	// controller
	{ SDL_CONTROLLER_BUTTON_A, INPUT_CTRLLR_A },
	{ SDL_CONTROLLER_BUTTON_B, INPUT_CTRLLR_B },
	{ SDL_CONTROLLER_BUTTON_X, INPUT_CTRLLR_X },
	{ SDL_CONTROLLER_BUTTON_Y, INPUT_CTRLLR_Y },
	{ SDL_CONTROLLER_BUTTON_BACK, INPUT_CTRLLR_BACK },
	{ SDL_CONTROLLER_BUTTON_START, INPUT_CTRLLR_START },
	{ SDL_CONTROLLER_BUTTON_LEFTSTICK, INPUT_CTRLLR_LSTICK },
	{ SDL_CONTROLLER_BUTTON_RIGHTSTICK, INPUT_CTRLLR_RSTICK },
	{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER, INPUT_CTRLLR_LSHOULDER },
	{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, INPUT_CTRLLR_RSHOULDER },
	{ SDL_CONTROLLER_BUTTON_DPAD_UP, INPUT_CTRLLR_DPAD_U },
	{ SDL_CONTROLLER_BUTTON_DPAD_DOWN, INPUT_CTRLLR_DPAD_D },
	{ SDL_CONTROLLER_BUTTON_DPAD_LEFT, INPUT_CTRLLR_DPAD_L },
	{ SDL_CONTROLLER_BUTTON_DPAD_RIGHT, INPUT_CTRLLR_DPAD_R },
};

struct input_t {
	s8 key[INPUT_KEY_TOTAL];
	ivec2_t mousepos;
	fvec2_t control_left, control_right, control_trig;
	SDL_GameController *controller;
	s32 controllerid;
};

struct output_t {
	SDL_Window *window;
	f32 win_w, win_h;
	f32 aspectratio;
	s32 fullscreen;
};

struct quad_t {
	fvec3_t verts[4];
	fvec3_t color;
};

struct particle_t {
	f32 pos_x, pos_y, pos_z;
	f32 col_r, col_g, col_b, col_a;
	f32 cam_dist;
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

	f32 time_curr; // the current time in the simulation
	               // this value is always increasing while the simulation isn't
				   // in "paused" mode

	s64 timestep_last, timestep_curr;

	struct molt_cfg_t *config;

	// NOTE (brian) we read from storage into particles, and sort them
	// into particles_draw for actual drawing
	f64 *particles_raw;
	struct particle_t *particles;
	struct particle_t *particles_draw;
	u64 particle_cnt;

	f64 volmin, volmax;

	s32 run, paused;
};

void viewer_bounds(f64 *p, u64 len, f64 *low, f64 *high);

void viewer_eventaxis(SDL_Event *event, struct simstate_t *state);
void viewer_eventbutton(SDL_Event *event, struct simstate_t *state);
void viewer_eventmotion(SDL_Event *event, struct simstate_t *state);
void viewer_eventdevice(SDL_Event *event, struct simstate_t *state);
void viewer_eventwindow(SDL_Event *event, struct simstate_t *state);
void v_keystatecycle(struct simstate_t *state);

void v_handleinput(struct simstate_t *state);

void v_debuginfo(struct simstate_t *state, struct frender_t *frender, struct fchar_t *ftab, int len);

u32 viewer_mkshader(char *vertex, char *fragment);

// font loading functions
void f_fontload(char *path, s32 fontsize, struct fchar_t *ftab, s32 len);
s32 f_vertadvance(struct fchar_t *ftab, s32 len);
void f_rendertext(struct frender_t *frender, struct fchar_t *ftab, char *text);

int v_loadopenglfuncs();
int v_instatecheck(struct simstate_t *state, int kstate, int amt, ...);
int v_iteminlist(int v, int n, ...);
/* v_particles_raw_to_formed : reads raw f64 * and formats it to the particle_t structure we need to render */
void v_particles_raw_to_formed(struct simstate_t *state, struct molt_cfg_t *config, struct particle_t *dst, f64 *src);
void v_addpart(struct simstate_t *state, f32 x, f32 y, f32 z, f32 r, f32 g, f32 b, s32 u);
void v_updatestate(struct simstate_t *state);
/* v_minsandmaxes : finds the mins and maxes for all amplitude lumps */
void v_minsandmaxes(struct simstate_t *state, f64 *max, f64 *min);

/* mix_f64 : linear interpolation for f64 values */
f64 mix_f64(f64 x, f64 y, f64 a);

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
typedef void (APIENTRY * GL_DrawArrays_Func)(GLenum, GLint, GLsizei);
GL_DrawArrays_Func vglDrawArrays;
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

typedef void (APIENTRY * GL_Uniform1fv_Func)(GLint, GLsizei count, const GLfloat *value);
GL_Uniform1fv_Func vglUniform1fv;
typedef void (APIENTRY * GL_Uniform2fv_Func)(GLint, GLsizei count, const GLfloat *value);
GL_Uniform1fv_Func vglUniform2fv;
typedef void (APIENTRY * GL_Uniform3fv_Func)(GLint, GLsizei count, const GLfloat *value);
GL_Uniform1fv_Func vglUniform3fv;
typedef void (APIENTRY * GL_Uniform4fv_Func)(GLint, GLsizei count, const GLfloat *value);
GL_Uniform1fv_Func vglUniform4fv;

typedef void (APIENTRY * GL_Uniform1i_Func)(GLint location, GLint v0);
GL_Uniform1f_Func vglUniform1i;
typedef void (APIENTRY * GL_Uniform2i_Func)(GLint location, GLint v0, GLint v1);
GL_Uniform2f_Func vglUniform2i;
typedef void (APIENTRY * GL_Uniform3i_Func)(GLint location, GLint v0, GLint v1, GLint v2);
GL_Uniform3f_Func vglUniform3i;
typedef void (APIENTRY * GL_Uniform4i_Func)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
GL_Uniform4f_Func vglUniform4i;

// texturing functions
typedef void (APIENTRY * GL_GenTextures_Func)(GLsizei, GLuint *textures);
GL_GenTextures_Func vglGenTextures;
typedef void (APIENTRY * GL_BindTexture_Func)(GLsizei, GLuint target);
GL_BindTexture_Func vglBindTexture;
typedef void (APIENTRY * GL_TexImage2D_Func)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
GL_TexImage2D_Func vglTexImage2D;
typedef void (APIENTRY * GL_TexParameteri_Func)(GLenum, GLenum, GLuint);
GL_TexParameteri_Func vglTexParameteri;
typedef void (APIENTRY * GL_ActiveTexture_Func)(GLenum);
GL_ActiveTexture_Func vglActiveTexture;
typedef void (APIENTRY * GL_BufferSubData_Func)(GLenum, GLintptr, GLsizeiptr, const GLvoid *);
GL_BufferSubData_Func vglBufferSubData;


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
	OGLFUNCMK(glDrawArrays),
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
	OGLFUNCMK(glUniform1i),
	OGLFUNCMK(glUniform2i),
	OGLFUNCMK(glUniform3i),
	OGLFUNCMK(glUniform4i),
	OGLFUNCMK(glUniform1fv),
	OGLFUNCMK(glUniform2fv),
	OGLFUNCMK(glUniform3fv),
	OGLFUNCMK(glUniform4fv),
	OGLFUNCMK(glViewport),
	OGLFUNCMK(glGenTextures),
	OGLFUNCMK(glBindTexture),
	OGLFUNCMK(glTexImage2D),
	OGLFUNCMK(glTexParameteri),
	OGLFUNCMK(glActiveTexture),
	OGLFUNCMK(glBufferSubData)
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

/* v_run : runs the molt graphical 3d simulation viewer */
s32 v_run(void *hunk, u64 hunksize, s32 fd, struct molt_cfg_t *cfg)
{
	SDL_GLContext glcontext;
	SDL_Event event;

	struct molt_cfg_t config;
	struct simstate_t state;
	struct shader_cont_t shader;

	struct frender_t frender;
	struct fchar_t ftab[128];

	struct particle_t *p;
	s32 rc;
	s64 i, elements;
	ivec3_t dim;
	u32 locPos, locCol;

	float bbverts[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f
	};

	hmm_mat4 view, proj, pers;

	memset(&ftab, 0, sizeof ftab);
	memset(&state, 0, sizeof state);

	// TODO (brian) put the lump strings somewhere better (common.h)
	rc = lump_read("CONFIG", 0, &config);
	if (rc < 0) {
		fprintf(stderr, "Couldn't load config lump!\n");
		return -1;
	}

	molt_cfg_parampull_xyz(&config, dim, MOLT_PARAM_PINC);

	elements = (s64)dim[0] * dim[1] * dim[2];

	state.config = &config;

	state.particles_raw = calloc(1, sizeof(f64) * elements);
	state.particles = calloc(1, sizeof(struct particle_t) * elements);
	state.particles_draw = calloc(1, sizeof(struct particle_t) * elements);
	state.particle_cnt = elements;

	v_minsandmaxes(&state, &state.volmin, &state.volmax);

	state.timestep_curr = state.config->t_params[MOLT_PARAM_START];
	state.timestep_last = state.config->t_params[MOLT_PARAM_START];

	rc = lump_read("AMP", state.timestep_curr, state.particles_raw);
	if (rc < 0) {
		fprintf(stderr, "couldn't read AMP %lld from lump system!\n", state.timestep_curr);
		return -1;
	}

	// initialize the camera
	state.cam_pos = HMM_Vec3(0, 4, 4);
	state.cam_front = HMM_MultiplyVec3f(state.cam_pos, -1);
	state.cam_up = HMM_Vec3(0, 1, 0);

	v_particles_raw_to_formed(&state, &config, state.particles, state.particles_raw);

	rc = 0, state.run = 1;

	if (SDL_Init(VIEWER_SDL_INITFLAGS) != 0) {
		ERRLOG("Couldn't Init SDL", SDL_GetError());
		return -1;
	}

	state.output.win_w = WIDTH;
	state.output.win_h = HEIGHT;
	state.output.aspectratio = state.output.win_w / state.output.win_h;
	state.output.window = SDL_CreateWindow("MOLT Viewer", 64, 64, state.output.win_w, state.output.win_h, VIEWER_SDL_WINFLAGS);
	if (state.output.window == NULL) {
		ERRLOG("Couldn't Create Window", SDL_GetError());
		return -1;
	}

	state.input.controller = SDL_GameControllerOpen(0);
	if (!state.input.controller) {
		ERRLOG("Couldn't open Controller", SDL_GetError());
	}

	state.cam_x = WIDTH;
	state.cam_y = HEIGHT;
	state.cam_sens = MOUSE_SENSITIVITY;
	state.cam_yaw = -90;

	if (SDL_SetRelativeMouseMode(SDL_TRUE) == -1) {
		ERRLOG("Couldn't Set Relative Motion Mode", SDL_GetError());
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	glcontext = SDL_GL_CreateContext(state.output.window);

	v_loadopenglfuncs();

	if (SDL_GL_SetSwapInterval(0) != 0) {
		ERRLOG("Couldn't set the swap interval", SDL_GetError());
	}

	// glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// setup our particle shader
	shader.shader = viewer_mkshader("src/particle.vert", "src/particle.frag");
	shader.str = "particle_shader";

	vglGenVertexArrays(1, &shader.vao);
	vglGenBuffers(1, &shader.vbo);

	vglBindBuffer(GL_ARRAY_BUFFER, shader.vbo);
	vglBufferData(GL_ARRAY_BUFFER, sizeof(bbverts), bbverts, GL_STATIC_DRAW);

	vglBindVertexArray(shader.vao);
	vglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	vglEnableVertexAttribArray(0);

	// setup our font rendering and shader
	f_fontload(FONT_PATH, 16, ftab, ARRSIZE(ftab));
	frender.vertadvance = f_vertadvance(ftab, ARRSIZE(ftab));
	frender.shader = viewer_mkshader("src/font.vert", "src/font.frag");

	vglGenVertexArrays(1, &frender.vao);
	vglGenBuffers(1, &frender.vbo);

	vglBindVertexArray(frender.vao);

	vglBindBuffer(GL_ARRAY_BUFFER, frender.vbo);
	vglBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

	vglEnableVertexAttribArray(0);
	vglVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	vglBindBuffer(GL_ARRAY_BUFFER, 0);
	vglBindVertexArray(0);

	// setup uniform locations so we only have to do it once
	shader.loc_view  = vglGetUniformLocation(shader.shader, "uView");
	shader.loc_proj  = vglGetUniformLocation(shader.shader, "uProj");
	frender.loc_view = vglGetUniformLocation(frender.shader, "uView");
	frender.loc_pers = vglGetUniformLocation(frender.shader, "uPers");

	locPos = vglGetUniformLocation(shader.shader, "uPos");
	locCol = vglGetUniformLocation(shader.shader, "uCol");

	// now we can finally run our simulation
	while (state.run) {

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_CONTROLLERAXISMOTION:
				viewer_eventaxis(&event, &state);
				break;
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				// that's right, one function does all the buttons :)
				viewer_eventbutton(&event, &state);
				break;
			case SDL_MOUSEMOTION:
				viewer_eventmotion(&event, &state);
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

		v_handleinput(&state);
		v_keystatecycle(&state);

		// create updated transform matricies
		state.cam_look = HMM_AddVec3(state.cam_pos, state.cam_front);
		view = HMM_LookAt(state.cam_pos, state.cam_look, state.cam_up);
		proj = HMM_Perspective(90.0f, state.output.aspectratio, 0.1f, 40.0f);
		pers = HMM_Orthographic(0, state.output.win_w, 0, state.output.win_h, -1.0, 1.0);

		if (state.timestep_last != state.timestep_curr) {

			memset(state.particles_raw, 0, sizeof(f64) * elements);
			memset(state.particles, 0, sizeof(struct particle_t) * elements);
			memset(state.particles_draw, 0, sizeof(struct particle_t) * elements);

			rc = lump_read("AMP", state.timestep_curr, state.particles_raw);
			if (rc < 0) {
				fprintf(stderr, "couldn't read AMP %lld from lump system!\n", state.timestep_curr);
				return -1;
			}

			v_particles_raw_to_formed(&state, &config, state.particles, state.particles_raw);

			state.timestep_last = state.timestep_curr;
		}

		v_updatestate(&state);

		// render
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* draw the particles */
		vglUseProgram(shader.shader);

		vglBindVertexArray(shader.vao);

		vglUniformMatrix4fv(shader.loc_view, 1, GL_FALSE, (f32 *)&view);
		vglUniformMatrix4fv(shader.loc_proj, 1, GL_FALSE, (f32 *)&proj);

		p = state.particles_draw;
		for (i = 0; i < elements; i++) {
			vglUniform3f(locPos, p[i].pos_x, p[i].pos_y, p[i].pos_z);
			vglUniform4f(locCol, p[i].col_r, p[i].col_g, p[i].col_b, p[i].col_a);
			vglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}

		// DEBUG (brian) debugging font renderin
		vglUseProgram(frender.shader);
		vglBindVertexArray(frender.vao);

		vglUniformMatrix4fv(frender.loc_view, 1, GL_FALSE, (f32 *)&view);
		vglUniformMatrix4fv(frender.loc_pers, 1, GL_FALSE, (f32 *)&pers);

		v_debuginfo(&state, &frender, ftab, ARRSIZE(ftab));

		// redraw the screen
		SDL_GL_SwapWindow(state.output.window);

		vglBindVertexArray(0);
		vglUseProgram(0);

		// get time to draw frame, wait if needed
		state.frame_curr = ((f32)SDL_GetTicks()) / 1000.0f;
		state.frame_diff = state.frame_curr - state.frame_last;

		if (!state.paused) {
			state.time_curr += state.frame_diff;
		}

#if 0
		if (state.frame_diff < TARGET_FRAMETIME) {
			SDL_Delay(((f32)SDL_GetTicks()) / 1000 - state.frame_curr);
		}
#endif

		state.frame_last = state.frame_curr;
	}

	vglDeleteVertexArrays(1, &frender.vao);
	vglDeleteBuffers(1, &frender.vbo);
	vglDeleteVertexArrays(1, &shader.vao);
	vglDeleteBuffers(1, &shader.vbo);

	// clean up from our gl shenanigans
	vglDeleteProgram(shader.shader);
	vglDeleteProgram(frender.shader);

	SDL_GameControllerClose(state.input.controller);
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(state.output.window);
	SDL_Quit();

	free(state.particles_raw);
	free(state.particles);
	free(state.particles_draw);

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

/* viewer_eventdevice : event handler for a controller device updates */
void viewer_eventdevice(SDL_Event *event, struct simstate_t *state)
{
	// if we get that we get a controller that plugs in after
	// the program starts up, we just load it right on in
	switch (event->cdevice.type) {
		case SDL_CONTROLLERDEVICEADDED:
			state->input.controllerid = event->cdevice.which;
			state->input.controller = SDL_GameControllerOpen(state->input.controllerid);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			SDL_GameControllerClose(state->input.controller);
			state->input.controller = NULL;
			break;
		case SDL_CONTROLLERDEVICEREMAPPED:
			break;
	}
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
		ptr = &state->input.key[INPUT_MOUSE_LEFT];
		break;
	case SDL_BUTTON_RIGHT:
		ptr = &state->input.key[INPUT_MOUSE_RIGHT];
		break;
	case SDL_BUTTON_MIDDLE:
		ptr = &state->input.key[INPUT_MOUSE_CENTER];
		break;
	default:
		return;
	}

	if (event->button.state == SDL_PRESSED) {
		*ptr = INSTATE_PRESSED;
	} else {
		*ptr = INSTATE_RELEASED;
	}
}

/* viewer_getinputs : reads and toggles the internal keystate */
void viewer_eventbutton(SDL_Event *event, struct simstate_t *state)
{
	int start, end, bval, bstate, i;

	switch (event->type) {
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		start = INPUT_KEY__START;
		end = INPUT_KEY__DONE;
		bval = event->key.keysym.sym;
		bstate = event->key.state;
		break;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		start = INPUT_MOUSE__START;
		end = INPUT_MOUSE__DONE;
		bval = event->button.button;
		bstate = event->button.state;
		break;

	case SDL_CONTROLLERBUTTONDOWN:
	case SDL_CONTROLLERBUTTONUP:
		start = INPUT_CTRLLR__START;
		end = INPUT_CTRLLR__DONE;
		bval = event->cbutton.button;
		bstate = event->cbutton.state;
		break;
	default:
		fprintf(stderr, "Bad event type [%d] for viewer_eventbutton\n", event->type);
		return;
	}

	for (i = start; i < end; i++) {
		if (bval == sdlkey_to_inputkey[i].from) {
			if (bstate == SDL_PRESSED) {
				state->input.key[sdlkey_to_inputkey[i].to] = INSTATE_PRESSED;
			} else {
				state->input.key[sdlkey_to_inputkey[i].to] = INSTATE_RELEASED;
			}
			break;
		}
	}
}

/* v_keystatecycle : cycles the keystate array */
void v_keystatecycle(struct simstate_t *state)
{
	// bonus benefit to thinking that all buttons are in this keystate arr
	// is that this modifies the state for the mouse, gamepad, etc
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
	f32 win_w, win_h, ar;

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
		win_w = event->window.data1;
		win_h = event->window.data2;
		ar = win_w / win_h;

		state->output.win_w = win_w;
		state->output.win_h = win_h;
		state->output.aspectratio = ar;

		vglViewport(0, 0, state->output.win_w, state->output.win_h);
		break;
	case SDL_WINDOWEVENT_CLOSE:
		state->run = 0;
		break;
	default:
		break;
	}
}

/* v_handleinput : updates the gamestate from the */
void v_handleinput(struct simstate_t *state)
{
	f32 camera_speed;
	hmm_vec3 tmp;

#define IN_MOVEF     INPUT_KEY_W, INPUT_CTRLLR_DPAD_U
#define IN_MOVEB     INPUT_KEY_S, INPUT_CTRLLR_DPAD_D
#define IN_MOVEL     INPUT_KEY_A, INPUT_CTRLLR_DPAD_L
#define IN_MOVER     INPUT_KEY_D, INPUT_CTRLLR_DPAD_R
#define IN_MOVEU     INPUT_KEY_SPACE, INPUT_CTRLLR_RSHOULDER
#define IN_MOVED     INPUT_KEY_SHIFT, INPUT_CTRLLR_LSHOULDER
#define IN_QUIT      INPUT_KEY_ESC, INPUT_KEY_Q
#define IN_PAUSE     INPUT_KEY_J
#define IN_FSCREEN   INPUT_KEY_F
#define IN_NEXTTIME  INPUT_KEY_UARROW, INPUT_KEY_RARROW
#define IN_PREVTIME  INPUT_KEY_DARROW, INPUT_KEY_LARROW
#define IN_STARTTIME INPUT_KEY_HOME
#define IN_ENDTIME   INPUT_KEY_END

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

	if (v_instatecheck(state, INSTATE_PRESSED, PP_NARG(IN_PAUSE), IN_PAUSE)) {
		state->paused = !state->paused;
	}

	if (v_instatecheck(state, INSTATE_PRESSED, PP_NARG(IN_NEXTTIME), IN_NEXTTIME)) {
		if (state->timestep_curr != state->config->t_params[MOLT_PARAM_STOP]) {
			state->timestep_curr++;
		}
	}

	if (v_instatecheck(state, INSTATE_PRESSED, PP_NARG(IN_PREVTIME), IN_PREVTIME)) {
		if (state->timestep_curr != state->config->t_params[MOLT_PARAM_START]) {
			state->timestep_curr--;
		}
	}

	if (v_instatecheck(state, INSTATE_PRESSED, PP_NARG(IN_STARTTIME), IN_STARTTIME)) {
		state->timestep_curr = 0;
	}

	if (v_instatecheck(state, INSTATE_PRESSED, PP_NARG(IN_ENDTIME), IN_ENDTIME)) {
		state->timestep_curr = state->config->t_params[MOLT_PARAM_STOP];
	}

	if (v_instatecheck(state, INSTATE_PRESSED, PP_NARG(IN_FULLSCREEN), IN_FSCREEN)) {
		if (state->output.fullscreen) {
			SDL_SetWindowFullscreen(state->output.window, 0);
		} else {
			SDL_SetWindowFullscreen(state->output.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}

		state->output.fullscreen = !state->output.fullscreen;
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
	vertex = sys_readfile(vertex_file);
	fragment = sys_readfile(fragment_file);

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

/* vec_dist : compute the distance between hmm_vec3s */
f32 vec_dist(hmm_vec3 cam, hmm_vec3 part)
{
	return (float)sqrt(
			pow(cam.x - part.x, 2) +
			pow(cam.y - part.y, 2) +
			pow(cam.z - part.z, 2));
}

/* partcmp : particle_t comparator (for distance) */
int partcmp(const void *a, const void *b)
{
	struct particle_t *pa;
	struct particle_t *pb;
	int rc;
	f32 a_dist, b_dist;

	pa = (struct particle_t *)a;
	pb = (struct particle_t *)b;

	a_dist = pa->cam_dist;
	b_dist = pb->cam_dist;

	if (a_dist < b_dist) {
		rc = -1;
	} else if (a_dist > b_dist) {
		rc = 1;
	} else {
		rc = 0;
	}

	return rc;
}

/* v_particles_raw_to_formed : reads raw f64 * and formats it to the particle_t structure we need to render */
void v_particles_raw_to_formed(struct simstate_t *state, struct molt_cfg_t *config, struct particle_t *dst, f64 *src)
{
	dvec3_t ddim;
	ivec3_t curr, dim;
	s64 idx;

	molt_cfg_parampull_xyz(config, dim, MOLT_PARAM_PINC);

	for (curr[0] = 0; curr[0] < dim[0]; curr[0]++) {
		for (curr[1] = 0; curr[1] < dim[1]; curr[1]++) {
			for (curr[2] = 0; curr[2] < dim[2]; curr[2]++) {
				Vec3Scale(ddim, curr, 1.0f);

				idx = IDX3D(curr[0], curr[1], curr[2], dim[1], dim[2]);

				dst[idx].pos_x = ddim[0];
				dst[idx].pos_y = ddim[1];
				dst[idx].pos_z = ddim[2];
				dst[idx].col_r = 1.0f;
				dst[idx].col_g = 0.0f;
				dst[idx].col_b = 1.0f;
				dst[idx].col_a = (src[idx] + -state->volmin) / (state->volmax + -state->volmin);
			}
		}
	}
}

/* v_updatestate : updates the particle system for drawing */
void v_updatestate(struct simstate_t *state)
{
	// NOTE (brian) eventually, this will be replaced with a function that
	// reads from a molt_cfg_t, but as the simulation isn't quite perfect,
	// this real-time computed solution will have to do

	struct particle_t *part_r;
	struct particle_t *part_w;
	hmm_vec3 pvec;
	u64 i;

	part_r = state->particles;
	part_w = state->particles_draw;

	for (i = 0; i < state->particle_cnt; i++) {
		pvec = HMM_Vec3(part_r[i].pos_x, part_r[i].pos_y, part_r[i].pos_z);
		part_r[i].cam_dist = vec_dist(state->cam_pos, pvec);
	}

	memcpy(part_w, part_r, state->particle_cnt * sizeof(struct particle_t));

	// qsort(part_w, state->particle_cnt, sizeof(struct particle_t), partcmp);
}

/* v_minsandmaxes : finds the mins and maxes for all amplitude lumps */
void v_minsandmaxes(struct simstate_t *state, f64 *max, f64 *min)
{
	f64 *hunk;
	f64 lmax, lmin;
	u64 i, entries, idx;
	int rc;
	ivec3_t dim;
	ivec3_t curr;

	molt_cfg_parampull_xyz(state->config, dim, MOLT_PARAM_PINC);

	hunk = calloc(dim[0] * (u64)dim[1] * dim[2], sizeof(f64));
	lmax = DBL_MIN;
	lmin = DBL_MAX;

	rc = lump_getnumentries("AMP", &entries);

	for (i = 0; i < entries; i++) {
		rc = lump_read("AMP", i, hunk);

		for (curr[0] = 0; curr[0] < dim[0]; curr[0]++) {
			for (curr[1] = 0; curr[1] < dim[1]; curr[1]++) {
				for (curr[2] = 0; curr[2] < dim[2]; curr[2]++) {
					idx = IDX3D(curr[0], curr[1], curr[2], dim[1], dim[2]);
					if (hunk[idx] < lmin) {
						lmin = hunk[idx];
					}
					if (hunk[idx] > lmax) {
						lmax = hunk[idx];
					}
				}
			}
		}
	}

	if (max) {
		*max = lmax;
	}
	if (min) {
		*min = lmin;
	}

	free(hunk);
}

/* v_debuginfo : draws debugging information to the screen */
void v_debuginfo(struct simstate_t *state, struct frender_t *frender, struct fchar_t *ftab, int len)
{
	s32 i;
	s64 val;
	f64 scale;
	f32 x, y;
	char buf[BUFSMALL];

	Vec4Set(frender->color, 1, 1, 1, 1);

	// NOTE (brian) we begin drawing from the top left of the screen
	x = 0;
	y = state->output.win_h - frender->vertadvance;
	Vec2Set(frender->pos, x, y);

	// debug header
	snprintf(buf, sizeof buf, "MOLT Viewer DEBUG\n");
	f_rendertext(frender, ftab, buf);

	// player position
	snprintf(buf, sizeof buf, "Pos       (%3.4f,%3.4f,%3.4f)\n", state->cam_pos.x, state->cam_pos.y, state->cam_pos.z);
	f_rendertext(frender, ftab, buf);

	// player position
	snprintf(buf, sizeof buf, "Front     (%3.4f,%3.4f,%3.4f)\n", state->cam_front.x, state->cam_front.y, state->cam_front.z);
	f_rendertext(frender, ftab, buf);

	scale = state->config->time_scale;

	// TODO (brian) add back the time debug info
	val = state->config->t_params[MOLT_PARAM_START];
	snprintf(buf, sizeof buf, "TimeStart %lld (%3.4e)\n", val, val * scale);
	f_rendertext(frender, ftab, buf);

	val = state->timestep_curr;
	snprintf(buf, sizeof buf, "TimeCurr  %lld (%3.4e)\n", val, val * scale);
	f_rendertext(frender, ftab, buf);

	val = state->config->t_params[MOLT_PARAM_STOP];
	snprintf(buf, sizeof buf, "TimeEnd   %lld (%3.4e)\n", val, val * scale);
	f_rendertext(frender, ftab, buf);

	snprintf(buf, sizeof buf, "Min (%3.4e), Max (%3.4e)\n", state->volmin, state->volmax);
	f_rendertext(frender, ftab, buf);
}

/* f_rendertext : renders text to the screen at the frender's position with the frender's color */
void f_rendertext(struct frender_t *frender, struct fchar_t *ftab, char *text)
{
	f32 x, y;
	f32 xpos, ypos, w, h;
	s32 i, c;
	u32 tmp;

	vglUseProgram(frender->shader);

	tmp = vglGetUniformLocation(frender->shader, "uColor");
	vglUniform4fv(tmp, 1, frender->color);

	vglActiveTexture(GL_TEXTURE0);
	vglBindVertexArray(frender->vao);

	x = frender->pos[0];
	y = frender->pos[1];

	for (i = 0; i < strlen(text); i++) {
		// here, we check if we have a unix newline (0x10), and advance the
		// cursor by the vertical advance amount if we have one, then continue
		if (text[i] == '\n') {
			frender->pos[1] -= frender->vertadvance;
			x = frender->pos[0];
			y = frender->pos[1];
			continue;
		}

		c = text[i];

		xpos = x + ftab[c].b_x;
		ypos = y - (ftab[c].f_y + ftab[c].b_y);

		w = ftab[c].f_x;
		h = ftab[c].f_y;

		// create new triangles to render our quads
#if 1
		f32 verts[6][4] = {
			{ xpos    , ypos + h, 0.0, 0.0 },
			{ xpos + w, ypos    , 1.0, 1.0 },
			{ xpos    , ypos    , 0.0, 1.0 },

			{ xpos    , ypos + h, 0.0, 0.0 },
			{ xpos + w, ypos + h, 1.0, 0.0 },
			{ xpos + w, ypos    , 1.0, 1.0 },
		};
#else
		f32 verts[6][4] = {
			{ xpos    , ypos - h, 0.0, 1.0 },
			{ xpos + w, ypos    , 1.0, 0.0 },
			{ xpos    , ypos    , 0.0, 0.0 },

			{ xpos    , ypos - h, 0.0, 1.0 },
			{ xpos + w, ypos - h, 1.0, 1.0 },
			{ xpos + w, ypos    , 1.0, 0.0 },
		};
#endif

		// render the glyph texture over our quad
		vglBindTexture(GL_TEXTURE_2D, ftab[c].texture);

		// update the contents of vbo memory
		vglBindBuffer(GL_ARRAY_BUFFER, frender->vbo);
		vglBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

		vglDrawArrays(GL_TRIANGLES, 0, 6);

		vglBindBuffer(GL_ARRAY_BUFFER, 0);

		x += ftab[c].advance;
	}

	vglBindVertexArray(0);
	vglBindTexture(GL_TEXTURE_2D, 0);
}

/* f_vertadvance : returns the desired font vertical advance value */
s32 f_vertadvance(struct fchar_t *ftab, s32 len)
{
	s32 i, ret;

	for (i = 0, ret = 0; i < len; i++) {
		if (ret < ftab[i].f_y) {
			ret = ftab[i].f_y;
		}
	}

	return ret;
}

/* f_fontload : load as many ascii characters into the font table as possible */
void f_fontload(char *path, s32 fontsize, struct fchar_t *ftab, s32 len)
{
	stbtt_fontinfo fontinfo;
	f32 scale_x, scale_y;
	s32 i, w, h, xoff, yoff, advance, lsb;
	unsigned char *ttf_buffer, *bitmap;
	u32 tex;

	ttf_buffer = (unsigned char *)sys_readfile(path);

	if (!ttf_buffer) {
		fprintf(stderr, "Couldn't read fontfile [%s]\n", path);
		exit(1);
	}

	stbtt_InitFont(&fontinfo, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));
	scale_y = stbtt_ScaleForPixelHeight(&fontinfo, fontsize);
	scale_x = scale_y;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (i = 0; i < len; i++) {
		bitmap = stbtt_GetCodepointBitmap(&fontinfo, scale_x, scale_y, i, &w, &h, &xoff, &yoff);

		stbtt_GetCodepointHMetrics(&fontinfo, i, &advance, &lsb);

		if (bitmap) {
			vglGenTextures(1, &tex);
			vglBindTexture(GL_TEXTURE_2D, tex);
			vglTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

			// setup the texture options
			vglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			vglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			vglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			vglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		} else {
			tex = -1;
		}

		ftab[i].texture = tex;
		ftab[i].f_x     = w;
		ftab[i].f_y     = h;
		ftab[i].b_x     = xoff;
		ftab[i].b_y     = yoff;
		ftab[i].advance = advance * scale_x;

		free(bitmap);
	}

	vglBindTexture(GL_TEXTURE_2D, 0);

	free(ttf_buffer);
}

/* mix_f64 : linear interpolation for f64 values */
f64 mix_f64(f64 x, f64 y, f64 a)
{
	return x * (1 - a) + y * a;
}

