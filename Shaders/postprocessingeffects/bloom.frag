#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 uv;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;
layout(set = 0, binding = 1) uniform sampler2D depth0;
layout(set = 0, binding = 2) uniform sampler2D IrradianceTexture;

#include "fog_frag.glsl"
#include "postprocessing_base.glsl"

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	mat4 proj;
	mat4 modelViewProjection;
	vec4 tintColor;
	FogSettings fogSettings;
}
ubo;

void main() {
	// getFogFactor()

	fragColor = texture(ColorTexture, uv) * 1.2;
}
