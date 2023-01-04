#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;


layout(location = 1) in vec4 velocity;
layout(location = 2) in vec2 normal;

layout(binding = 1) uniform sampler2D panorama;


layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	vec4 diffuse;
	float deltaTime;


}
ubo;


void main() {
    // Physical based rendering.


    fragColor = vec4(1,1,1,1);
}