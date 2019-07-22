/*
 * Brian Chrzanowski
 * Sat Jul 20, 2019 13:51
 *
 * MOLT Viewer Fragment Shader
 *
 * Shader applies the color given to the vertex shader
 */

#version 330

in vec3 vColor;

out vec4 oColor;

void main()
{
	oColor = vec4(vColor, 1.0f);
}

