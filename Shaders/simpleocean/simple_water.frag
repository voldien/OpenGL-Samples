#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

#include "common.glsl"
#include "phongblinn.glsl"

layout(set = 0, binding = 0) uniform sampler2D ReflectionTexture;
layout(set = 0, binding = 1) uniform sampler2D DepthTexture;

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

	// TODO: replace later.
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

vec4 bump2(const in float height, const in float dist) {

	const float x = dFdxFine(height) * dist;
	const float y = dFdxFine(height) * dist;

	const vec4 normal = vec4(x, y, 1, 0);

	return normalize(normal);
}

void main() {

	/*	*/
	const vec3 viewDir = normalize(ubo.camera.position.xyz - vertex);

	const float height = noise(vertex);
	const vec3 heightNormal = normalize((ubo.model * bump2(height, 0.1)).xyz);

	const vec4 lightColor =
		computePhongDirectional(ubo.directional, heightNormal.xyz, viewDir.xyz, ubo.shininess.r, ubo.specularColor.rgb);

	/*	Compute depth difference	*/
	const float current_z = getExpToLinear(ubo.camera.near, ubo.camera.far, texture(DepthTexture, gl_FragCoord.xy).r);
	const float shader_z = getExpToLinear(ubo.camera.near, ubo.camera.far, gl_FragCoord.z);

	float inter = clamp((shader_z - current_z) / 100.0, 0.2, 1);

	fragColor = (lightColor + ubo.ambientColor);
	fragColor.a = inter;
}