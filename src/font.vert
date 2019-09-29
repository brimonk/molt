/*
 * Brian Chrzanowski
 * Fri Jul 26, 2019 23:11
 *
 * Font Vertex Shader
 */

#version 330

layout (location = 0) in vec4 vVert;

out vec2 fTexCoords;

uniform mat4 uPers;
uniform mat4 uView;

void main()
{
	gl_Position = uPers * vec4(vVert.xy, 0.0, 1.0);
	fTexCoords = vVert.zw;
}

