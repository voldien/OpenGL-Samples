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

float qFunc(const in Wave w) { return (1 / (w.wavelength * w.amplitude * ubo.nrWaves)) * ubo.stepness; }

float sfunc(const in Wave w, const in vec2 UV, const in float time) {
	return sin(dot(w.direction, UV.xy) * w.wavelength + time * w.speed);
}
float wafunc(const in Wave w) { return w.wavelength * w.amplitude; }

float cfunc(const in Wave w, const in vec2 UV, const in float time) {
	return cos(dot(w.direction, UV.xy) * w.wavelength + time * w.speed);
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

	float normalX = 0;
	float normalY = 0;
	float normalZ = 0;
	for (uint i = 0; i < min(ubo.nrWaves, MaxWaves); i++) {
		const Wave w = ubo.waves[i];

		// normalX += qFunc(w) * w.direction.x * w.direction.y * wafunc(w) * sfunc(w, UV, time);
		normalX += w.direction.x * wafunc(w) * cfunc(w, UV, time);
		normalY += w.direction.y * wafunc(w) * cfunc(w, UV, time);
		normalZ += qFunc(w) * wafunc(w) * sfunc(w, UV, time);
	}

	return normalize(vec3(-normalX, -normalY, 1 - normalZ));
}

// Gerstner Waves
vec3 computeGerstnerWaves(const in float time, const in vec3 vertex, const in vec3 Normal, const in vec2 UV) {
	float height = 0;
	float xOffset = 0;
	float yOffset = 0;

	float x = Vertex.x;
	float y = Vertex.y;

	for (uint i = 0; i < min(ubo.nrWaves, MaxWaves); i++) {
		const Wave w = ubo.waves[i];

		height += computeHeight(ubo.time, ubo.waves[i], UV);

		const float Q = qFunc(w);
		xOffset += Q * w.amplitude * w.direction.x * cos(dot(w.direction, UV.xy) * w.wavelength + time * w.speed);
		yOffset += Q * w.amplitude * w.direction.y * cos(dot(w.direction, UV.xy) * w.wavelength + time * w.speed);
	}

	return vec3(Vertex.x + xOffset, Vertex.y + yOffset, 0) + Normal * height;
}

void main() {

	const vec3 surface_vertex = computeGerstnerWaves(ubo.time, Vertex, Normal, TextureCoord);

	const vec3 surface_normal = computeNormal(ubo.time, TextureCoord);

	{
		gl_Position = ubo.modelViewProjection * vec4(surface_vertex, 1.0);

		vertex = (ubo.model * vec4(surface_vertex, 1.0)).xyz;

		normal = normalize((ubo.model * vec4(surface_normal, 0.0)).xyz);
	}
}