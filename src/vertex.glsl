/*
 * Brian Chrzanowski
 * Sat Jul 20, 2019 13:48
 *
 * MOLT Viewer Vertex Shader
 *
 * As of now, the main goal is just to translate from our CPU stored verticies
 * into our GPU stored verticies. Shortly here, we're probably going to want
 * to actually just call this program with CPU-based translation matricies.
 */

#version 330

layout (location = 0) in vec3 iPos;
layout (location = 1) in vec3 iColor;

out vec3 vColor;

void main()
{
	// just translating from CPU to GPU, effectively
	gl_Position = vec4(iPos, 1.0f);
	vColor = iColor;
}

