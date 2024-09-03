#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out vec3 vertex;
layout(location = 1) out vec2 UV;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 tangent;

struct Wave {
	float wavelength;
	float amplitude;
	float speed;
	float steepness;
	vec2 direction;
	vec2 padding;
};

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
	vec4 specularColor;
	vec4 ambientColor;
	vec4 position;

	Wave waves[64];
	int nrWaves;
	float time;

	/*	Material	*/
	float shininess;
	float fresnelPower;
	vec4 oceanColor;
}
ubo;

vec3 computeNormal(const in float time) {

	float normalX = 0;
	float normalY = 0;

	for (int i = 0; i < min(ubo.nrWaves, 64); i++) {

		const Wave w = ubo.waves[i];

		normalX += -(w.wavelength * w.direction.x * w.amplitude *
					 cos(dot(w.direction, Vertex.xy) * w.wavelength + time * w.speed));
		normalY += -(w.wavelength * w.direction.y * w.amplitude *
					 cos(dot(w.direction, Vertex.xy) * w.wavelength + time * w.speed));
	}

	return vec3(normalX, 1, normalY);
}
float computeHeight(const in float time, in const Wave w) {
	return w.amplitude * sin(dot(w.direction, Vertex.xy) * w.wavelength + time * w.speed);
}

// Gerstner Waves
vec3 computeGerstnerWaves(const in float time, in const Wave wave) {
	float height = 0;
	float x = Vertex.x;
	float y = Vertex.y;

	for (int i = 0; i < ubo.nrWaves; i++) {
		height += computeHeight(ubo.time, ubo.waves[i]);
	}

	return vec3(x, height, y);
}

// TODO: compute
vec3 computeTangent(const in float time, in const Wave w) { return vec3(0); }

void main() {

	float height = 0;
	for (int i = 0; i < min(ubo.nrWaves, 64); i++) {
		height += computeHeight(ubo.time, ubo.waves[i]);
	}

	const vec3 surface_normal = normalize(computeNormal(ubo.time));

	const vec3 surface_vertex = Vertex + Normal * height;

	gl_Position = ubo.modelViewProjection * vec4(surface_vertex, 1.0);

	vertex = (ubo.model * vec4(surface_vertex, 1.0)).xyz;
	normal = surface_normal; //(ubo.model * vec4(surface_normal, 0.0)).xyz;
	// normal = normalize((ubo.model * vec4(Normal, 0.0)).xyz);
	tangent = (ubo.model * vec4(Tangent, 0.0)).xyz;

	UV = TextureCoord;
}