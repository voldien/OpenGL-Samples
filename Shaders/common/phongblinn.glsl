struct DirectionalLight {
	vec4 direction;
	vec4 lightColor;
};

void computeBlinnDirectional(const in DirectionalLight light, const in vec3 normal, const in position) {

	const vec3 diffVertex = (ubo.point_light[i].position - vertex);
	const vec3 lightDir = normalize(diffVertex);
	const float dist = length(diffVertex);

	const float attenuation =
		1.0 / (ubo.point_light[i].constant_attenuation + ubo.point_light[i].linear_attenuation * dist +
			   ubo.point_light[i].qudratic_attenuation * (dist * dist));

	const float contribution = max(dot(normalize(normal), lightDir), 0.0);

	pointLightColors +=
		attenuation * ubo.point_light[i].color * contribution * ubo.point_light[i].range * ubo.point_light[i].intensity;

	/*	*/
	const vec3 reflectDir = reflect(-lightDir, normal);
	const float spec = pow(max(dot(viewDir, reflectDir), 0.0), ubo.shininess);
}

void computePhongDirectional(const in DirectionalLight light) {

	const vec3 diffVertex = (ubo.point_light[i].position - vertex);
	const vec3 lightDir = normalize(diffVertex);
	const float dist = length(diffVertex);

	const float attenuation =
		1.0 / (ubo.point_light[i].constant_attenuation + ubo.point_light[i].linear_attenuation * dist +
			   ubo.point_light[i].qudratic_attenuation * (dist * dist));

	const float contribution = max(dot(normalize(normal), lightDir), 0.0);

	pointLightColors +=
		attenuation * ubo.point_light[i].color * contribution * ubo.point_light[i].range * ubo.point_light[i].intensity;

	/*	*/
	const vec3 reflectDir = reflect(-lightDir, normal);
	const float spec = pow(max(dot(viewDir, reflectDir), 0.0), ubo.shininess);
}
