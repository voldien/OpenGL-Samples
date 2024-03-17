#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_control_flow_attributes : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

struct spot_light {
	vec4 position;
	vec4 direction;
	vec4 color;
	float range;
	float angle;
	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;
	float padd0;
	float padd1;
};

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 ambientColor;
	spot_light spot_light[4];
}
ubo;

layout(binding = 1) uniform sampler2D DiffuseTexture;

void main() {

	vec4 pointLightColors = vec4(0);

	[[unroll]] for (int i = 0; i < 4; i++) {

		const vec3 deltaSpace = (vertex - ubo.spot_light[i].position.xyz); // TODO resolve this later in the!
		const vec3 light_direction = normalize(deltaSpace);

		const float SpotFactor = dot(light_direction, ubo.spot_light[i].direction.xyz);
		const float dist = length(deltaSpace);

		if (SpotFactor > ubo.spot_light[i].angle) {

			const float attenuation =
				1.0 / (ubo.spot_light[i].constant_attenuation + ubo.spot_light[i].linear_attenuation * dist +
					   ubo.spot_light[i].qudratic_attenuation * (dist * dist));

			const float SpotEffect = smoothstep(0.1, 0.0, 1.0 - SpotFactor);
			const float contribution = max(0.0, dot(normal, -light_direction)) * SpotFactor;

			pointLightColors += contribution * max(0.0, SpotFactor) * attenuation * ubo.spot_light[i].intensity *
								ubo.spot_light[i].range * ubo.spot_light[i].color * SpotEffect;
		}
	}

	fragColor = texture(DiffuseTexture, uv) * (ubo.ambientColor + pointLightColors);
}