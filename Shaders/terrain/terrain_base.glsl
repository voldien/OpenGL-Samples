#include "common.glsl"
#include "phongblinn.glsl"

struct Terrain {
	ivec2 size;
	vec2 tileOffset;
	vec2 tile_noise_size;
	vec2 tile_noise_offset;
};

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 viewProjection;
	mat4 modelViewProjection;

	Camera camera;

	Terrain terrain;

	/*	Material	*/
	vec4 diffuseColor;
	vec4 ambientColor;
	vec4 specularColor;
	vec4 shininess;

	/*	Light source.	*/
	DirectionalLight directional;

	FogSettings fogSettings;

	/*	Tessellation Settings.	*/
	vec4 gEyeWorldPos;
	float gDispFactor;
	float tessLevel;

	float maxTessellation;
	float minTessellation;
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
	vec3 Tangent[3];
	vec2 TexCoord[3];
};