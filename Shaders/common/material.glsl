#ifndef _COMMON_MATERIALS_H_
#define _COMMON_MATERIALS_H_ 1

/*	*/
struct material {
	ivec4 info;				 /*	*/
	vec4 ambientColor;		 /*	Ambient Color.	*/
	vec4 diffuseColor;		 /*	Diffuse Color.	*/
	vec4 transparency;		 /*	Transparent/Transmission.	*/
	vec4 specular_roughness; /*	Specular Color, roughness in Alpha Channel*/
	vec4 emission;			 /*	Emission Color.	*/
	vec4 clip_;				 /*	Multiple parameters packaged, (Clipping,)	 */
};

#endif
