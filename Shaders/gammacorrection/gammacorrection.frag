#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 uv;

layout(binding = 1) uniform sampler2D AlbedoTexture;

layout(binding = 0, std140) uniform UniformBufferBlock {
	float exposure;
	float gamma;
}
ubo;

void main() {

	/*  */
	fragColor = textureLod(AlbedoTexture, uv, 0);
	fragColor = vec4(1.0) - exp(-fragColor * ubo.exposure);

	/*  */
	fragColor = pow(fragColor, vec4(1.0 / ubo.gamma));

	/*  */
	fragColor = texture(AlbedoTexture, uv);// * pow();
}