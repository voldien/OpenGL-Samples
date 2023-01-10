#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

// layout(early_fragment_tests) in;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform samplerCube ShadowTexture;

struct point_light {
	vec3 position;
	float range;
	vec4 color;
	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;
	float bias;
	float shadowStrength;
};

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProjection[6];
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	vec4 cameraPosition;

	point_light point_light[4];
}
ubo;

float ShadowCalculation(const in vec3 fragPosLightSpace) {

	vec3 frag2Light = (fragPosLightSpace - ubo.point_light[0].position);

	float closestDepth = texture(ShadowTexture, normalize(frag2Light)).r;
	closestDepth *= ubo.point_light[0].range;

	float bias = 0.05;
	float currentDepth = length(frag2Light);
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	return shadow;
}

void main() {
	vec4 pointLightColors = vec4(0);
	for (int i = 0; i < 1; i++) {
		vec3 diffVertex = (ubo.point_light[i].position - vertex);
		float dist = length(diffVertex);

		float attenuation =
			1.0 / (ubo.point_light[i].constant_attenuation + ubo.point_light[i].linear_attenuation * dist +
				   ubo.point_light[i].qudratic_attenuation * (dist * dist));

		float contribution = max(dot(normal, normalize(diffVertex)), 0.0);

		pointLightColors += attenuation * ubo.point_light[i].color * contribution * ubo.point_light[i].range *
							ubo.point_light[i].intensity * ShadowCalculation(vertex);
	}

	fragColor = texture(DiffuseTexture, UV) * (ubo.ambientColor + pointLightColors);
}