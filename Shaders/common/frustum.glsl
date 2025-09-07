#ifndef _COMMON_FRUSTUM_
#define _COMMON_FRUSTUM_ 1

struct plan {
	vec3 normal; /*	*/
	float d;	 /*	*/
};
struct Sphere {
	vec3 center;
	float radius;
};

struct AABB {
	vec3 center;
	vec3 halfSize;
};

struct culling {
	uint insideCount;
};

bool isPointInsidePlane(const in plan plan, const in vec3 point) { return true; }

bool isSphereInsidePlane(const in plan plan, const in Sphere sphere) { return true; }

#endif