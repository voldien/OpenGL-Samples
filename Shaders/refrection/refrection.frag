#version 460
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
	vec4 lookDirection;
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	vec4 cameraPosition;

	float IOR;
}
ubo;

vec2 inverse_equirectangular(const in vec3 direction) {
	const vec2 invAtan = vec2(0.1591, 0.3183);
	vec2 uv = vec2(atan(direction.z, direction.x), asin(direction.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

void main() {

	const vec3 viewDir = normalize(ubo.cameraPosition.xyz - vertex);
	const vec3 N = normalize(normal);

	/*	*/
	const vec3 halfwayDir = normalize(normalize(ubo.direction.xyz) + viewDir);
	const float spec = pow(max(dot(N, halfwayDir), 0.0), 16.0);

	/*	*/
	const float contribution = max(0.0, dot(N, normalize(ubo.direction.xyz)));

	/*	*/
	const vec3 refrectionDir = normalize(refract(viewDir, N, ubo.IOR));

	/*	*/
	const vec2 reflection_uv = inverse_equirectangular(refrectionDir);

	float fresnelBias = 0.1;
	float fresnelScale = 0.1;
	float fresnelPower = 2;

	// const float refFactor = fresnelBias + fresnelScale * pow(viewDir + dot(viewDir, N), fresnelPower);

	/*	*/
	fragColor = texture(ReflectionTexture, reflection_uv) * (ubo.ambientColor + spec * contribution + contribution * ubo.lightColor);
}
