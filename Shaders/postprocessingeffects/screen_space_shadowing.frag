#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

/*  */
layout(location = 1) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;
layout(set = 0, binding = 6) uniform sampler2D DepthTexture;

layout(push_constant) uniform Settings {
	layout(offset = 0) float blend;
	layout(offset = 4) int g_sss_max_steps;
	layout(offset = 8) float g_sss_ray_max_distance;
	layout(offset = 12) float g_sss_thickness;
	layout(offset = 16) float g_sss_step_length;
	layout(offset = 32) vec2 g_taa_jitter_offset;
	layout(offset = 48) vec3 light_direction;
}
settings;

#include "postprocessing_base.glsl"

bool is_valid_uv(const vec2 value) {
	return (value.x >= 0.0f && value.x <= 1.0f) && (value.y >= 0.0f && value.y <= 1.0f);
}

float screen_fade(const in vec2 uv) {
	vec2 fade = max(vec2(0.0), 12.0f * abs(uv - 0.5f) - 5.0f);
	return clamp(1.0 - dot(fade, fade), 0, 1);
}

vec3 calcViewPosition(const in vec2 coords) {
	const float fragmentDepth = texture(DepthTexture, coords).r;
	return calcViewPosition(coords, constantCommon.constant.camera.inverseProj, fragmentDepth);
}

float interleaved_gradient_noise(in vec2 position_screen) {
	// g_frame *
	position_screen += vec2(0); // vec2(notEqual(settings.g_taa_jitter_offset, vec2(0))); // temporal factor
	vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
	return fract(magic.z * fract(dot(position_screen, magic.xy)));
}

float ScreenSpaceShadows() {
	const mat4 g_projection = constantCommon.constant.camera.proj;

	/* Compute ray position and direction (in view-space)	*/
	vec3 ray_pos = calcViewPosition(screenUV);
	const vec3 ray_dir = normalize(settings.light_direction);

	/* Compute ray step	*/
	const vec3 ray_step = ray_dir * settings.g_sss_step_length;

	/*	Ray march towards the light	*/
	float occlusion = 0.0;
	vec2 ray_uv = vec2(0.0);

	for (uint i = 0; i < settings.g_sss_max_steps; i++) {
		/* Step the ray	*/
		float ray_offset = 1;
		ray_pos += ray_step * ray_offset;

		/*	Update Ray UV.	*/
		vec4 offset = vec4(ray_pos, 1.0);
		offset = constantCommon.constant.camera.proj * offset;
		offset.xyz /= offset.w;				 // perspective divide
		offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
		ray_uv = offset.xy;

		/*	Ensure the UV coordinates are inside the screen	*/
		if (is_valid_uv(ray_uv)) {

			// Compute the difference between the ray's and the camera's depth
			float depth_z = calcViewPosition(ray_uv).z;
			float depth_delta = ray_pos.z - depth_z;

			// Check if the camera can't "see" the ray (ray depth must be larger than the camera depth, so
			bool can_the_camera_see_the_ray = (depth_delta > 0.0f) && (depth_delta < settings.g_sss_thickness);
			bool occluded_by_the_original_pixel =
				true; // abs(ray_pos.z - depth_original) < settings.g_sss_max_delta_from_original_depth;
			if (can_the_camera_see_the_ray && occluded_by_the_original_pixel) {
				occlusion = 1.0f;

				/* Fade out as we approach the edges of the screen	*/
				occlusion *= screen_fade(ray_uv);

				break;
			}
		}
	}

	/*	*/
	return 1 - occlusion;
}

void main() {
	const float shadow = ScreenSpaceShadows();

	fragColor =vec4(shadow.xxx, 1);//  mix(0, vec4(shadow.xxx, 1), settings.blend);
}
