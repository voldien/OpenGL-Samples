#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;

struct DirectionLight {
	BaseLight base;
	vec3 Direction;
};



layout(location = 2) uniform sampler2D DiffuseTexture;
layout(location = 3) uniform sampler2D NormalTexture;


void main() {

	// Create new normal per pixel based on the normal map.
	vec3 Mnormal = normalize(normal);
	vec3 Ttangent = normalize(tangent);
	Ttangent = normalize(Ttangent - dot(Ttangent, Mnormal) * Mnormal);
	vec3 bittagnet = cross(Ttangent, Mnormal);

	vec3 NormalMapBump = 2.0 * texture2D(NormalTexture, UV).xyz - vec3(1);

	vec3 alteredNormal = mat3(Ttangent, bittagnet, Mnormal) * NormalMapBump;

	// Compute directional light
	contribution = max(0.0, dot(alteredNormal, -normalize(_DirectionLight[x].Direction)));
	EndColor += vec3(_DirectionLight[x].base.Color * contribution * _DirectionLight[x].base.Intensity);

	fragColor = texture2D(DiffuseTexture, UV) * vec4(LightColor(), 1);
}