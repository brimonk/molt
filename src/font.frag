/*
 * Brian Chrzanowski
 * Fri Jul 26, 2019 23:16
 *
 * Font Fragment Shader
 */

#version 330

in  vec2 fTexCoords;
out vec4 oColor;

uniform sampler2D uText;
uniform vec4 uColor;

void main()
{
	vec4 sampled;

   	sampled = texture(uText, fTexCoords);
	oColor = uColor * vec4(1.0, 1.0, 1.0, sampled.r);
	// oColor = uColor;
}

