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
layout(set = 0, binding = 11) uniform sampler2D DepthTexture;

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

/*	Works, but can get cause mosaic, because of low sampling for far away objects.	*/
vec4 bump2(const in float height, const in float dist) {

	const float x = dFdxFine(height) * dist;
	const float y = dFdxFine(height) * dist;

	const vec4 normal = vec4(x, y, 1, 0);

	return normalize(normal);
}

void main() {

	/*	*/
	const vec3 viewDir = normalize(ubo.camera.position.xyz - vertex);

	/*	*/
	const float height = noise(vertex);
	const vec3 heightNormal = normalize((ubo.model * bump2(height, 0.1)).xyz);

	/*	*/
	const vec4 lightColor =
		computePhongDirectional(ubo.directional, heightNormal.xyz, viewDir.xyz, ubo.shininess.r, ubo.specularColor.rgb);

	/*	*/
	const vec3 reflection = normalize(reflect(-viewDir, heightNormal));
	const vec2 reflection_uv = inverse_equirectangular(reflection);

	/*	Compute depth difference	*/
	const vec2 texSize = textureSize(DepthTexture, 0).xy;
	const vec2 screen_uv = gl_FragCoord.xy / texSize;
	const float current_z = getExpToLinear(ubo.camera.near, ubo.camera.far, texture(DepthTexture, screen_uv).r);
	const float shader_z = getExpToLinear(ubo.camera.near, ubo.camera.far, gl_FragCoord.z);

	/*	*/
	const float diff_depth = abs(shader_z - current_z); //, 0;

	const float blend_water_depth = 0.050; // ubo.shininess.x;
	float translucent = clamp(diff_depth / blend_water_depth, 0, 1);
	translucent = 1 - translucent;
	translucent = translucent * translucent / (2 * (translucent * translucent - translucent) + 1);

	/*	*/
	const vec4 bottomOceanColor = vec4(0.2196, 0.2471, 0.2941, 1.0);
	const vec4 surfaceOceanColor = vec4(0.8902, 0.9176, 1.0, 1.0);

	const vec4 mixColor = mix(bottomOceanColor, surfaceOceanColor, 1 - translucent);

	fragColor = (lightColor + ubo.ambientColor) * mixColor * texture(ReflectionTexture, reflection_uv);
	fragColor.a = 1 - translucent;
}