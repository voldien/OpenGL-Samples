#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

/*  */
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;

#include "postprocessing_base.glsl"

layout(push_constant) uniform UniformBufferBlock {

	float _Blend;
	vec4 _Color;
	vec4 _MainTex_TexelSize;
	float _Speed;
	float _LineWidth;
}
settings;

void main() {

	/*	Compute sample coordinate.	*/
	// vec2 sampleTexCoord = i.texcoord;

	// /*	*/
	// vec4 referenceColor = SAMPLE_TEXTURE2D(_MainTex, sampler_MainTex, sampleTexCoord);
	// vec4 scanlineColor = SAMPLE_TEXTURE2D(_MainTex, sampler_MainTex, sampleTexCoord);

	// /*	*/
	// float scans = clamp(0.35 + settings._LineWidth * sin(settings._Speed * settings._Time.y + screenUV.y * _ScreenParams.y * 2.0), 0.0, 1.0);

	// float s = pow(scans, 1.7);

	// /*	*/
	// scanlineColor = scanlineColor * (0.4 + 0.7 * s);

	// /*	*/
	// scanlineColor *= 1.0 - 0.65 * (clamp((fmod(_ScreenParams.z, 2.0) - 1.0) * 2.0, 0.0, 1.0));

	// scanlineColor *= _Color;

	// return mix(referenceColor, scanlineColor, settings._Blend.xxxx);
}