#version 450
#extension GL_ARB_separate_shader_objects : enable

/*  */
layout(location = 0) out vec4 fragColor;

/*  */
layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;

/*  */
layout(binding = 0) uniform sampler2D diffuse;

void main() { fragColor = texture(diffuse, UV); }