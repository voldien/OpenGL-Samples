#ifndef _COMMON_MATERIALS_H_
#define _COMMON_MATERIALS_H_ 1

/*	*/
struct material {
	vec4 ambientColor;
	vec4 diffuseColor;
	vec4 transparency;
	vec4 specular_roughness;
	vec4 emission;
	vec4 clip_;
};

#endif