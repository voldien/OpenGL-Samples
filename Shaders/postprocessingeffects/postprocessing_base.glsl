#include "colorspace.glsl"
#include "common.glsl"
#include "transformation.glsl"
#include"light.glsl"
//#include"scene.glsl"

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

	vec4 time;
	//ivec4 frame;
};

struct light_settings {
	DirectionalLight directional[16];
	PointLight point[64];
	uint directionalCount;
	uint pointCount;
};

/*	*/
layout(set = 2, binding = 5, std140) uniform UniformLightBufferBlock { light_settings light; }
LightUBO;

struct BaseSettings {
	float blend;
};

layout(set = 1, binding = 1, std140) uniform UniformCommonPostProcessingBufferBlock { common_data constant; }
constantCommon;
