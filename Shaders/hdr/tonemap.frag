#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 uv;

layout(binding = 1) uniform sampler2D HDRTexture;

layout(binding = 0, std140) uniform UniformBufferBlock {
	float exposure;
	float bloomFactor;
	float birghtMax;
}
ubo;

void main() {
	float exposure;

	float bloomFactor;

	float brightMax;

	vec4 hdrColor = texture(HDRTexture, uv);

	float Y = dot(vec4(0.30, 0.59, 0.11, 0.0), hdrColor);
	float YD = exposure * (exposure / brightMax + 1.0) / (exposure + 1.0);
	fragColor *= YD;
}