#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;
layout(location = 3) in vec4 instanceColor;

#include "debug_drawer.glsl"

void main() { fragColor = instanceColor; }