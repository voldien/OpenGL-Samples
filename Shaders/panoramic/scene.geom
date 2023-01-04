#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(triangles_adjacency) in; // six vertices in
layout(triangle_strip, max_vertices = 18) out;
void main(){}