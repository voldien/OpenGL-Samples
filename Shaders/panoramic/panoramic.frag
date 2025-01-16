#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 ViewDir;

#include "common.glsl"
#include "phongblinn.glsl"
#include "scene.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProjection[6];
	mat4 modelViewProjection;

	/*	Light source.	*/
	DirectionalLight directional;
	Camera camera;

	/*	Material settings*/
	vec4 specularColor;
	vec4 ambientColor;

	vec4 shininess;
}
ubo;

void main() {

	const vec3 viewDir = normalize(ubo.camera.position.xyz - vertex);

	vec4 lightColor = computePhongDirectional(ubo.directional, -normalize(normal), viewDir, ubo.shininess.r, ubo.specularColor.rgb);

	/*	*/
	const vec2 irradiance_uv = inverse_equirectangular(normalize(normal));
	const vec4 irradiance_color = texture(IrradianceTexture, irradiance_uv).rgba;

	/*	*/
	fragColor = texture(DiffuseTexture, uv) * (ubo.ambientColor * irradiance_color + lightColor);
}