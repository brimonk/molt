/*
 * Brian Chrzanowski
 * Sat Jul 20, 2019 13:51
 *
 * MOLT Viewer Fragment Shader
 *
 * Shader applies the color given to the vertex shader
 */

#version 400

in vec4 vColor;

out vec4 oColor;

void main()
{
	oColor = vColor;
}

