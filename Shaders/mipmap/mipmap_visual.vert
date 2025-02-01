#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

/*	*/
layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec3 TextureCoordinate;

/*	*/
layout(location = 0) out vec2 uv;

/*	*/
layout(set = 0, binding = 0, std140) uniform UniformBufferBlock { mat4 modelViewProjection; }
ubo;

void main() {
	gl_Position = ubo.modelViewProjection * vec4(Vertex, 1.0);
	uv = TextureCoordinate.xy;
}