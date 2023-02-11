#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	vec4 tintColor;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	/*  */
	vec4 cameraPosition;
	vec2 scale;
}
ubo;

layout(location = 0) out vec2 FUV;

void main() {
	const vec3 VPosition = gl_in[0].gl_Position.xyz;

	const vec3 CameraN = normalize(ubo.cameraPosition.xyz - VPosition);

	/*  */
	vec3 right = normalize(cross(CameraN, vec3(0.0, 1.0, 0.0)));
	vec3 up = cross(CameraN, right);

	up *= ubo.scale.y;
	right *= ubo.scale.x;

	/*  */
	vec3 vertexPosition = VPosition - (right * 0.5) - (up * 0.5);
	gl_Position = ubo.ViewProj * vec4(vertexPosition, 1.0);
	FUV = vec2(0.0f, 0.0f);
	EmitVertex();

	/*  */
	vertexPosition = VPosition + (right * 0.5) - (up * 0.5);
	gl_Position = ubo.ViewProj * vec4(vertexPosition, 1.0);
	FUV = vec2(0.0f, 1.0f);
	EmitVertex();

	/*  */
	vertexPosition = VPosition - (right * 0.5) + (up * 0.5);
	gl_Position = ubo.ViewProj * vec4(vertexPosition, 1.0);
	FUV = vec2(1.0f, 0.0f);
	EmitVertex();

	/*  */
	vertexPosition = VPosition + (right * 0.5) + (up * 0.5);
	gl_Position = ubo.ViewProj * vec4(vertexPosition, 1.0);
	FUV = vec2(1.0f, 1.0f);
	EmitVertex();

	EndPrimitive();
}