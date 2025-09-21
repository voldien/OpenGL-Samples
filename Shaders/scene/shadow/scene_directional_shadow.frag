#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

#extension GL_ARB_conservative_depth : enable
#extension GL_EXT_conservative_depth : enable

layout(early_fragment_tests) in;


layout(location = 8) flat in ivec2 fAssigns;


#if defined(GL_EXT_conservative_depth) || defined(GL_ARB_conservative_depth)
layout(depth_less) out float gl_FragDepth;
#endif

void main() { gl_FragDepth = gl_FragCoord.z; }