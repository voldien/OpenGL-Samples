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

layout(location = 0) out vec4 vertex;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 tangent;
layout(location = 4) out vec3 bitangent;
layout(location = 5) out vec3 velocity;

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	*/
}
ubo;

#include "scene.glsl"

void main() {

	const mat4 model = getModel(vAssigns.y);

	/*	*/
	gl_Position = ubo.modelViewProjection * vec4(Vertex, 1.0);

	/*	*/
	vertex = (ubo.modelView * vec4(Vertex, 1.0));

	/*	*/
	normal = normalize((ubo.model * vec4(Normal, 0.0)).xyz);
	tangent = normalize((ubo.model * vec4(Tangent, 0.0)).xyz);

	/*	*/
	const vec3 Ttangent = normalize(tangent - dot(tangent, normal) * normal);
	bitangent = cross(Ttangent, normal);

	/*	*/
	uv = TextureCoord;

	velocity = getVelocity(vAssigns.y, Vertex);
}
