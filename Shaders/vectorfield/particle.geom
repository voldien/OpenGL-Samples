#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(points) in;
layout(triangle_strip) out;
layout(max_vertices = 4) out;

struct particle_setting {
	float speed;
	float lifetime;
	float gravity;
};

layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	*/
	float deltaTime;
	float time;
	float zoom;
	vec4 ambientColor;
	vec4 color;

	particle_setting setting;
}
ubo;

layout(location = 0) smooth out vec2 uv;
layout(location = 1) smooth out vec4 gColor;
layout(location = 0) in vec4 velocity[];

void main() {
	const int noffsets = 4;
	int i, j;

	/*	Polygone offset.	*/
	const vec3 polyoffset[] = vec3[](vec3(1.0, 1.0, 0), vec3(1.0, -1.0, 0), vec3(-1.0, 1.0, 0), vec3(-1.0, -1.0, 0));

	/*	UV  coordinate.	*/
	const vec2 cUV[] = vec2[](vec2(1.0, 1.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(0.0, 0.0));

	/*  Create Billboard quad.    */
	for (i = 0; i < gl_in.length(); i++) {
		for (j = 0; j < noffsets; j++) {

			/*	Compute inverse zoom - expressed as a polynominal.	*/
			const float invZoom = (1.0f / ubo.zoom + ubo.zoom * (1.0f / 150.0f)) * 0.5f;
			const vec3 particlePos = gl_in[i].gl_Position.xyz + polyoffset[j] * invZoom;

			/*	Velocity.	*/
			// const vec2 velocity = gl_in[i].gl_Position.zw;

			/*	*/
			gl_Position = ubo.modelViewProjection * vec4(particlePos, 1.0);

			/*	Compute particle color.	*/
			const float reduce = (1.0 / 20.0);
			const float green = (1.0 / 20.0);
			const float blue = (1.0 / 50.0);
			gColor =
				vec4(velocity[0].x * 10 * reduce * length(velocity[0]), velocity[0].x * green, length(velocity[0]) * blue, 1.0);
			uv = cUV[j];
			EmitVertex();
		}
	}
	EndPrimitive();
}
