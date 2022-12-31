#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

layout(binding = 0) uniform sampler2D ReflectionTexture;
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
	vec3 cameraPosition;

	float IOR;
}
ubo;

vec2 inverse_equirectangular(vec3 direction) {
	const vec2 invAtan = vec2(0.1591, 0.3183);
	vec2 uv = vec2(atan(direction.z, direction.x), asin(direction.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

void main() {

	vec3 viewDir = normalize(ubo.cameraPosition.xyz - vertex);

	vec3 halfwayDir = normalize(ubo.direction.xyz + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), 8);

	float contribution = max(0.0, dot(normal, ubo.direction.xyz));

	vec3 refrectionDir = refract(viewDir, normal, 1.333);

	vec2 reflection_uv = inverse_equirectangular(refrectionDir);

	fragColor = texture(ReflectionTexture, reflection_uv) * (ubo.ambientColor + spec);
}
