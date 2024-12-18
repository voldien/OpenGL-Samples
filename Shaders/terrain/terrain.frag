#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 in_WorldPosition;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

#include "terrain_base.glsl"
#include "phongblinn.glsl"
#include "fog_frag.glsl"

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 2) uniform sampler2D NormalTexture;
layout(binding = 2) uniform sampler2D Irradiance;


void main() {

	const vec3 alteredNormal = normal;

	/*	*/
	vec4 lightColor = computeBlinnDirectional(ubo.directional, alteredNormal, ubo.camera.viewDir.xyz, ubo.shininess.r, vec3(1));

//

	fragColor = texture(DiffuseTexture, UV) * (in_WorldPosition.y * 0.01)  * (lightColor + ubo.ambientColor);
	fragColor = blendFog(fragColor, ubo.fogSettings);
	// fragColor = vec4(normal, 1);
}
