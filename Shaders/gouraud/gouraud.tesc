#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(vertices = 1) out;

layout(location = 0) in vec3 WorldPos_CS_in[];
layout(location = 2) in vec3 Normal_CS_in[];

// attributes of the output CPs
layout(location = 0) out vec3 WorldPos_ES_in[1];
layout(location = 2) out vec3 Normal_ES_in[1];

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	vec4 gEyeWorldPos;
	float tessLevel;
}
ubo;

struct OutputPatch {
	vec3 WorldPos_B030;
	vec3 WorldPos_B021;
	vec3 WorldPos_B012;
	vec3 WorldPos_B003;
	vec3 WorldPos_B102;
	vec3 WorldPos_B201;
	vec3 WorldPos_B300;
	vec3 WorldPos_B210;
	vec3 WorldPos_B120;
	vec3 WorldPos_B111;
	vec3 Normal[3];
	vec2 TexCoord[3];
};

layout(location = 3) out patch OutputPatch oPatch;

float GetTessLevel(float Distance0, float Distance1) {
	float AvgDistance = (Distance0 + Distance1) / 2.0;

	// TODO lerp.
	if (AvgDistance <= 50.0) {
		return 20.0;
	} else if (AvgDistance <= 100.0) {
		return 15.0;
	} else if (AvgDistance <= 200.0) {
		return 10.0;
	} else {
		return 4.0;
	}
}

void main() {

	/*	Set the control points of the output patch	*/
	WorldPos_ES_in[gl_InvocationID] = WorldPos_CS_in[gl_InvocationID];
	Normal_ES_in[gl_InvocationID] = Normal_CS_in[gl_InvocationID];

	/*	Calculate the distance from the camera to the three control points	*/
	float EyeToVertexDistance0 = distance(ubo.gEyeWorldPos.xyz, WorldPos_ES_in[0]);
	// float EyeToVertexDistance1 = distance(ubo.gEyeWorldPos.xyz, WorldPos_ES_in[1]);
	// float EyeToVertexDistance2 = distance(ubo.gEyeWorldPos.xyz, WorldPos_ES_in[2]);

	/*	Calculate the tessellation levels	*/
	// gl_TessLevelOuter[0] = GetTessLevel(EyeToVertexDistance1, EyeToVertexDistance2) * ubo.tessLevel;
	// gl_TessLevelOuter[1] = GetTessLevel(EyeToVertexDistance2, EyeToVertexDistance0) * ubo.tessLevel;
	// gl_TessLevelOuter[2] = GetTessLevel(EyeToVertexDistance0, EyeToVertexDistance1) * ubo.tessLevel;
	// gl_TessLevelInner[0] = gl_TessLevelOuter[2];

	// gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}