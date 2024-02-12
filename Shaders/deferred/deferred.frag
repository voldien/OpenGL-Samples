#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 1) in flat int InstanceID;

layout(binding = 0) uniform sampler2D AlbedoTexture;
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

layout(binding = 1, std140) uniform UniformBufferLight { point_light point_light[64]; }
pointlightUBO;

vec2 CalcTexCoord() { return gl_FragCoord.xy / vec2(1920, 1080); }

void main() {

	const vec2 uv = CalcTexCoord();

	const vec4 color = vec4(texture(AlbedoTexture, uv).rgb, 1);
	const vec3 world = texture(WorldTexture, uv).xyz;
	const vec3 normal = texture(NormalTexture, uv).xyz;

	/*	*/
	const vec3 diffVertex = (pointlightUBO.point_light[InstanceID].position - world.xyz);
	const float dist = length(diffVertex);

	/*	*/
	const float attenuation = 1.0 / (pointlightUBO.point_light[InstanceID].constant_attenuation +
							   pointlightUBO.point_light[InstanceID].linear_attenuation * dist +
							   pointlightUBO.point_light[InstanceID].qudratic_attenuation * (dist * dist));

	/*	*/
	const float contribution = max(dot(normal, -normalize(diffVertex)), 0.0);

	const vec4 pointLightColors = attenuation * pointlightUBO.point_light[InstanceID].color * contribution *
							pointlightUBO.point_light[InstanceID].range *
							pointlightUBO.point_light[InstanceID].intensity;

	fragColor = color * pointLightColors;
}