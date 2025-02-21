#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

#include "common.glsl"
#include "phongblinn.glsl"

layout(set = 0, binding = 1) uniform sampler2D NormalTexture;
layout(set = 0, binding = 0) uniform sampler2D ReflectionTexture;
layout(set = 0, binding = 11) uniform sampler2D DepthTexture;

struct Terrain {
	ivec2 size;
	vec2 tileOffset;
	vec2 tile_noise_size;
	vec2 tile_noise_offset;
};

struct WaterSettings {
	vec4 bottomOceanColor;
	vec4 surfaceOceanColor;
	float depth;
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

	/*	Convert normal map texture to a vector.	*/
	vec3 NormalMapBump = 2.0 * texture(NormalTexture, UV).xyz - vec3(1.0, 1.0, 1.0);
	/*	Scale non forward axis.	*/
	NormalMapBump.xy *= 0.1;
	/*	Compute the new normal vector on the specific surface normal.	*/
	const vec3 alteredNormal = normalize(mat3(tangent, bitangent, normal) * NormalMapBump);

	const vec4 lightColor =
		computeBlinnDirectional(ubo.directional, alteredNormal, viewDir.xyz, ubo.shininess.r, ubo.specularColor.rgb);

	/*	*/
	const vec3 reflection = normalize(reflect(-viewDir, alteredNormal));
	const vec2 reflection_uv = inverse_equirectangular(reflection);

	/*	*/
	const float blend_water_depth = 1.0075;

	/*	Compute depth difference	*/
	const vec2 texSize = textureSize(DepthTexture, 0).xy;
	const vec2 screen_uv = gl_FragCoord.xy / texSize;
	const float camera_z = getExpToLinear(ubo.camera.near, ubo.camera.far, texture(DepthTexture, screen_uv).r);
	const float shader_z = getExpToLinear(ubo.camera.near, ubo.camera.far, gl_FragCoord.z) - (1 / blend_water_depth);

	/*	*/
	const float diff_depth = camera_z - shader_z; //, 0;

	float translucent = pow(diff_depth, 32);
	translucent = clamp(translucent, 0, 1);

	const float foam = step(diff_depth, 0.9);

	/*	*/
	const vec4 bottomOceanColor = vec4(0.00667, 0.00784, 0.00941, 0.8);
	const vec4 surfaceOceanColor = vec4(0.8902, 0.9176, 1.0, 1);

	vec4 mixColor = mix(bottomOceanColor, surfaceOceanColor, translucent);
	mixColor.rgb += vec3(foam);

	const vec4 reflective_color = texture(ReflectionTexture, reflection_uv);

	/*	*/
	fragColor = (lightColor + ubo.ambientColor) * mixColor * ubo.diffuseColor * reflective_color;
	fragColor.a = mixColor.a;
	// fragColor = vec4(translucent);
	// fragColor.a = 1;
}