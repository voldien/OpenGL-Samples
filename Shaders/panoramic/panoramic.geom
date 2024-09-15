#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) in vec3 InVertex[];
layout(location = 1) in vec2 InUV[];
layout(location = 2) in vec3 InNormal[];

layout(location = 0) out vec4 OutVertex;
layout(location = 1) out vec2 OutUV;
layout(location = 2) out vec3 OutNormal;

layout(constant_id = 0) const float NrLayers = 6;
layout(constant_id = 1) const float NrFaces = 3;

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
	vec4 specularColor;
	vec4 ambientColor;
	vec4 viewDir;

	float shininess;
}
ubo;

void main() {

	[[unroll]] for (int face = 0; face < NrLayers; ++face) {
		gl_Layer = face; // built-in variable that specifies to which face we render.

		[[unroll]] for (int i = 0; i < NrFaces; ++i) // for each triangle vertex
		{
			OutVertex = gl_in[i].gl_Position;
			gl_Position = (ubo.ViewProjection[gl_Layer]) * gl_in[i].gl_Position;
			OutUV = InUV[i];
			OutNormal = InNormal[i];
			EmitVertex();
		}
		EndPrimitive();
	}
}