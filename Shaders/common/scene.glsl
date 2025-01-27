#ifndef _COMMON_SCENE_H_
#define _COMMON_SCENE_H_ 1

#include "common.glsl"
#include "light.glsl"
#include "material.glsl"

struct global_rendering_settings {
	vec4 ambientColor;
};

struct common_data {
	Camera camera;
	Frustum frustum;
	FogSettings fogSettings;
	global_rendering_settings globalSettings;

	mat4 view[3];
	mat4 proj[3];
};

struct Node {
	mat4 model;
};

struct tessellation_settings {
	float tessLevel;
	float gDispFactor;
};

struct light_settings {
	DirectionalLight directional[16];
	PointLight point[64];
};


/*	*/
layout(set = 1, binding = 1, std140) uniform UniformCommonBufferBlock { common_data constant; }
constantCommon;

/*	*/
layout(set = 1, binding = 2, std140) uniform UniformNodeBufferBlock { Node node[512]; }
NodeUBO;

/*	*/
layout(set = 1, binding = 3, std140) uniform UniformSkeletonBufferBlock { mat4 gBones[512]; }
skeletonUBO;

/*	*/
layout(set = 1, binding = 4, std140) uniform UniformMaterialBufferBlock { material materials[512]; }
MaterialUBO;

/*	*/
layout(set = 2, binding = 5, std140) uniform UniformLightBufferBlock { light_settings light; }
LightUBO;

/*	*/
layout(binding = 0) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2D NormalTexture;
layout(binding = 2) uniform sampler2D AlphaMaskedTexture;

/*	*/
layout(binding = 5) uniform sampler2D RoughnessTexture;
layout(binding = 6) uniform sampler2D MetalicTexture;
layout(binding = 4) uniform sampler2D EmissionTexture;
layout(binding = 7) uniform sampler2D DisplacementTexture;
layout(binding = 8) uniform sampler2D AOTexture;

/*	*/
layout(binding = 10) uniform sampler2D IrradianceTexture;
layout(binding = 11) uniform samplerCube prefilterMap;
layout(binding = 12) uniform sampler2D brdfLUT;


material getMaterial() { return MaterialUBO.materials[0]; }


#endif