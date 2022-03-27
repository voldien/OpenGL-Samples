#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec3 vVertex;

layout(binding = 0) uniform samplerCube texture0;

void main(void) { fragColor = texture(texture0, vVertex); }