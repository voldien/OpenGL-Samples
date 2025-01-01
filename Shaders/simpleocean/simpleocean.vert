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

#include "common.glsl"
#include "light.glsl"

layout(constant_id = 10) const int MaxWaves = 128;

struct Wave {
	float wavelength;	/*	*/
	float amplitude;	/*	*/
	float speed;		/*	*/
	float rolling;		/*	*/
	vec2 direction;		/*	*/
	vec2 creast_offset; /*	*/
};

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	DirectionalLight directional;
	Camera camera;

	Wave waves[MaxWaves];
	int nrWaves;
	float time;
	float stepness;
	float rolling;

	/*	Material	*/
	vec4 oceanColor;
	vec4 specularColor;
	vec4 ambientColor;
	float shininess;
	float fresnelPower;
}
ubo;

float computeHeightStepness(const in float time, in const Wave w) {
	return 2 * pow(((w.amplitude * sin(dot(w.direction, Vertex.xy) * w.wavelength + time * w.speed)) * (1.0 / 2.0)),
				   w.rolling);
}

float computeHeight(const in float time, in const Wave w, const in vec2 UV) {
	return w.amplitude * sin(dot(w.direction, UV.xy) * w.wavelength + time * w.speed);
}

vec3 computeBiNormal(const in float time, const in vec2 UV) {
	float normalX = 0;

	for (uint i = 0; i < min(ubo.nrWaves, MaxWaves); i++) {

		const Wave w = ubo.waves[i];

		normalX += (w.wavelength * w.direction.x * UV.x) * w.amplitude *
				   cos(dot(w.direction, UV.xy) * w.wavelength + time * w.speed);
	}
	return vec3(1, 0, normalX);
}

vec3 computeTangent(const in float time, const in vec2 UV) {
	float normalX = 0;
	float normalY = 0;

	for (uint i = 0; i < min(ubo.nrWaves, MaxWaves); i++) {

		const Wave w = ubo.waves[i];

		normalY += (w.wavelength * w.direction.y * UV.y) * w.amplitude *
				   cos(dot(w.direction, UV.xy) * w.wavelength + time * w.speed);
	}

	return vec3(0, 1, normalY);
}

vec3 computeNormal(const in float time, const in vec2 UV) {

	const vec3 surface_binormal = computeBiNormal(time, UV);
	const vec3 surface_tangent = computeTangent(time, UV);

	const vec3 surface_normal = cross(surface_binormal, surface_tangent);

	return normalize(surface_normal);
}

void main() {

	/*	*/
	if (ubo.rolling != 0) {
	}

	float height = 0;
	for (uint i = 0; i < min(ubo.nrWaves, MaxWaves); i++) {
		height += computeHeight(ubo.time, ubo.waves[i], TextureCoord);
	}

	const vec3 surface_normal = computeNormal(ubo.time, TextureCoord);

	const vec3 surface_vertex = Vertex + Normal * height;

	gl_Position = ubo.modelViewProjection * vec4(surface_vertex, 1.0);

	vertex = (ubo.model * vec4(surface_vertex, 1.0)).xyz;
	normal = normalize((ubo.model * vec4(surface_normal, 0.0)).xyz);
	UV = TextureCoord;
}