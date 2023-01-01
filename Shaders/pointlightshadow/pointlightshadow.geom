#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

// layout(location = 0) in vec2 vUV[];
layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProjection[6];
	mat4 modelViewProjection;
	/*	Light source.	*/
	vec4 ambientColor;
	// point_light point_light[4];
}
ubo;

// uniform mat4 ubo.ViewProjection[6];
// uniform mat4 ubo.model;

void main() {

	/*	first texture	*/
	gl_Layer = 0;
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[0].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[1].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[2].gl_Position;
	EmitVertex();
	EndPrimitive();

	/*	Second texture.	*/
	gl_Layer = 1;
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[0].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[1].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[2].gl_Position;
	EmitVertex();
	EndPrimitive();

	/*	Third texture.	*/
	gl_Layer = 2;
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[0].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[1].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[2].gl_Position;
	EmitVertex();
	EndPrimitive();

	/*	Fourth texture.	*/
	gl_Layer = 3;
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[0].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[1].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[2].gl_Position;
	EmitVertex();
	EndPrimitive();

	/*	Fifth texture.	*/
	gl_Layer = 4;
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[0].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[1].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[2].gl_Position;
	EmitVertex();
	EndPrimitive();

	/*	Sixth texture.	*/
	gl_Layer = 5;
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[0].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[1].gl_Position;
	EmitVertex();
	gl_Position = (ubo.ViewProjection[gl_Layer] * ubo.model) * gl_in[2].gl_Position;
	EmitVertex();
	EndPrimitive();
}