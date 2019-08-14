/*
 * Brian Chrzanowski
 * Tue Aug 13, 2019 23:09
 *
 * Vertex Shader for the world triangles.
 *
 * TODO
 * unify the Model, View and Projection names between shaders
 */

#version 330

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec3 vChecker;

out vec3 fPosition;
out vec3 fNormal;
out vec3 fColor;
out vec3 fChecker;
out vec3 fCoord;

void main()
{
	mat4 lTransposed = transpose(inverse(uView * uModel));
	mat3 lNorm = mat3(lTransposed[0], lTransposed[1], lTransposed[2]);

	fPosition = ((uModel * uView) * vec4(vPosition.xyz, 1.0)).xyz;
	fNormal = lNorm * vNormal;
	fColor = vColor;
	fCoord = vPosition.xyz;

	gl_Position = uProj * uView * uModel * vec4(vPosition.xyz, 1);
}

