#include "material.glsl"

struct Node {
	mat4 model;
};

layout(binding = 0) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2D NormalTexture;
layout(binding = 2) uniform sampler2D AlphaMaskedTexture;

layout(binding = 5) uniform sampler2D RoughnessTexture;
layout(binding = 6) uniform sampler2D MetalicTexture;
layout(binding = 4) uniform sampler2D EmissionTexture;
layout(binding = 7) uniform sampler2D DisplacementTexture;

layout(binding = 10) uniform sampler2D IrradianceTexture;