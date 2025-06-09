#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_conservative_depth: enable

layout(early_fragment_tests) in;
layout (depth_less) out float gl_FragDepth;

void main() { gl_FragDepth = gl_FragCoord.z; }