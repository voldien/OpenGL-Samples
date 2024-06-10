#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 1) in vec4 velocity;
layout(location = 2) in vec2 normal;


/*	Include common.	*/
#include"ocean_base.glsl"

layout(binding = 1) uniform sampler2D panorama;



void main() {
    // Physical based rendering.


    fragColor = vec4(1,1,1,1);
}