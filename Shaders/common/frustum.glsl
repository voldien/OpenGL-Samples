#ifndef _COMMON_FRUSTUM_
#define _COMMON_FRUSTUM_ 1

#include "common.glsl"

struct HyperPlane {
	vec3 normal; /*	*/
	float d;	 /*	*/
};

struct BoundingSphere {
	vec3 center;
	float radius;
};

struct AABB {
	vec3 min, max;
};

struct BoundingBox {
	vec3 center;
	vec3 halfSize;
};

struct culling {
	uint insideCount;
};

bool isInClipSpace(const in vec3 vertexClipSpace) {
	return vertexClipSpace.x < -1 || vertexClipSpace.x > 1 || vertexClipSpace.y < -1 || vertexClipSpace.y > 1;
}

bool isInClipSpace(const in vec4 vertex) {
	const vec3 v0ClipSpace = vertex.xyz / vertex.w;
	return isInClipSpace(v0ClipSpace);
}

bool isPlaneInsidePlane(const in HyperPlane plan, const in vec3 point) { return true; }

bool isPointInsidePlane(const in HyperPlane plan, const in vec3 point) { return true; }

bool isSphereInsidePlane(const in HyperPlane plan, const in BoundingSphere sphere) { return true; }

bool isPlaneInsideFrustum(const in Frustum frustum, const in HyperPlane plan) { return true; }

#endif