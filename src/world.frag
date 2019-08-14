/*
 * Brian Chrzanowski
 * Tue Aug 13, 2019 23:09
 *
 * Fragment Shader for the world triangles.
 */

#version 330

uniform vec4 uLightPos;
uniform vec3 uAmbientLight;

in vec3 fPosition;
in vec3 fNormal;
in vec3 fColor;
in vec3 fChecker;
in vec3 fCoord;

out vec4 fOutput;

void main()
{
#if 0
	vec3 L = normalize(uLightPos.xyz - fPosition.xyz);
	vec3 N = normalize(fNormal);

	vec3 a = fColor;
	vec3 b = fColor * fChecker.z;

	vec3 c = step(fChecker.y, fract(fCoord / fChecker.x));
	float d = max(dot(L, N), 0.0);

	float k = c.x + c.z - 2.0 * c.x * c.z;

	fOutput = vec4(mix(a, b, k) * (d + AmbientLight), 1.0);
#endif

	fOutput = vec4(1.0, 0, 0, 1.0f);
}


