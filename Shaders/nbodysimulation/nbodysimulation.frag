

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec4 velocity;

layout(binding = 1) uniform sampler2D texSampler;

struct particle_setting {
	float speed;
	float lifetime;
	float gravity;

	float _SofteningSquared, _DeltaTime, _Damping;
	uint _NumBodies;
	ivec4 _GroupDim, _ThreadDim;
};

layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	particle_setting setting;
}
ubo;

void main() { outColor = vec4(velocity.xyz, 1.0); }
