#ifndef _COMMON_SCENE_H_
#define _COMMON_SCENE_H_ 1

// TODO: relocate
#extension GL_ARB_texture_cube_map_array : enable

#include "common.glsl"
#include "frustum.glsl"
#include "light.glsl"
#include "material.glsl"
#include "transformation.glsl"

/*	Shadow Rendering Options.	*/
#define SHADOW_MODE_HARD 0x1
#define SHADOW_MODE_SOFT 0x2
#define SHADOW_MODE_VARIANCE 0x4

/*	Rendering Feature Modes.	*/
#define RENDERING_MODE_EARLY_DEPTH 0x1
#define RENDERING_MODE_CLIPPING 0x2

/*	Scene Rendering Options.	*/
layout(constant_id = 12) const bool UseClipping = true;
layout(constant_id = 13) const uint ShadowMapMode = SHADOW_MODE_SOFT; /*	*/

layout(constant_id = 15) const uint RenderingMode = 0;

/*	*/
layout(constant_id = 16) const int MAX_BONES = 512;
layout(constant_id = 17) const int MAX_BONE_INFLUENCE = 4;

struct tessellation_settings {
	float tessLevel;
	float gDispFactor;
};

struct global_rendering_settings {
	vec4 ambientColor;	/*	*/
	vec4 specularColor; /*	*/
};

struct common_data {
	Camera camera;
	Frustum frustum;

	global_rendering_settings globalSettings;

	mat4 view[3];
	mat4 proj[3];

	vec4 time; /*	Delta, */
			   // ivec4 frame;
};

struct Node {
	mat4 model; /*	*/
};

struct light_settings {

	// ShadowLight shadows;
	//  TODO: seperate for shadow data.
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
layout(set = 1, binding = 5, std140) uniform UniformSkeletonPrevBufferBlock { mat4 gBones[1024]; }
skeletonUBOPrev;

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
/*	Depth/FrameBuffer Textures.	*/
layout(set = 2, binding = 9) uniform sampler2D BackBufferTexture;
layout(set = 2, binding = 13) uniform sampler2D CameraDepthTexture;

/*	Image Based Lightning Textures.	*/
layout(set = 1, binding = 10) uniform samplerCube IrradianceTexture; /*	*/
layout(set = 1, binding = 11) uniform sampler2D prefilterMap;		 /*	*/
layout(set = 1, binding = 12) uniform sampler2D BRDFLUT;			 /*	*/
/*	Light Set.	*/
layout(set = 3, binding = 20) uniform samplerCube PointShadowTexture[4];
layout(set = 3, binding = 24) uniform sampler2DShadow DirectionalShadowTexture[4];

/*	*/
float getElapsedTime() { return constantCommon.constant.time.x; }
float getDeltaTime() { return constantCommon.constant.time.y; }

/*	*/
Camera getCamera() { return constantCommon.constant.camera; }
Camera getCamera(const in uint index) { return constantCommon.constant.camera; }
Frustum getFrustm() { return constantCommon.constant.frustum; }
Frustum getFrustm(const in uint index) { return constantCommon.constant.frustum; }

global_rendering_settings getRenderingSettings() { return constantCommon.constant.globalSettings; }

/*	Transformation based on Camera and common data.	*/
// TODO: use transform functions here
vec3 scene_world_to_view(const in vec3 x) { return (constantCommon.constant.camera.view * vec4(x, 1)).xyz; }

mat4 getModel(const in uint index) { return NodeUBO.node[index].model; }
mat4 getModel() { return getModel(0); }

mat4 getPrevModel(const in uint index) { return NodePrevUBO.node[index].model; }

vec3 getVelocity(const uint model_index, const in vec3 objectVertex) {
	const vec3 prevVertex = (getModel(model_index) * vec4(objectVertex, 1)).xyz;
	const vec3 currVertex = (getModel(model_index) * vec4(objectVertex, 1)).xyz;

	return (prevVertex - currVertex);
}

/*	*/
material getMaterial(const uint index) { return MaterialUBO.materials[index]; }
material getMaterial() { return getMaterial(0); }

/*	*/
uint getDirectionalLightCount() { return LightUBO.light.directionalCount; }
uint getPointLightCount() { return LightUBO.light.pointCount; }

/*	*/
DirectionalLight getDirectional(const in uint index) { return LightUBO.light.directional[index]; }
PointLight getPointLight(const in uint index) { return LightUBO.light.point[index]; }

#endif
