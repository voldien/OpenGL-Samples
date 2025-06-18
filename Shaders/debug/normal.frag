
#version 460
#extension GL_ARB_separate_shader_objects : enable
/*  */
layout(location = 0) out vec4 fragColor;

layout(location = 0) smooth in vec3 normal;

void main() { fragColor =  vec4(normal, 1); }