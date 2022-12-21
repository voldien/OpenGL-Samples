#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

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
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 ambientColor;
	vec4 specularColor;
	vec3 viewPos;
	float shininess;

	point_light point_light[4];
}
ubo;

layout(binding = 1) uniform sampler2D DiffuseTexture;

void main() {

	float spec = 0.0;

	vec3 viewDir = normalize(ubo.viewPos - vertex);

	// Compute directional light
	vec4 pointLightColors = vec4(0);
	for (int i = 0; i < 4; i++) {

		vec3 diffVertex = (ubo.point_light[i].position - vertex);
		vec3 lightDir = normalize(diffVertex);
		float dist = length(diffVertex);

		float attenuation =
			1.0 / (ubo.point_light[i].constant_attenuation + ubo.point_light[i].linear_attenuation * dist +
				   ubo.point_light[i].qudratic_attenuation * (dist * dist));

		float contribution = max(dot(normal, lightDir), 0.0);

		pointLightColors += attenuation * ubo.point_light[i].color * contribution * ubo.point_light[i].range *
							ubo.point_light[i].intensity;

		/*  */
		vec3 halfwayDir = normalize(lightDir + viewDir);
		spec = pow(max(dot(normal, halfwayDir), 0.0), ubo.shininess);

		// vec3 reflectDir = reflect(-lightDir, normal);
		// spec = pow(max(dot(viewDir, reflectDir), 0.0), ubo.shin);

		pointLightColors += (ubo.specularColor * spec);
	}
	pointLightColors.a = 1.0;

	fragColor = texture(DiffuseTexture, uv) * (ubo.ambientColor + pointLightColors);
}