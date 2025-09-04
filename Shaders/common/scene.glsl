#ifndef _COMMON_SCENE_H_
#define _COMMON_SCENE_H_ 1

//TODO: relocate
#extension GL_ARB_texture_cube_map_array : enable

#include "common.glsl"
#include "light.glsl"
#include "material.glsl"
#include "transformation.glsl"

/*	*/
layout(constant_id = 16) const int MAX_BONES = 512;
layout(constant_id = 17) const int MAX_BONE_INFLUENCE = 4;

struct tessellation_settings {
	float tessLevel;
	float gDispFactor;
};

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
	// ivec4 frame;
};

struct Node {
	mat4 model; /*	*/
};

struct light_settings {
	//TODO: seperate for shadow data.

	DirectionalLight directional[16];
	PointLight point[64];
	uint directionalCount;
	uint pointCount;
};

/*	*/
layout(set = 1, binding = 1, std140) uniform UniformCommonBufferBlock { common_data constant; }
constantCommon;

/*	*/
layout(set = 1, binding = 2, std140) uniform UniformNodeBufferBlock { Node node[512]; }
NodeUBO;

/*	*/
layout(set = 1, binding = 6, std140) uniform UniformPrevNodeBufferBlock { Node node[512]; }
NodePrevUBO;

/*	*/
layout(set = 1, binding = 3, std140) uniform UniformSkeletonBufferBlock { mat4 gBones[1024]; }
skeletonUBO;

/*	*/
layout(set = 1, binding = 4, std140) uniform UniformMaterialBufferBlock {
	material materials[128];
	tessellation_settings tessellation[128];
}
MaterialUBO;

/*	*/
layout(set = 2, binding = 5, std140) uniform UniformLightBufferBlock { light_settings light; }
LightUBO;

/*	*/
layout(set = 0, binding = 0) uniform sampler2D DiffuseTexture;
layout(set = 0, binding = 1) uniform sampler2D NormalTexture;
layout(set = 0, binding = 2) uniform sampler2D AlphaMaskedTexture;

/*	Physical Based Material Textures.	*/
layout(set = 0, binding = 3) uniform sampler2D RoughnessTexture;
layout(set = 0, binding = 8) uniform sampler2D MetalicTexture;
layout(set = 0, binding = 4) uniform sampler2D EmissionTexture;
layout(set = 0, binding = 7) uniform sampler2D DisplacementTexture;
layout(set = 0, binding = 6) uniform sampler2D AOTexture;

/*	*/
layout(set = 0, binding = 9) uniform sampler2D BackBufferTexture;
layout(set = 2, binding = 13) uniform sampler2D CameraDepthTexture;

/*	Image Based Lightning Textures.	*/
layout(set = 1, binding = 10) uniform samplerCube IrradianceTexture; /*	*/
layout(set = 1, binding = 11) uniform sampler2D prefilterMap;	   /*	*/
layout(set = 1, binding = 12) uniform sampler2D BRDFLUT;		   /*	*/


/*	Light.	*/
layout(set = 3, binding = 20) uniform samplerCube PointShadowTexture[4];
layout(set = 3, binding = 24) uniform sampler2DShadow DirectionalShadowTexture[4];

mat4 getModel(const in int index) { return NodeUBO.node[index].model; }
mat4 getModel() { return getModel(0); }

/*	*/
material getMaterial(const int index) { return MaterialUBO.materials[index]; }
material getMaterial() { return getMaterial(0); }

/*	*/
uint getDirectionalLightCount() { return LightUBO.light.directionalCount; }
uint getPointLightCount() { return LightUBO.light.pointCount; }

/*	*/
DirectionalLight getDirectional(const in uint index) { return LightUBO.light.directional[index]; }
PointLight getPointLight(const in uint index) { return LightUBO.light.point[index]; }

/*	*/
Camera getCamera() { return constantCommon.constant.camera; }

// TODO: use transform functions here
vec3 scene_world_to_view(const in vec3 x) { return (constantCommon.constant.camera.view * vec4(x, 1)).xyz; }

#endif
