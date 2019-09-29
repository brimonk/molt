/*
 * Brian Chrzanowski
 * Thu Aug 22, 2019 20:56
 *
 * Particle System Fragment Shader
 *
 * Shader lineraly interpolates between colorLo and colorHi based on fMag
 * (input magnitude from whatever function gets executed)
 */

#version 330

in  vec4 fColor;
out vec4 oColor;

void main()
{
	oColor = fColor;
}

