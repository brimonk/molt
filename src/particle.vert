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

layout (location = 0) in vec2 iPart;

out float fMag;

uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uModel;
uniform vec4 uPos;
uniform float radius;

void main()
{
	vec3 cameraRight, cameraUp;
	vec3 newPos;
	vec2 scaled;

	fMag = uPos.w;

	cameraRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
	cameraUp = vec3(uView[0][1], uView[1][1], uView[2][1]);

	newPos = uPos.xyz;
	newPos += cameraRight * iPart.x * 0.1;
	newPos += cameraUp * iPart.y * 0.1;

	// gl_Position = uProj * uView * translation * vec4(scaled, 0.0f, 1.0f);
	gl_Position = uProj * uView * uModel * vec4(newPos, 1.0f);
}

