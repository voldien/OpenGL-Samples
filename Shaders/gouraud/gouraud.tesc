#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable

// define the number of CPs in the output patch
layout(vertices = 3) out;

layout(location = 0) in vec3 WorldPos_CS_in[];
layout(location = 1) in vec3 Normal_CS_in[];

// attributes of the output CPs
layout(location = 0) out vec3 WorldPos_ES_in[3];
layout(location = 1) out vec3 Normal_ES_in[3];

#include "gouraud_base.glsl"


float GetTessLevel(float Distance0, float Distance1) {
	float AvgDistance = (Distance0 + Distance1) / 2.0;

	const float maxTessellation = 20.0;
	const float minTessellation = 0.04;

	return mix(minTessellation, maxTessellation, 100 / (AvgDistance + 10));
}

void main() {

	WorldPos_ES_in[gl_InvocationID] = WorldPos_CS_in[gl_InvocationID];
	Normal_ES_in[gl_InvocationID] = Normal_CS_in[gl_InvocationID];

	/*	Calculate the distance from the camera to the three control points	*/
	float EyeToVertexDistance0 = distance(ubo.gEyeWorldPos.xyz, WorldPos_ES_in[0]);
	float EyeToVertexDistance1 = distance(ubo.gEyeWorldPos.xyz, WorldPos_ES_in[1]);
	float EyeToVertexDistance2 = distance(ubo.gEyeWorldPos.xyz, WorldPos_ES_in[2]);

	/*	Calculate the tessellation levels	*/
	gl_TessLevelOuter[0] = GetTessLevel(EyeToVertexDistance1, EyeToVertexDistance2) * ubo.tessLevel;
	gl_TessLevelOuter[1] = GetTessLevel(EyeToVertexDistance2, EyeToVertexDistance0) * ubo.tessLevel;
	gl_TessLevelOuter[2] = GetTessLevel(EyeToVertexDistance0, EyeToVertexDistance1) * ubo.tessLevel;
	gl_TessLevelInner[0] = gl_TessLevelOuter[2];

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}