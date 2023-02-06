#version 460
#extension GL_ARB_separate_shader_objects : enable
/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 uv;

layout(binding = 1) uniform sampler2D diffuse;

/*	*/
layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	float clipping;
}
ubo;

void main() {
	fragColor = texture(diffuse, uv);
	if (fragColor.a < ubo.clipping) {
		discard;
	}
}