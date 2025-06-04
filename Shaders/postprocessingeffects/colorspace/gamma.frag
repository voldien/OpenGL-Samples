#version 460 core
#extension GL_ARB_derivative_control : enable
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

layout(binding = 1) uniform sampler2D ColorTexture;

layout(push_constant) uniform Settings {
	layout(offset = 0) float exposure;
	layout(offset = 4) float gamma;
}
settings;

void main() {

	vec4 fragColor = texture(ColorTexture, screenUV);
	fragColor = vec4(1.0) - exp(-fragColor * settings.exposure);

	const float gamma = settings.gamma;
	fragColor = pow(fragColor, vec4(1.0 / gamma));

	fragColor = vec4(fragColor.rgb, 1.0);
}