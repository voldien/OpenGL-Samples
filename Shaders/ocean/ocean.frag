#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 1) in vec4 velocity;

layout(binding = 1) uniform sampler2D panorama;


void main() {
    fragColor = vec4(1,1,1,1);
}