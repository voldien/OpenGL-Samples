#version 460
#extension GL_ARB_separate_shader_objects : enable

/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec4 LightColor;

void main() { fragColor = vec4(LightColor.rgb, 1.0); }
