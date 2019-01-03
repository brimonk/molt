/*
 * Brian Chrzanowski
 * Tue Jan 01, 2019 12:25 
 *
 * "OpenCL" MOLT Module
 * Read "softmolt.c" for more information on the structure of these modules.
 *
 * GPU (or computing device) work it a little different than the CPU bound
 * softmolt implementation. Quick recap about how softmolt "works".
 *
 * 1. main() sets up initial problem state
 * 2. foreach timestep, softmolt's doslice function is called
 * 3. after every iteration, the output is stored
 *
 * However, the software version only has one memory space to work from. Ideally
 * we want to avoid as many unnecessary memcpys as possible. To optimize for
 * performance, when the problem is starting up (first call to doslice from
 * molt_run), we copy the problem state to the GPU, and fail if we can't. On
 * subsequent calls to doslice, we can skip the initial copy phase, and 
 */

#include <stdio.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "../structures.h"
#include "../vect.h"

#define GPU_ENTRY_FUNC "doslice"

int module_notinit = 0;

int oclmolt_init(struct molt_t *molt);

/* oclmoltdevice_init : try to setup opencl and get device; fail if we can't */
__attribute__((constructor))
int oclmoltdevice_init()
{
	/*
	 * 1. Get Platform Information
	 * 2. Choose a preferred platform
	 * 3. Load and compile the OpenCL kernel
	 * 4. 
	 */
	cl_int a;
	cl_platform_id plats[8];
	cl_uint numplats;
	char buf[64];

	a = clGetPlatformIDs(8, plats, &numplats);

	printf("number of platforms %d\n", numplats);
	printf("number of platforms %d\n", numplats);

	a = clGetPlatformInfo(NULL, CL_PLATFORM_VERSION, sizeof(buf), buf, NULL);

	printf("%s\n", buf);

	return (int)a;
}

/* oclmolt_free : return and safely close all of the OpenCL related resources */
__attribute__((destructor))
int oclmoltdevice_free()
{
	return 0;
}

/* doslice : entry function for the module, bootstrapping from initial state */
int doslice(struct molt_t *molt)
{
	if (module_notinit) {
		module_notinit = 1;

		oclmolt_init(molt);
	}

	return 0;
}

/* oclmolt_init : */
int oclmolt_init(struct molt_t *molt)
{
	return 0;
}

