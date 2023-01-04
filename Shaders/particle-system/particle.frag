#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 velocity;

layout(binding = 1) uniform sampler2D spriteTexture;

struct particle_setting {
	float speed;
	float lifetime;
	float gravity;
};

layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	*/
	float deltaTime;

	particle_setting setting;
}
ubo;

void main() { outColor = texture(spriteTexture, gl_PointCoord.xy) * vec4(abs(velocity.xyz), 1.0); }
