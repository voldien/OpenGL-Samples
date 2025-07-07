#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
/*	*/
layout(location = 8) in ivec2 vAssigns;

layout(location = 0) out vec3 vertex;
layout(location = 1) out vec2 UV;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 tangent;
/*	*/
layout(location = 8) flat invariant out ivec2 fAssigns;

#include "scene.glsl"

 

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProjection[6];
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	vec4 cameraPosition;

	PointLight point_light[4];
	vec4 PCFFilters[32];
	float diskRadius;
	int samples;
}
ubo;

void main() {
	const mat4 model = getModel(vAssigns.y);
	const mat4 viewProj = getCamera().viewProj;

	gl_Position = (viewProj * model) * vec4(Vertex, 1.0);
	vertex = (model * vec4(Vertex, 1.0)).xyz;
	normal = (model * vec4(Normal, 0.0)).xyz;
	tangent = (model * vec4(Tangent, 0.0)).xyz;
	UV = TextureCoord;

	/*	*/
	fAssigns = vAssigns;
}
