#version 460 core
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec3 vVertex;

layout(set = 0, binding = 0) uniform sampler2D SourceEnvTexture;

#include "common.glsl"

void main() {

	const vec3 normal = normalize(vVertex);

	vec3 irradiance = vec3(0.0);

	/*	Find tangent.	*/
	const vec3 c1 = cross(normal, vec3(0.0, 0.0, 1.0));
	const vec3 c2 = cross(normal, vec3(0.0, 1.0, 0.0));
	vec3 right;
	if (length(c1) > length(c2)) {
		right = normalize(c1);
	} else {
		right = normalize(c2);
	}
	const vec3 up = normalize(cross(normal, right));

	const float sampleDelta = PI2 / 512u;

	float nrSamples = 0.0;
	for (float phi = 0.0; phi < PI2; phi += sampleDelta) {

		vec3 sub_irradiance = vec3(0);
		uint sub_irradiance_sample_count = 0;

		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
			/* spherical to cartesian (in tangent space)	*/
			const vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

			/* tangent space to world	*/
			const vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

			/*	*/
			const vec2 panoramic_coordinate = sphere_uv_mapping(normalize(sampleVec));

			/*	Sample from pre-filter downsampled environment texture.	*/
			sub_irradiance += textureLod(SourceEnvTexture, panoramic_coordinate, 1).rgb * cos(theta) * sin(theta);

			sub_irradiance_sample_count++;
		}

		irradiance += sub_irradiance * (1.0 / float(sub_irradiance_sample_count));
		nrSamples++;
	}

	irradiance = PI * irradiance * (1.0 / float(nrSamples));

	fragColor.rgba = vec4(irradiance, 1);
}
