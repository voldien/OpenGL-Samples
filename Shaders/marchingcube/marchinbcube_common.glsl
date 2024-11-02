#include "common.glsl"

struct Vertex {
	vec3 pos;
	float scale;
	vec3 normal;
	float size;
};

struct MarchingCubeCellData {
	float voxel_size;
	float threshold;
	float mag;
	float scale;
	vec4 position_offset;
	vec4 random_offset;
};

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	FogSettings fogSettings;

	MarchingCubeCellData settings;
}
ubo;