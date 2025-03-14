#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;

layout(location = 0) out vec3 vertex;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec4 instanceColor;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	/*	Settings.	*/
	uint nrFaces;
	uint nrElements;
	float delta;
	float scale;
}
ubo;

struct _instance_data_t {
	mat4 model;
	vec4 color;
};

layout(binding = 1, std140) readonly restrict buffer UniformInstanceBlock { _instance_data_t model[]; }
instance_ubo;

float rand(const in vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453); }

void main() {
	const mat4 model = instance_ubo.model[gl_InstanceID].model;
	const vec4 instance_color = instance_ubo.model[gl_InstanceID].color;

	gl_Position = ubo.proj * ubo.view * model * vec4(Vertex, 1.0);
	uv = TextureCoord;

	normal = normalize((model * vec4(Normal, 0.0)).xyz);
	instanceColor =
		instance_color; // vec4(abs(rand(vec2(gl_InstanceID, 0))), 0, abs(rand(vec2(gl_InstanceID, 10))), 1) + ;
}
