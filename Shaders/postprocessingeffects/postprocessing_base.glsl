#include "colorspace.glsl"
#include "common.glsl"

// TODO: remove
struct global_rendering_settings {
	vec4 ambientColor;
	FogSettings fogSettings;
};

struct common_data {
	Camera camera;
	Frustum frustum;

	global_rendering_settings globalSettings;

	mat4 view[3];
	mat4 proj[3];
};

layout(set = 1, binding = 1, std140) uniform UniformCommonPostProcessingBufferBlock { common_data constant; }
constantCommon;
