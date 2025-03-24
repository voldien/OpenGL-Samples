#include "common.glsl"

const uint chunk_size = 10;
const uint chunk_size2 = chunk_size * chunk_size;
const uint num_point_4_cell = 15;
const uint nr_points_per_cell = (num_point_4_cell * 9 * 9 * 9);

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

// layout(set = 0, binding = 1, std420) uniform buffer UniformBufferBlock {
// 	MarchingCubeCellData settings;
// }
// ubo;