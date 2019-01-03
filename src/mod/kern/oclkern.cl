/*
 * Brian Chrzanowski
 * Wed Jan 02, 2019 12:38
 *
 * An OpenCL Kernel for MOLT simulation
 */

#include "../../structures.h"

__kernel void main(__global int *A, __global int *B, __global int *C)
{
	int i;
	i = get_global_id(0);
	C[i] = A[i] + B[i];
}

