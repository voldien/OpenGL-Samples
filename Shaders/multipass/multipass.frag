#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 1) out float Depth;
layout(location = 2) out vec3 Normal;
layout(location = 3) out vec3 WorldSpace;

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;


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

	fragColor = texture2D(DiffuseTexture, UV);
    Normal = normal;
    WorldSpace = Vertex;
}