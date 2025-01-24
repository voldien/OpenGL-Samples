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
struct culling {
	uint insideCount;
};

#endif