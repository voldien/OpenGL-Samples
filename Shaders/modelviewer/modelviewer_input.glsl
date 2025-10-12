#ifndef _MODELVIEWER_INPUT_
#define _MODELVIEWER_INPUT_ 1

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
layout(location = 4) in uvec4 BoneIDs; /*	*/
layout(location = 5) in vec4 Weights;  /*	*/

/*	*/
layout(location = 8) in ivec2 vAssigns;

#endif