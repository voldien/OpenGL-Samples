#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(triangles, equal_spacing, ccw) in;

layout(location = 0) in vec3 WorldPos_ES_in[];
layout(location = 1) in vec3 Normal_ES_in[];

layout(location = 0) out vec4 LightColor_FS_in;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Material	*/
	vec4 diffuseColor;

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

layout(location = 4) in patch OutputPatch oPatch;

vec2 interpolate2D(const in vec2 v0, const in vec2 v1, const in vec2 v2) {
	return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate3D(const in vec3 v0, const in vec3 v1, const in vec3 v2) {
	return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

float computeLightContributionFactor(in vec3 direction, in vec3 normalInput) {
	return max(0.0, dot(-normalInput, direction));
}

void main() {

	const vec3 Normal_FS_in = normalize(interpolate3D(Normal_ES_in[0], Normal_ES_in[1], Normal_ES_in[2]));

	const vec3 WorldPos_FS_in = interpolate3D(WorldPos_ES_in[0], WorldPos_ES_in[1], WorldPos_ES_in[2]);

	LightColor_FS_in =
		ubo.diffuseColor * computeLightContributionFactor(ubo.direction.xyz, Normal_FS_in) * ubo.lightColor +
		ubo.ambientColor;

	/*	*/
	gl_Position = ubo.modelViewProjection * vec4(WorldPos_FS_in, 1.0);
}