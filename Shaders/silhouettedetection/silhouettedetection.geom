#version 450

layout(triangles_adjacency) in;
layout(line_strip, max_vertices = 6) out;

layout(location = 0) out vec3 vertex[];

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec3 lightPos;
	vec4 ambientColor;
}
ubo;

void EmitLine(int StartIndex, int EndIndex) {
	gl_Position = gl_in[StartIndex].gl_Position;
	EmitVertex();

	gl_Position = gl_in[EndIndex].gl_Position;
	EmitVertex();

	EndPrimitive();
}

void main() {
	vec3 e1 = vertex[2] - vertex[0];
	vec3 e2 = vertex[4] - vertex[0];
	vec3 e3 = vertex[1] - vertex[0];
	vec3 e4 = vertex[3] - vertex[2];
	vec3 e5 = vertex[4] - vertex[2];
	vec3 e6 = vertex[5] - vertex[0];

	vec3 Normal = cross(e1, e2);
	vec3 LightDir = ubo.lightPos - vertex[0];

	if (dot(Normal, LightDir) > 0.00001) {

		Normal = cross(e3, e1);

		if (dot(Normal, LightDir) <= 0) {
			EmitLine(0, 2);
		}

		Normal = cross(e4, e5);
		LightDir = ubo.lightPos - vertex[2];

		if (dot(Normal, LightDir) <= 0) {
			EmitLine(2, 4);
		}

		Normal = cross(e2, e6);
		LightDir = ubo.lightPos - vertex[4];

		if (dot(Normal, LightDir) <= 0) {
			EmitLine(4, 0);
		}
	}
}