#version 450
#extension GL_ARB_separate_shader_objects : enable
layout (vertices = 3) out;


// attributes of the input CPs
layout(location = 0) in vec3 WorldPos_CS_in[];
layout(location = 1) in vec2 TexCoord_CS_in[];
layout(location = 2) in vec3 Normal_CS_in[];

// attributes of the output CPs
layout(location = 0) out vec3 WorldPos_ES_in[];
layout(location = 1) out vec2 TexCoord_ES_in[];
layout(location = 2) out vec3 Normal_ES_in[];
void main() {}
