#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in flat int InstanceID;

layout(binding = 1) uniform sampler2D WorldTexture;
layout(binding = 2) uniform sampler2D DepthTexture;
layout(binding = 3) uniform sampler2D NormalTexture;

struct point_light {
	vec3 position;
	float range;
	vec4 color;
	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;
};

layout(binding = 0, std140) uniform UniformBufferBlock {
	point_light point_light[64];
}
ubo;

void main() {

	vec4 world = texture(WorldTexture, uv);
	vec3 normal = texture(NormalTexture, uv).rgb;

	vec3 diffVertex = (ubo.point_light[InstanceID].position - world.xyz);
	float dist = length(diffVertex);

	float attenuation = 1.0 / (ubo.point_light[InstanceID].constant_attenuation + ubo.point_light[InstanceID].linear_attenuation * dist +
							   ubo.point_light[InstanceID].qudratic_attenuation * (dist * dist));

	float contribution = max(dot(normal, normalize(diffVertex)), 0.0);

	vec4 pointLightColors =
		attenuation * ubo.point_light[InstanceID].color * contribution * ubo.point_light[InstanceID].range * ubo.point_light[InstanceID].intensity;

	fragColor = pointLightColors;
}