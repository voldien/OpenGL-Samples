#ifndef _DEBUG_DRAWER_H_
#define _DEBUG_DRAWER_H_ 1


layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;
}
ubo;

struct InstanceData{
	mat4 model;
	vec4 color;
};

layout(binding = 1, std140) uniform UniformInstanceBlock {
	InstanceData ins[512];
}
instance_ubo;

#endif
