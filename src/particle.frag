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

in float fMag;

out vec4 oColor;

void main()
{
	float alpha;

	alpha = mix(-1.0, 1.0, fMag);
	oColor = vec4(1.0f, 0.0f, 0.0f, alpha);
}

