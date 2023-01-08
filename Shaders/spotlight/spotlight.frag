#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

struct spot_light {
	vec4 position;
	vec4 color;
	vec4 direction;
	float range;
	float angle;
	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;
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
	for (int i = 0; i < 4; i++) {

		vec3 deltaSpace = (ubo.spot_light[i].position.xyz - vertex); // TODO resolve this later in the!

		float SpotFactor = dot(normalize(deltaSpace), -normalize(ubo.spot_light[i].direction.xyz));
		float SpotEffect;

		if (SpotFactor > ubo.spot_light[i].angle) {
			float dist = length(deltaSpace);

			float attenuation =
				1.0 / (ubo.spot_light[i].constant_attenuation + ubo.spot_light[i].linear_attenuation * dist +
					   ubo.spot_light[i].qudratic_attenuation * (dist * dist));

			float contribution = max(0.0, dot(normalize(normal), normalize(deltaSpace))) * SpotFactor;
			SpotEffect = 1.0; // smoothstep(0.1 , 0.0f, 1.0f - SpotFactor);
			pointLightColors += contribution * max(0.0, SpotFactor) * attenuation * ubo.spot_light[i].intensity *
								ubo.spot_light[i].color;
		}
	}

	fragColor = texture(DiffuseTexture, uv) * (ubo.ambientColor + pointLightColors);
}