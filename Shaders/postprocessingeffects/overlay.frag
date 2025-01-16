#version 460
#extension GL_ARB_separate_shader_objects : enable

precision mediump float;
precision mediump int;

/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 screenUV;
/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;

void main() { fragColor = texture(ColorTexture, screenUV); }
