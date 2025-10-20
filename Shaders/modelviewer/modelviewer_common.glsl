#ifndef _MODELVIEWER_COMMON_
#define _MODELVIEWER_COMMON_ 1

#include "pbr_common.glsl"

#define USE_TESSELLATION 0x1
#define TESSELLATION_SIMPLE 0x2

/*	*/
layout(constant_id = 64) const uint UseTessellation = USE_TESSELLATION | TESSELLATION_SIMPLE;

#endif
