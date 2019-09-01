/*
 * Brian Chrzanowski
 * Thu Aug 22, 2019 20:56
 *
 * Particle System Frag Shader
 *
 * Shader lineraly interpolates between colorLo and colorHi based on fMag
 * (input magnitude from whatever function gets executed)
 */

#version 330

in float fMag;

out vec4 oColor;

vec3 colorLo = vec3(0, 0, 1);
vec3 colorHi = vec3(1, 0, 0);

void main()
{
	vec3 tmpColor;

	tmpColor = vec3(0.0);
	tmpColor = mix(colorLo, colorHi, fMag);

	oColor = vec4(tmpColor, 1);
}

