#ifndef _COMMON_SCENE_H_
#define _COMMON_SCENE_H_ 1

/*	*/
struct material {
	vec4 colorTint;
	vec4 transparency;
	vec4 specular_roughness;
	vec4 emission;
	vec4 clip_;
};

#endif