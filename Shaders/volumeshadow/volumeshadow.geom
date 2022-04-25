#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(triangles_adjacency) in; // six vertices in
layout(triangle_strip, max_vertices = 18) out;

void main() {
	// vec3 e1 = WorldPos[2] - WorldPos[0];
	// vec3 e2 = WorldPos[4] - WorldPos[0];
	// vec3 e3 = WorldPos[1] - WorldPos[0];
	// vec3 e4 = WorldPos[3] - WorldPos[2];
	// vec3 e5 = WorldPos[4] - WorldPos[2];
	// vec3 e6 = WorldPos[5] - WorldPos[0];

	// vec3 Normal = cross(e1,e2);
}