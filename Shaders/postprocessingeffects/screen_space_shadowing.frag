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

#include "postprocessing_base.glsl"

layout(push_constant) uniform Settings {
	layout(offset = 0) BaseSettings base;
	layout(offset = 4) int g_sss_max_steps;
	layout(offset = 8) float g_sss_ray_max_distance;
	layout(offset = 12) float g_sss_thickness;
	layout(offset = 16) float g_sss_step_length;
	layout(offset = 32) vec2 g_taa_jitter_offset;
	layout(offset = 48) vec3 light_direction;
	layout(offset = 60) float intensity;
	layout(offset = 64) float g_resolution;
}
settings;

bool is_valid_uv(const vec2 value) {
	return (value.x >= 0.0f && value.x <= 1.0f) && (value.y >= 0.0f && value.y <= 1.0f);
}

float screen_fade(const in vec2 uv) {
	vec2 fade = max(vec2(0.0), 12.0f * abs(uv - 0.5f) - 5.0f);
	return clamp(1.0 - dot(fade, fade), 0, 1);
}

vec3 calcViewPosition(const in vec2 coords) {
	const float fragmentDepth = texture(DepthTexture, coords).r;
	return calcViewPosition(coords, inverse(constantCommon.constant.camera.proj), fragmentDepth);
}

float interleaved_gradient_noise(in vec2 position_screen) {
	// g_frame *
	position_screen += vec2(0); // 1 * any(settings.g_taa_jitter_offset); // temporal factor

	vec3 magic = vec3(0.06711056f, 0.00583715f, 52.9829189f);
	return fract(magic.z * fract(dot(position_screen, magic.xy)));
}

const float g_sss_max_delta_from_original_depth = 1.5f;

vec3 ScreenSpaceShadows() {

	/*	*/
	const mat4 g_projection = constantCommon.constant.camera.proj;
	/* Compute ray viewPos and direction (in view-space)	*/
	const vec3 viewPos = calcViewPosition(screenUV);
	vec3 ray_pos = viewPos;

	const vec3 ray_dir =
		normalize(world_to_view(constantCommon.constant.camera.view, settings.light_direction.xyz, true));

	float depth_original = ray_pos.z;

	const float g_sss_step_length = settings.g_sss_ray_max_distance / settings.g_sss_max_steps;

	/* Compute ray step	*/
	const vec3 ray_step = ray_dir * g_sss_step_length;
	const float ray_offset = 0; // interleaved_gradient_noise(settings.g_resolution * screenUV);
	ray_pos += ray_step * ray_offset;

	/*	Ray march towards the light	*/
	float occlusion = 0.0;
	vec2 ray_uv = vec2(0.0);

	/*	*/
	for (uint i = 0; i < settings.g_sss_max_steps; i++) {
		/* Step the ray	*/
		ray_pos += ray_step;
		ray_uv = view_to_uv(g_projection, ray_pos, true);

		/*	Ensure the UV coordinates are inside the screen	*/
		if (!is_valid_uv(ray_uv)) {
			return vec3(1);
		}

		// Compute the difference between the ray's and the camera's depth
		const float depth_z = calcViewPosition(ray_uv).z;
		const float depth_delta = ray_pos.z - depth_z;

		// Check if the camera can't "see" the ray (ray depth must be larger than the camera depth, so
		const bool can_the_camera_see_the_ray =
			(settings.intensity * depth_delta > 0.0001f) && (depth_delta < settings.g_sss_thickness);

		const bool occluded_by_the_original_pixel =
			abs(ray_pos.z - depth_original) < g_sss_max_delta_from_original_depth;

		if (can_the_camera_see_the_ray && occluded_by_the_original_pixel) {

			occlusion += 1 * screen_fade(ray_uv);
			 break;
		}
	}

	//occlusion *= (1.0 / settings.g_sss_max_steps);
	occlusion *= settings.base.blend;

	return vec3((1 - occlusion));
}

void main() {
	const vec3 shadow = ScreenSpaceShadows();

	fragColor = vec4(shadow, 1);
}
