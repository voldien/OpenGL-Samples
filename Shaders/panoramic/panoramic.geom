#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) in vec3 InVertex[];
layout(location = 1) in vec2 InUV[];
layout(location = 2) in vec3 InNormal[];
layout(location = 3) in vec3 InTangent[];

layout(location = 0) out vec3 OutVertex;
layout(location = 1) out vec2 OutUV;
layout(location = 2) out vec3 OutNormal;
layout(location = 3) out vec3 OutTangent;
layout(location = 4) out vec3 ViewDir;

layout(constant_id = 8) const uint NrLayers = 6;
layout(constant_id = 9) const uint NrFaces = 3;

#include "common.glsl"
#include "phongblinn.glsl"
#include "scene.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProjection[6];
	mat4 modelViewProjection;

	/*	Light source.	*/
	DirectionalLight directional;
	Camera camera;

	/*	Material settings*/
	vec4 specularColor;
	vec4 ambientColor;

	vec4 shininess;
}
ubo;

/*	Based on the cubemap layer direction.	*/
const vec3 view_directions[6] = {vec3(1, 0, 0),	 vec3(-1, 0, 0), vec3(0, 1, 0),
								 vec3(0, -1, 0), vec3(0, 0, -1), vec3(0.0, 0.0, 1)};

void main() {

	[[unroll]] for (uint face = 0; face < NrLayers; ++face) {
		gl_Layer = int(face);						  // built-in variable that specifies to which face we render.
		[[unroll]] for (uint i = 0; i < NrFaces; ++i) // for each triangle vertex
		{
			OutVertex = gl_in[i].gl_Position.xyz;
			gl_Position = (ubo.ViewProjection[gl_Layer]) * gl_in[i].gl_Position;
			/*	*/
			OutUV = InUV[i];
			OutNormal = InNormal[i];
			OutTangent = InTangent[i];
			ViewDir = (ubo.ViewProjection[face] * vec4(view_directions[face], 0)).xyz;

			EmitVertex();
		}
		EndPrimitive();
	}
}