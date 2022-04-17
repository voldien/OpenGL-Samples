#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out vec2 UV;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 tangent;

layout(location = 0) uniform mat4 NormalMatrix;
layout(location = 1) uniform mat4 MVP;

void main() {
	gl_Position = MVP * vec4(Vertex, 1.0);
	normal = NormalMatrix * Normal;
	tangent = NormalMatrix * Tangent;
	UV = TextureCoord;
}