#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

// layout(early_fragment_tests) in;

void main() { gl_FragDepth = gl_FragCoord.z; }