/*
 * Brian Chrzanowski
 * Sat Jul 20, 2019 13:48
 *
 * MOLT Viewer Vertex Shader
 *
 * Implements a simple particle system for each grid position
 */

#version 400

layout (location = 0) in vec3 iVert;
layout (location = 1) in vec4 iPart;

out vec4 vColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

uniform float uCLo;
uniform float uCHi;

void main()
{
	vec2 lColor;

	gl_Position = uProj * uView * uModel * vec4(iPart.x, iPart.y, iPart.z, 1.0f);

	lColor.x = mix(uCLo, uCHi, iPart.w);
	lColor.y = mix(uCHi, uCLo, iPart.w);

	vColor = vec4(lColor.x, lColor.y, 0, 1);
}

