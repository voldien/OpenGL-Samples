#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(early_fragment_tests) in;

layout(location = 0) in vec2 TextureCoord;

layout(binding = 0) uniform sampler2D DiffuseTexture;

void main() {
	const float alpha = texture(DiffuseTexture, TextureCoord).a;
	if (alpha < 1.0) {
		discard;
	} else {
		gl_FragDepth = gl_FragCoord.z;
	}
}