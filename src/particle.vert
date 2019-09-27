/*
 * Brian Chrzanowski
 * Thu Aug 22, 2019 18:56
 *
 * Particle System Vertex Shader
 *
 * This is a billboarding shader that follows some instructions from here:
 * http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/
 */

#version 330

layout (location = 0) in vec3 iPart;

out vec3 fColor;

uniform vec3 uPos; // position
uniform vec3 uCol; // color

uniform ivec2 uRes;
uniform mat4  uView;
uniform mat4  uProj;

void main()
{
	vec3 cameraRight, cameraUp;
	vec3 newPos;

	cameraRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
	cameraUp = vec3(uView[0][1], uView[1][1], uView[2][1]);

	newPos = uPos.xyz +
		cameraRight * iPart.x * 0.1 +
		cameraUp    * iPart.y * 0.1;

	gl_Position = uProj * uView * vec4(newPos, 1.0f);

	// pass color through to the fragment shader
	fColor = uCol;
}

