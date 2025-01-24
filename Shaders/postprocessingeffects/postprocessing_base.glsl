#include "colorspace.glsl"
#include "common.glsl"

struct common_data {
	Camera camera;
	Frustum frustum;

	mat4 view[3];
	mat4 proj[3];
};

layout(set = 1, binding = 1, std140) uniform UniformCommonPostProcessingBufferBlock { common_data constant; }
constantCommon;