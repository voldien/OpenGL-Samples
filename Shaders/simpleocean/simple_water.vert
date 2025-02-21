#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out vec3 vertex;
layout(location = 1) out vec2 UV;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 tangent;
layout(location = 4) out vec3 bitangent;

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
	vec4 ambientColor;
	vec4 diffuseColor;
	vec4 specularColor;
	vec4 shininess;

	/*	Light source.	*/
	DirectionalLight directional;

	/*	Tessellation Settings.	*/
	vec4 gEyeWorldPos;
	float gDispFactor;
	float tessLevel;

	float maxTessellation;
	float minTessellation;
}
ubo;

void main() {

	gl_Position = (ubo.modelViewProjection * vec4(Vertex, 1.0));

	vertex = (ubo.model * vec4(Vertex, 0.0)).xyz;
	normal = normalize((ubo.model * vec4(Normal, 0.0)).xyz);
	tangent = (ubo.model * vec4(Tangent, 0.0)).xyz;
	bitangent = cross(tangent, normal);
	UV = TextureCoord * 1500;
}